#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    constexpr double maxCaptureSeconds = 20.0;
    const char* paramIds[] = { "tail", "size", "damping", "sibilance", "dry",
                               "hp", "lp", "drive", "width", "output" };

    // Reduces harsh high-frequency (sibilant) bursts. Used BEFORE the reverb,
    // so "S" sounds do not get smeared backwards into the riser.
    void deEss (juce::AudioBuffer<float>& b, int n, float amount, double sr)
    {
        if (amount <= 0.001f || n <= 0 || sr < 16000.0)
            return;

        auto coef = juce::dsp::IIR::Coefficients<float>::makeHighPass (sr, 6000.0f, 0.707f);
        juce::dsp::IIR::Filter<float> f1[2], f2[2];
        for (int c = 0; c < 2; ++c) { f1[c].coefficients = coef; f2[c].coefficients = coef; }

        std::vector<float> hp0 ((size_t) n), hp1 ((size_t) n);
        double sum = 0.0;
        for (int i = 0; i < n; ++i)
        {
            hp0[(size_t) i] = f2[0].processSample (f1[0].processSample (b.getSample (0, i)));
            hp1[(size_t) i] = f2[1].processSample (f1[1].processSample (b.getSample (1, i)));
            sum += (double) hp0[(size_t) i] * hp0[(size_t) i] + (double) hp1[(size_t) i] * hp1[(size_t) i];
        }

        const float rms = (float) std::sqrt (sum / (2.0 * n));
        const float thr = std::max (1.5f * rms, 1.0e-5f);
        const float atk = (float) std::exp (-1.0 / (0.001 * sr));
        const float rel = (float) std::exp (-1.0 / (0.030 * sr));
        float env = 0.0f;

        for (int i = 0; i < n; ++i)
        {
            const float level = std::max (std::abs (hp0[(size_t) i]), std::abs (hp1[(size_t) i]));
            env = level > env ? atk * env + (1.0f - atk) * level
                              : rel * env + (1.0f - rel) * level;
            const float g = env > thr ? std::pow (thr / env, amount * 1.5f) : 1.0f;

            b.setSample (0, i, b.getSample (0, i) - hp0[(size_t) i] + hp0[(size_t) i] * g);
            b.setSample (1, i, b.getSample (1, i) - hp1[(size_t) i] + hp1[(size_t) i] * g);
        }
    }
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout RiserProcessor::createLayout()
{
    using namespace juce;
    using Range = NormalisableRange<float>;
    using Attr  = AudioParameterFloatAttributes;

    std::vector<std::unique_ptr<RangedAudioParameter>> p;
    auto add = [&p] (const char* id, const char* name, Range r, float def, const char* unit = "")
    {
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, r, def,
                                                            Attr().withLabel (unit)));
    };

    add ("tail",      "Tail Length", Range (0.5f, 10.0f, 0.01f), 3.0f, "s");
    add ("size",      "Room Size",   Range (0.0f, 1.0f, 0.001f), 0.8f);
    add ("damping",   "Damping",     Range (0.0f, 1.0f, 0.001f), 0.5f);
    add ("sibilance", "Sibilance",   Range (0.0f, 1.0f, 0.001f), 0.7f);
    add ("dry",       "Dry Clip",    Range (0.0f, 1.0f, 0.001f), 0.0f);
    add ("hp",        "Low Cut",     Range (20.0f, 2000.0f, 1.0f, 0.4f), 120.0f, "Hz");
    add ("lp",        "High Cut",    Range (1000.0f, 20000.0f, 1.0f, 0.4f), 14000.0f, "Hz");
    add ("drive",     "Drive",       Range (0.0f, 1.0f, 0.001f), 0.0f);
    add ("width",     "Width",       Range (0.0f, 1.5f, 0.001f), 1.0f);
    add ("output",    "Output",      Range (-24.0f, 6.0f, 0.1f), 0.0f, "dB");

    return { p.begin(), p.end() };
}

RiserProcessor::RiserProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    for (auto* id : paramIds)
        apvts.addParameterListener (id, this);
}

RiserProcessor::~RiserProcessor()
{
    for (auto* id : paramIds)
        apvts.removeParameterListener (id, this);

    if (exportFile.existsAsFile())
        exportFile.deleteFile();
}

bool RiserProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    return l.getMainInputChannelSet()  == juce::AudioChannelSet::stereo()
        && l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void RiserProcessor::prepareToPlay (double sampleRate, int)
{
    const int newCap = (int) (maxCaptureSeconds * sampleRate);
    if (newCap != capacity || sampleRate != currentSR)
    {
        currentSR = sampleRate;
        capacity = newCap;
        capturedBuffer.setSize (2, capacity);
        capturedBuffer.clear();
        capturePos = 0;
        capturedLength = 0;
        capturing = false;
    }
}

void RiserProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int numCh = buffer.getNumChannels();

    if (capturing.load())
    {
        const int pos = capturePos.load();
        const int toCopy = juce::jmin (n, capacity - pos);
        if (toCopy > 0 && numCh > 0)
        {
            for (int ch = 0; ch < 2; ++ch)
                capturedBuffer.copyFrom (ch, pos, buffer, juce::jmin (ch, numCh - 1), 0, toCopy);
            capturePos.store (pos + toCopy);
        }
        if (toCopy < n)
            capturing = false; // buffer full
        // audio passes through unchanged while capturing
    }
    else if (previewing.load())
    {
        const juce::SpinLock::ScopedTryLockType lock (renderLock);
        if (lock.isLocked() && renderedBuffer.getNumSamples() > 0)
        {
            const int len = renderedBuffer.getNumSamples();
            const int pos = previewPos.load();
            const int toCopy = juce::jmax (0, juce::jmin (n, len - pos));

            buffer.clear();
            for (int ch = 0; ch < numCh; ++ch)
                if (toCopy > 0)
                    buffer.copyFrom (ch, 0, renderedBuffer, juce::jmin (ch, 1), pos, toCopy);

            previewPos.store (pos + toCopy);
            if (pos + toCopy >= len)
                previewing = false;
        }
    }
}

//==============================================================================
void RiserProcessor::startCapture()
{
    if (capacity <= 0) return;
    stopPreview();
    capturePos = 0;
    capturedLength = 0;
    capturing = true;
}

void RiserProcessor::stopCapture()
{
    if (! capturing.exchange (false)) return;
    capturedLength = capturePos.load();
}

bool RiserProcessor::loadFile (const juce::File& f)
{
    if (capturing.load() || capacity <= 0) return false;

    juce::AudioFormatManager fm;
    fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader (fm.createReaderFor (f));
    if (reader == nullptr || reader->lengthInSamples <= 0) return false;

    // cap very long files before reading
    const auto maxSrc = (juce::int64) (maxCaptureSeconds * reader->sampleRate);
    const int len = (int) juce::jmin (reader->lengthInSamples, maxSrc);

    juce::AudioBuffer<float> tmp (2, len);
    reader->read (&tmp, 0, len, 0, true, true);

    juce::AudioBuffer<float> res;
    if (std::abs (reader->sampleRate - currentSR) > 1.0)
    {
        const double ratio = reader->sampleRate / currentSR;
        const int outLen = juce::jmin (capacity, (int) (len / ratio));
        res.setSize (2, outLen);
        for (int ch = 0; ch < 2; ++ch)
        {
            juce::LagrangeInterpolator interp;
            interp.process (ratio, tmp.getReadPointer (ch), res.getWritePointer (ch), outLen);
        }
    }
    else
    {
        res = std::move (tmp);
    }

    const int n = juce::jmin (capacity, res.getNumSamples());
    stopPreview();
    capturedBuffer.clear();
    for (int ch = 0; ch < 2; ++ch)
        capturedBuffer.copyFrom (ch, 0, res, ch, 0, n);
    capturePos = n;
    capturedLength = n;
    return true;
}

