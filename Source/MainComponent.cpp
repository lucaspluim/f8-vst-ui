#include "MainComponent.h"

MainComponent::MainComponent()
{
    setSize(368, 368);  // 30% smaller than previous
    addAndMakeVisible(xyControl);

    presetsFolder = NativeDialogs::getPresetsFolder();
}

MainComponent::~MainComponent()
{
}

void MainComponent::paint(juce::Graphics& g)
{
    auto preset = xyControl.getCurrentPreset();

    // Fill background with color matching the preset
    juce::Colour backgroundColor;
    juce::Colour borderColor;

    if (preset == XYControlComponent::Preset::Blue)
    {
        backgroundColor = juce::Colours::white;
        borderColor = juce::Colours::black;
    }
    else if (preset == XYControlComponent::Preset::Red)
    {
        backgroundColor = juce::Colour(0xFFFF0000);  // Red
        borderColor = juce::Colours::black;
    }
    else // Black
    {
        backgroundColor = juce::Colour(0xFF0A0A0A);  // Very dark gray instead of pure black
        borderColor = juce::Colours::white;
    }

    g.fillAll(backgroundColor);

    // Subtle drop shadow for depth (Apple-style)
    auto controlBounds = xyControl.getBounds().toFloat();
    float cornerRadius = 24.0f;

    juce::Path shadowPath;
    shadowPath.addRoundedRectangle(controlBounds, cornerRadius);

    // Use appropriate shadow based on preset - all use dark shadows for uniformity
    juce::Colour shadowColor;
    if (preset == XYControlComponent::Preset::Blue)
        shadowColor = juce::Colour(0x14000000);  // Subtle dark on white
    else if (preset == XYControlComponent::Preset::Red)
        shadowColor = juce::Colour(0x30000000);  // Darker on red
    else // Black
        shadowColor = juce::Colour(0x40000000);  // Dark shadow on very dark gray (barely visible but maintains uniformity)

    juce::DropShadow shadow(shadowColor, 18, juce::Point<int>(0, 4));
    shadow.drawForPath(g, shadowPath);

    // Draw blue progress ring during hold (stays glued to border)
    // Only show after brief delay to avoid flashing on quick double-clicks
    if (holdProgress > 0.07f)  // ~200ms delay before becoming visible
    {
        // Adjust progress to start from 0 after the delay
        float adjustedProgress = (holdProgress - 0.07f) / 0.93f;

        // Blue ring color with opacity based on progress
        g.setColour(juce::Colour(0xff007aff).withAlpha(0.3f + adjustedProgress * 0.7f));

        // Stroke width grows with progress - ring gets thicker but stays attached
        float strokeWidth = 2.0f + adjustedProgress * 10.0f;

        // Draw ring that stays glued to the XY border (no expansion)
        g.drawRoundedRectangle(controlBounds, cornerRadius, strokeWidth);
    }
}

void MainComponent::resized()
{
    auto bounds = getLocalBounds();

    // Add padding (26px on each side) for clickable area
    auto xySize = bounds.getWidth() - 52;  // 26px padding on each side
    xyControl.setBounds(bounds.withSizeKeepingCentre(xySize, xySize));
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
