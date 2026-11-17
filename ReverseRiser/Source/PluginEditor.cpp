#include "PluginEditor.h"

namespace
{
    const juce::Colour bg     { 0xff14161c };
    const juce::Colour panel  { 0xff1e222b };
    const juce::Colour accent { 0xff7c5cff };

    struct KnobDef { const char* id; const char* name; };
    const KnobDef knobDefs[] = {
        { "tail", "Tail" },      { "size", "Room Size" }, { "damping", "Damping" },
        { "sibilance", "Sibilance" }, { "dry", "Dry Clip" },
        { "hp", "Low Cut" },     { "lp", "High Cut" },    { "drive", "Drive" },
        { "width", "Width" },    { "output", "Output" }
    };
}

//==============================================================================
void DragPad::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (panel);
    g.fillRoundedRectangle (r, 8.0f);
    g.setColour (accent.withAlpha (0.5f));
    g.drawRoundedRectangle (r.reduced (0.5f), 8.0f, 1.0f);

    if (! proc.hasRender())
    {
        g.setColour (juce::Colours::grey);
        g.setFont (15.0f);
        g.drawText ("Capture audio or load a file, then Render", getLocalBounds(),
                    juce::Justification::centred);
        return;
    }

    auto peaks = proc.getPeaks();
    if (! peaks.empty())
    {
        auto area = r.reduced (10.0f, 8.0f);
        const float mid = area.getCentreY();
        const float half = area.getHeight() * 0.5f;
        juce::Path p;
        for (size_t i = 0; i < peaks.size(); ++i)
        {
            const float x = area.getX() + area.getWidth() * (float) i / (float) (peaks.size() - 1);
            p.addLineSegment ({ x, mid - peaks[i] * half, x, mid + peaks[i] * half }, 1.0f);
        }
        g.setColour (accent);
        g.strokePath (p, juce::PathStrokeType (1.5f));
    }

    g.setColour (juce::Colours::white.withAlpha (0.8f));
    g.setFont (12.0f);
    g.drawText ("Drag this into your playlist  |  " + juce::String (proc.getRenderedSeconds(), 1) + " s",
                getLocalBounds().removeFromBottom (18), juce::Justification::centred);
}

void DragPad::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging || e.getDistanceFromDragStart() < 6 || ! proc.hasRender())
        return;

    auto f = proc.getExportFile();
    if (f.existsAsFile())
    {
        dragging = true;
        juce::DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this);
    }
}

//==============================================================================
RiserEditor::RiserEditor (RiserProcessor& p)
    : AudioProcessorEditor (&p), proc (p), pad (p)
{
    setSize (640, 450);

    for (size_t i = 0; i < knobs.size(); ++i)
    {
        auto& k = knobs[i];
        k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 18);
        k.slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
        k.label.setText (knobDefs[i].name, juce::dontSendNotification);
        k.label.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (k.slider);
        addAndMakeVisible (k.label);
        k.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            proc.apvts, knobDefs[i].id, k.slider);
    }

    for (auto* b : { &captureButton, &loadButton, &renderButton, &playButton })
        addAndMakeVisible (b);
    addAndMakeVisible (autoButton);
    addAndMakeVisible (status);
    addAndMakeVisible (pad);

    autoButton.setToggleState (true, juce::dontSendNotification);
    status.setJustificationType (juce::Justification::centredRight);
    status.setColour (juce::Label::textColourId, juce::Colours::lightgrey);

    captureButton.onClick = [this]
    {
        if (proc.isCapturing())
        {
            proc.stopCapture();
            if (proc.renderRiser()) lastRenderedChange = lastSeenChange = proc.getChangeCounter();
        }
        else
            proc.startCapture();
    };

    loadButton.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Select an audio file", juce::File(),
                                                       "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [this] (const juce::FileChooser& fc)
                              {
                                  auto f = fc.getResult();
                                  if (f.existsAsFile() && proc.loadFile (f))
                                  {
                                      proc.renderRiser();
                                      lastRenderedChange = lastSeenChange = proc.getChangeCounter();
                                  }
                              });
    };

    renderButton.onClick = [this]
    {
        proc.renderRiser();
        lastRenderedChange = lastSeenChange = proc.getChangeCounter();
    };

    playButton.onClick = [this]
    {
        if (proc.isPreviewing()) proc.stopPreview();
        else                     proc.startPreview();
    };

    startTimerHz (10);
}

RiserEditor::~RiserEditor() { stopTimer(); }

void RiserEditor::timerCallback()
{
    captureButton.setButtonText (proc.isCapturing() ? "Stop Capture" : "Capture");
    captureButton.setColour (juce::TextButton::buttonColourId,
                             proc.isCapturing() ? juce::Colours::darkred : juce::Colour (0xff2c313d));
    playButton.setButtonText (proc.isPreviewing() ? "Stop" : "Play");

    if (proc.isCapturing())
        status.setText ("Capturing... " + juce::String (proc.getCapturedSeconds(), 1) + " s (play your audio now)",
                        juce::dontSendNotification);
    else if (proc.hasCapture())
        status.setText ("Clip: " + juce::String (proc.getCapturedSeconds(), 1) + " s", juce::dontSendNotification);
    else
        status.setText ("Press Capture, then play audio through this track", juce::dontSendNotification);

    // debounced re-render when a knob moved
    const int c = proc.getChangeCounter();
    if (c != lastSeenChange)
        lastSeenChange = c;
    else if (lastSeenChange != lastRenderedChange && autoButton.getToggleState()
             && proc.hasCapture() && ! proc.isCapturing())
    {
        proc.renderRiser();
        lastRenderedChange = lastSeenChange;
    }

    pad.repaint();
}

void RiserEditor::paint (juce::Graphics& g)
{
    g.fillAll (bg);
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (24.0f, juce::Font::bold));
    g.drawText ("REVERSE RISER", 20, 8, 300, 36, juce::Justification::centredLeft);
}

void RiserEditor::resized()
{
    auto area = getLocalBounds().reduced (20);
    area.removeFromTop (40);

    auto row = area.removeFromTop (34);
    captureButton.setBounds (row.removeFromLeft (110));
    row.removeFromLeft (8);
    loadButton.setBounds (row.removeFromLeft (100));
    row.removeFromLeft (8);
    renderButton.setBounds (row.removeFromLeft (90));
    row.removeFromLeft (8);
    playButton.setBounds (row.removeFromLeft (70));
    row.removeFromLeft (12);
    autoButton.setBounds (row.removeFromLeft (130));

    area.removeFromTop (6);
    status.setBounds (area.removeFromTop (20));
    area.removeFromTop (6);
    pad.setBounds (area.removeFromTop (110));
    area.removeFromTop (12);

    const int colW = area.getWidth() / 5;
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        const int col = (int) i % 5, rowIdx = (int) i / 5;
        juce::Rectangle<int> cell (area.getX() + col * colW, area.getY() + rowIdx * 100, colW, 100);
        knobs[i].label.setBounds (cell.removeFromTop (16));
        knobs[i].slider.setBounds (cell);
    }
}