bool RiserProcessor::renderRiser()
{
    const int L = capturedLength.load();
    if (capturing.load() || L < (int) (0.05 * currentSR))
        return false;

    const double sr = currentSR;
    const int T = (int) (param ("tail") * sr);
    const int total = L + T;

    // 1) reverse the clip, leaving silence after it for the reverb tail
    juce::AudioBuffer<float> work (2, total);
    work.clear();
    for (int ch = 0; ch < 2; ++ch)
    {
        work.copyFrom (ch, 0, capturedBuffer, ch, 0, L);
        std::reverse (work.getWritePointer (ch), work.getWritePointer (ch) + L);
    }
    const float ref = juce::jmax (0.01f, capturedBuffer.getMagnitude (0, L));

    // 2) remove harsh sibilance so it does not echo backwards
    deEss (work, L, param ("sibilance"), sr);

    // 3) reverb (wet only)
    juce::Reverb reverb;
    reverb.setSampleRate (sr);
    juce::Reverb::Parameters rp;
    rp.roomSize = param ("size");
    rp.damping = param ("damping");
    rp.wetLevel = 1.0f;
    rp.dryLevel = 0.0f;
    rp.width = 1.0f;
    rp.freezeMode = 0.0f;
    reverb.setParameters (rp);
    reverb.processStereo (work.getWritePointer (0), work.getWritePointer (1), total);

    // 4) tone shaping + drive on the wet signal
    {
        const float hpF = param ("hp");
        const float lpF = juce::jmin (param ("lp"), (float) (sr * 0.45));
        juce::dsp::IIR::Filter<float> hp[2], lp[2];
        for (int c = 0; c < 2; ++c)
        {
            hp[c].coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass (sr, hpF, 0.707f);
            lp[c].coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass (sr, lpF, 0.707f);
        }

        const float drive = param ("drive");
        const float k = 1.0f + drive * 9.0f;

        for (int c = 0; c < 2; ++c)
        {
            auto* d = work.getWritePointer (c);
            for (int i = 0; i < total; ++i)
            {
                float s = lp[c].processSample (hp[c].processSample (d[i]));
                if (drive > 0.001f)
                    s = std::tanh (k * s / ref) * ref;
                d[i] = s;
            }
        }

        // stereo width (mid/side)
        const float width = param ("width");
        auto* l = work.getWritePointer (0);
        auto* r = work.getWritePointer (1);
        for (int i = 0; i < total; ++i)
        {
            const float m = 0.5f * (l[i] + r[i]);
            const float s = 0.5f * (l[i] - r[i]) * width;
            l[i] = m + s;
            r[i] = m - s;
        }
    }

    // 5) reverse everything: the tail now swells up into the original clip
    for (int ch = 0; ch < 2; ++ch)
        std::reverse (work.getWritePointer (ch), work.getWritePointer (ch) + total);

    // 6) optionally mix the original clip back in at the end
    const float dry = param ("dry");
    if (dry > 0.001f)
        for (int ch = 0; ch < 2; ++ch)
            work.addFrom (ch, T, capturedBuffer, ch, 0, L, dry);

    // 7) tiny fades to avoid clicks, then normalise to -1 dBFS + output gain
    const int fadeIn  = juce::jmin (total, (int) (0.02 * sr));
    const int fadeOut = juce::jmin (total, (int) (0.005 * sr));
    work.applyGainRamp (0, fadeIn, 0.0f, 1.0f);
    work.applyGainRamp (total - fadeOut, fadeOut, 1.0f, 0.0f);

    const float peak = work.getMagnitude (0, total);
    if (peak > 1.0e-6f)
        work.applyGain (0.891f / peak * juce::Decibels::decibelsToGain (param ("output")));

    // 8) waveform overview
    std::vector<float> newPeaks (512, 0.0f);
    for (int b = 0; b < 512; ++b)
    {
        const int s0 = (int) ((juce::int64) b * total / 512);
        const int s1 = juce::jmax (s0 + 1, (int) ((juce::int64) (b + 1) * total / 512));
        newPeaks[(size_t) b] = juce::jmax (work.getMagnitude (0, s0, s1 - s0),
                                           work.getMagnitude (1, s0, s1 - s0));
    }

    writeExportFile (work);

    {
        const juce::SpinLock::ScopedLockType lock (renderLock);
        renderedBuffer = std::move (work);
        peaks = std::move (newPeaks);
        renderedLength = renderedBuffer.getNumSamples();
    }
    return true;
}

void RiserProcessor::writeExportFile (const juce::AudioBuffer<float>& buf)
{
    auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("ReverseRiser");
    dir.createDirectory();

    if (exportFile.existsAsFile())
        exportFile.deleteFile();

    exportFile = dir.getChildFile ("Riser_" + juce::Time::getCurrentTime().formatted ("%Y%m%d_%H%M%S") + ".wav");

    auto stream = std::make_unique<juce::FileOutputStream> (exportFile);
    if (! stream->openedOk())
        return;

    juce::WavAudioFormat wav;
    std::unique_ptr<juce::AudioFormatWriter> writer (
        wav.createWriterFor (stream.get(), currentSR, 2, 24, {}, 0));
    if (writer != nullptr)
    {
        stream.release(); // writer owns it now
        writer->writeFromAudioSampleBuffer (buf, 0, buf.getNumSamples());
    }
}

void RiserProcessor::startPreview()
{
    if (! hasRender() || capturing.load()) return;
    previewPos = 0;
    previewing = true;
}

void RiserProcessor::stopPreview() { previewing = false; }

double RiserProcessor::getCapturedSeconds() const
{
    const int n = capturing.load() ? capturePos.load() : capturedLength.load();
    return currentSR > 0 ? n / currentSR : 0.0;
}

double RiserProcessor::getRenderedSeconds() const
{
    return currentSR > 0 ? renderedLength.load() / currentSR : 0.0;
}

std::vector<float> RiserProcessor::getPeaks() const
{
    const juce::SpinLock::ScopedLockType lock (renderLock);
    return peaks;
}

//==============================================================================
juce::AudioProcessorEditor* RiserProcessor::createEditor() { return new RiserEditor (*this); }

void RiserProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void RiserProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new RiserProcessor(); }
