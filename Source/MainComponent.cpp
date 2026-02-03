#include "MainComponent.h"

MainComponent::MainComponent()
{
    setSize(700, 700);
    addAndMakeVisible(xyControl);

    presetsFolder = NativeDialogs::getPresetsFolder();
}

MainComponent::~MainComponent()
{
}

void MainComponent::paint(juce::Graphics& g)
{
    // Fill background
    g.fillAll(juce::Colour(0xfff5f5f7));

    // Draw drop shadow for XY control with appropriate color for preset
    auto controlBounds = xyControl.getBounds();
    juce::Path shadowPath;
    // Use slightly larger corner radius for shadow to avoid pointed edges
    shadowPath.addRoundedRectangle(controlBounds.toFloat(), 26.0f);

    // Use lighter shadow for dark presets, darker shadow for light presets
    auto preset = xyControl.getCurrentPreset();
    juce::Colour shadowColor;

    if (preset == XYControlComponent::Preset::Blue)
        shadowColor = juce::Colour(0x14000000);  // Dark shadow for white background
    else if (preset == XYControlComponent::Preset::Red)
        shadowColor = juce::Colour(0x30000000);  // Darker shadow for red
    else // Black
        shadowColor = juce::Colour(0x40000000);  // Even darker shadow for black

    // Use slightly larger radius for smoother corners
    juce::DropShadow shadow(shadowColor, 18, juce::Point<int>(0, 4));
    shadow.drawForPath(g, shadowPath);

    // Draw blue progress outline during hold
    if (holdProgress > 0.0f)
    {
        // Blue outline color with opacity based on progress
        g.setColour(juce::Colour(0xff007aff).withAlpha(0.3f + holdProgress * 0.7f));

        // Stroke width grows with progress
        float strokeWidth = 2.0f + holdProgress * 6.0f;

        // Expansion grows with progress - starts at the edge and expands outward
        float expansion = holdProgress * 12.0f;

        // Draw rounded rectangle outline
        g.drawRoundedRectangle(controlBounds.toFloat().expanded(expansion),
                              24.0f + expansion * 0.5f,
                              strokeWidth);
    }
}

void MainComponent::resized()
{
    auto bounds = getLocalBounds();

    // Make it 500x500 like the HTML version
    xyControl.setBounds(bounds.withSizeKeepingCentre(500, 500));
}

void MainComponent::mouseDown(const juce::MouseEvent& event)
{
    // Check if click is outside the XY control area
    if (!xyControl.getBounds().contains(event.getPosition()))
    {
        isHoldingOutside = true;
        holdStartTime = juce::Time::currentTimeMillis();
        holdProgress = 0.0f;
        menuShown = false;
        startTimer(16);  // ~60fps for smooth animation
    }
}

void MainComponent::mouseUp(const juce::MouseEvent& event)
{
    isHoldingOutside = false;
    holdProgress = 0.0f;
    stopTimer();
    repaint();
}

void MainComponent::mouseDoubleClick(const juce::MouseEvent& event)
{
    // Check if double-click is outside the XY control area
    if (!xyControl.getBounds().contains(event.getPosition()))
    {
        // Cycle to next preset
        auto currentPreset = static_cast<int>(xyControl.getCurrentPreset());
        currentPreset = (currentPreset + 1) % 3;  // 0->1->2->0
        xyControl.setPreset(static_cast<XYControlComponent::Preset>(currentPreset));
    }
}

void MainComponent::timerCallback()
{
    if (isHoldingOutside && !menuShown)
    {
        int64_t currentTime = juce::Time::currentTimeMillis();
        int64_t holdDuration = currentTime - holdStartTime;

        // Update hold progress for visual feedback (0.0 to 1.0)
        holdProgress = juce::jmin(1.0f, holdDuration / 3000.0f);
        repaint();

        if (holdDuration >= 3000)  // 3 seconds
        {
            menuShown = true;
            holdProgress = 0.0f;
            stopTimer();
            repaint();
            showPresetOptions();
        }
    }
}

void MainComponent::showPresetOptions()
{
    // Show native macOS menu
    NativeDialogs::showPresetMenu([this](int result)
    {
        if (result == 1)
        {
            // Save preset
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
            // Load preset
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

void MainComponent::savePresetToFile(const juce::File& file)
{
    // Get current XY position (normalized 0-1)
    auto position = xyControl.getPosition();

    // Get current preset
    int presetIndex = static_cast<int>(xyControl.getCurrentPreset());

    // Create JSON object
    juce::var presetData(new juce::DynamicObject());
    auto* obj = presetData.getDynamicObject();
    obj->setProperty("x", position.x);
    obj->setProperty("y", position.y);
    obj->setProperty("preset", presetIndex);

    // Write to file
    juce::String jsonString = juce::JSON::toString(presetData, true);
    file.replaceWithText(jsonString);

    // Show confirmation
    NativeDialogs::showConfirmation("Preset Saved",
        "Preset saved to " + file.getFileName(), [](){});
}

void MainComponent::loadPresetFromFile(const juce::File& file)
{
    // Read file
    juce::String jsonString = file.loadFileAsString();

    // Parse JSON
    juce::var presetData = juce::JSON::parse(jsonString);

    if (presetData.isObject())
    {
        auto* obj = presetData.getDynamicObject();

        float x = obj->getProperty("x");
        float y = obj->getProperty("y");
        int presetIndex = obj->getProperty("preset");

        // Apply preset
        xyControl.setPreset(static_cast<XYControlComponent::Preset>(presetIndex));
        xyControl.setPosition(x, y);

        // Show confirmation
        NativeDialogs::showConfirmation("Preset Loaded",
            "Loaded preset from " + file.getFileName(), [](){});
    }
}
