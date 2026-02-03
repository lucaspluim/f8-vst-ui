#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include "XYControlComponent.h"
#include "NativeDialogs.h"

class MainComponent : public juce::Component,
                      private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

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

    XYControlComponent xyControl;

    bool isHoldingOutside = false;
    int64_t holdStartTime = 0;
    bool menuShown = false;
    float holdProgress = 0.0f;  // 0.0 to 1.0 for visual feedback

    juce::File presetsFolder;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
