#include "PluginProcessor.h"
#include "PluginEditor.h"

XYControlAudioProcessorEditor::XYControlAudioProcessorEditor(XYControlAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(700, 700);
    addAndMakeVisible(xyControl);

    presetsFolder = NativeDialogs::getPresetsFolder();

    // Set initial position from parameters
    xyControl.setPosition(*audioProcessor.xParam, *audioProcessor.yParam);
    xyControl.setPreset(static_cast<XYControlComponent::Preset>((int)*audioProcessor.presetParam));

    startTimerHz(30);  // Update parameters regularly
}

XYControlAudioProcessorEditor::~XYControlAudioProcessorEditor()
{
}

void XYControlAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xfff5f5f7));

    auto controlBounds = xyControl.getBounds();
    juce::Path shadowPath;
    shadowPath.addRoundedRectangle(controlBounds.toFloat(), 26.0f);

    auto preset = xyControl.getCurrentPreset();
    juce::Colour shadowColor;

    if (preset == XYControlComponent::Preset::Blue)
        shadowColor = juce::Colour(0x14000000);
    else if (preset == XYControlComponent::Preset::Red)
        shadowColor = juce::Colour(0x30000000);
    else
        shadowColor = juce::Colour(0x40000000);

    juce::DropShadow shadow(shadowColor, 18, juce::Point<int>(0, 4));
    shadow.drawForPath(g, shadowPath);

    // Draw blue progress outline during hold
    if (holdProgress > 0.0f)
    {
        g.setColour(juce::Colour(0xff007aff).withAlpha(0.3f + holdProgress * 0.7f));
        float strokeWidth = 2.0f + holdProgress * 6.0f;
        float expansion = holdProgress * 12.0f;
        g.drawRoundedRectangle(controlBounds.toFloat().expanded(expansion),
                              24.0f + expansion * 0.5f,
                              strokeWidth);
    }
}

void XYControlAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    xyControl.setBounds(bounds.withSizeKeepingCentre(500, 500));
}

void XYControlAudioProcessorEditor::mouseDown(const juce::MouseEvent& event)
{
    if (!xyControl.getBounds().contains(event.getPosition()))
    {
        isHoldingOutside = true;
        holdStartTime = juce::Time::currentTimeMillis();
        holdProgress = 0.0f;
        menuShown = false;
        startTimer(16);
    }
}

void XYControlAudioProcessorEditor::mouseUp(const juce::MouseEvent& event)
{
    isHoldingOutside = false;
    holdProgress = 0.0f;
    stopTimer();
    repaint();
}

void XYControlAudioProcessorEditor::mouseDoubleClick(const juce::MouseEvent& event)
{
    if (!xyControl.getBounds().contains(event.getPosition()))
    {
        auto currentPreset = static_cast<int>(xyControl.getCurrentPreset());
        currentPreset = (currentPreset + 1) % 3;
        xyControl.setPreset(static_cast<XYControlComponent::Preset>(currentPreset));
        *audioProcessor.presetParam = currentPreset;
    }
}

void XYControlAudioProcessorEditor::timerCallback()
{
    // Update parameters from XY control
    updateParametersFromXY();

    // Handle hold progress
    if (isHoldingOutside && !menuShown)
    {
        int64_t currentTime = juce::Time::currentTimeMillis();
        int64_t holdDuration = currentTime - holdStartTime;

        holdProgress = juce::jmin(1.0f, holdDuration / 3000.0f);
        repaint();

        if (holdDuration >= 3000)
        {
            menuShown = true;
            holdProgress = 0.0f;
            stopTimer();
            repaint();
            showPresetOptions();
        }
    }
}

void XYControlAudioProcessorEditor::updateParametersFromXY()
{
    auto position = xyControl.getPosition();
    *audioProcessor.xParam = position.x;
    *audioProcessor.yParam = position.y;
    *audioProcessor.presetParam = static_cast<int>(xyControl.getCurrentPreset());
}

void XYControlAudioProcessorEditor::showPresetOptions()
{
    NativeDialogs::showPresetMenu([this](int result)
    {
        if (result == 1)
        {
            NativeDialogs::showSaveDialog(presetsFolder, [this](juce::File file)
            {
                if (file != juce::File())
                {
                    savePresetToFile(file);
                }
            });
        }
        else if (result == 2)
        {
            NativeDialogs::showPresetBrowser(presetsFolder, [this](juce::File file)
            {
                if (file != juce::File())
                {
                    loadPresetFromFile(file);
                }
            });
        }

        isHoldingOutside = false;
    });
}

void XYControlAudioProcessorEditor::savePresetToFile(const juce::File& file)
{
    auto position = xyControl.getPosition();
    int presetIndex = static_cast<int>(xyControl.getCurrentPreset());

    juce::var presetData(new juce::DynamicObject());
    auto* obj = presetData.getDynamicObject();
    obj->setProperty("x", position.x);
    obj->setProperty("y", position.y);
    obj->setProperty("preset", presetIndex);

    juce::String jsonString = juce::JSON::toString(presetData, true);
    file.replaceWithText(jsonString);

    NativeDialogs::showConfirmation("Preset Saved",
        "Preset saved to " + file.getFileName(), [](){});
}

void XYControlAudioProcessorEditor::loadPresetFromFile(const juce::File& file)
{
    juce::String jsonString = file.loadFileAsString();
    juce::var presetData = juce::JSON::parse(jsonString);

    if (presetData.isObject())
    {
        auto* obj = presetData.getDynamicObject();

        float x = obj->getProperty("x");
        float y = obj->getProperty("y");
        int presetIndex = obj->getProperty("preset");

        xyControl.setPreset(static_cast<XYControlComponent::Preset>(presetIndex));
        xyControl.setPosition(x, y);

        *audioProcessor.xParam = x;
        *audioProcessor.yParam = y;
        *audioProcessor.presetParam = presetIndex;

        NativeDialogs::showConfirmation("Preset Loaded",
            "Loaded preset from " + file.getFileName(), [](){});
    }
}
