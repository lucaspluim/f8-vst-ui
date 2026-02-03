#include "PluginProcessor.h"
#include "PluginEditor.h"

XYControlAudioProcessor::XYControlAudioProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    addParameter(xParam = new juce::AudioParameterFloat("x", "X Position", 0.0f, 1.0f, 0.5f));
    addParameter(yParam = new juce::AudioParameterFloat("y", "Y Position", 0.0f, 1.0f, 0.5f));
    addParameter(presetParam = new juce::AudioParameterInt("preset", "Preset", 0, 2, 0));
}

XYControlAudioProcessor::~XYControlAudioProcessor()
{
}

const juce::String XYControlAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool XYControlAudioProcessor::acceptsMidi() const
{
    return false;
}

bool XYControlAudioProcessor::producesMidi() const
{
    return false;
}

bool XYControlAudioProcessor::isMidiEffect() const
{
    return false;
}

double XYControlAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int XYControlAudioProcessor::getNumPrograms()
{
    return 1;
}

int XYControlAudioProcessor::getCurrentProgram()
{
    return 0;
}

void XYControlAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String XYControlAudioProcessor::getProgramName(int index)
{
    return {};
}

void XYControlAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
}

void XYControlAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
}

void XYControlAudioProcessor::releaseResources()
{
}

bool XYControlAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void XYControlAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Pass-through audio (no processing)
    // The XY control is just for UI/parameter control
}

bool XYControlAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* XYControlAudioProcessor::createEditor()
{
    return new XYControlAudioProcessorEditor(*this);
}

void XYControlAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // Save state
    juce::MemoryOutputStream stream(destData, true);
    stream.writeFloat(*xParam);
    stream.writeFloat(*yParam);
    stream.writeInt(*presetParam);
}

void XYControlAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // Load state
    juce::MemoryInputStream stream(data, static_cast<size_t>(sizeInBytes), false);
    *xParam = stream.readFloat();
    *yParam = stream.readFloat();
    *presetParam = stream.readInt();
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new XYControlAudioProcessor();
}
