#pragma once
#include "PluginProcessor.h"

// Shows the rendered riser waveform. Drag it into the FL Studio playlist.
class DragPad : public juce::Component
{
public:
    explicit DragPad (RiserProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { dragging = false; }
    void mouseDrag (const juce::MouseEvent&) override;

private:
    RiserProcessor& proc;
    bool dragging = false;
};

class RiserEditor : public juce::AudioProcessorEditor,
                    private juce::Timer
{
public:
    explicit RiserEditor (RiserProcessor&);
    ~RiserEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    RiserProcessor& proc;

    juce::TextButton captureButton { "Capture" }, loadButton { "Load File" },
                     renderButton { "Render" }, playButton { "Play" };
    juce::ToggleButton autoButton { "Auto re-render" };
    juce::Label status;
    DragPad pad;
    std::array<Knob, 10> knobs;
    std::unique_ptr<juce::FileChooser> chooser;

    int lastSeenChange = 0, lastRenderedChange = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RiserEditor)
};
