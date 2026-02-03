#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "XYControlComponent.h"
#include "NativeDialogs.h"

class XYControlAudioProcessorEditor : public juce::AudioProcessorEditor,
                                       private juce::Timer
{
public:
    XYControlAudioProcessorEditor(XYControlAudioProcessor&);
    ~XYControlAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;

private:
    void timerCallback() override;
    void showPresetOptions();
    void savePresetToFile(const juce::File& file);
    void loadPresetFromFile(const juce::File& file);
    void updateParametersFromXY();

    XYControlAudioProcessor& audioProcessor;
    XYControlComponent xyControl;

    bool isHoldingOutside = false;
    int64_t holdStartTime = 0;
    bool menuShown = false;
    float holdProgress = 0.0f;

    juce::File presetsFolder;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(XYControlAudioProcessorEditor)
};
