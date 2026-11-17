#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_dsp/juce_dsp.h>

class RiserProcessor : public juce::AudioProcessor,
                       private juce::AudioProcessorValueTreeState::Listener
{
public:
    RiserProcessor();
    ~RiserProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Reverse Riser"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // ---- message-thread API used by the editor ----
    void startCapture();
    void stopCapture();
    bool isCapturing() const { return capturing.load(); }
    bool loadFile (const juce::File&);
    bool renderRiser();
    void startPreview();
    void stopPreview();
    bool isPreviewing() const { return previewing.load(); }

    bool hasCapture() const { return capturedLength.load() > 0; }
    bool hasRender() const { return renderedLength.load() > 0; }
    double getCapturedSeconds() const;
    double getRenderedSeconds() const;
    int getChangeCounter() const { return changeCounter.load(); }
    std::vector<float> getPeaks() const;
    juce::File getExportFile() const { return exportFile; }

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void parameterChanged (const juce::String&, float) override { ++changeCounter; }
    float param (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    void writeExportFile (const juce::AudioBuffer<float>&);

    double currentSR = 44100.0;
    int capacity = 0;
    juce::AudioBuffer<float> capturedBuffer { 2, 0 };
    std::atomic<int> capturePos { 0 }, capturedLength { 0 };
    std::atomic<bool> capturing { false }, previewing { false };

    mutable juce::SpinLock renderLock;
    juce::AudioBuffer<float> renderedBuffer { 2, 0 };
    std::atomic<int> renderedLength { 0 }, previewPos { 0 };
    std::vector<float> peaks;
    std::atomic<int> changeCounter { 0 };
    juce::File exportFile;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RiserProcessor)
};
