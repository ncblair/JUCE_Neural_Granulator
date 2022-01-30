#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "GranulatorVoice.h"

//==============================================================================
AudioPluginAudioProcessor::AudioPluginAudioProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ), apvts(*this, nullptr, "Parameters", createParameters()), 
                       logger(juce::File("/users/ncblair/COMPSCI/JUCE_Harmonic_Oscillator/log.txt"), "Damped Log\n")
{
    
    granulator.clearVoices();
    for (auto i = 0; i < 16; ++i) {
        granulator.addVoice(new GranulatorVoice());
    }
    granulator.setVoiceStealingEnabled(true);

    // granulator.enableLegacyMode(2);

    zone_layout.clearAllZones();
    zone_layout.setLowerZone(15, 48, 2);
    granulator.setZoneLayout(zone_layout);
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor()
{
}

//==============================================================================
const juce::String AudioPluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AudioPluginAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AudioPluginAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AudioPluginAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AudioPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AudioPluginAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int AudioPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AudioPluginAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String AudioPluginAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void AudioPluginAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void AudioPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    granulator.setCurrentPlaybackSampleRate(sampleRate);
    for (int i = 0; i < granulator.getNumVoices(); ++i) {
        if (auto voice = dynamic_cast<GranulatorVoice*>(granulator.getVoice(i))) {
            //update voice parameters from value tree
            voice->prepareToPlay(sampleRate, samplesPerBlock, getTotalNumOutputChannels());
        }
    }
}

void AudioPluginAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool AudioPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}

void AudioPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    for (int i = 0; i < granulator.getNumVoices(); ++i) {
        if (auto voice = dynamic_cast<juce::MPESynthesiserVoice*>(granulator.getVoice(i))) {
            //update voice parameters from value tree
            auto gran_voice = dynamic_cast<GranulatorVoice*>(voice);
            gran_voice->updateParameters(apvts);
        }
    }
    granulator.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
    // INTERNAL CLIPPING
    // for (int c = 0; c < totalNumOutputChannels; ++c) {
    //     juce::FloatVectorOperations::clip(buffer.getWritePointer(c), buffer.getReadPointer(c), -1.0f, 1.0f, buffer.getNumSamples());
    // }
}

//==============================================================================
bool AudioPluginAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor()
{
    return new AudioPluginAudioProcessorEditor (*this);
}

//==============================================================================
void AudioPluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::ignoreUnused (destData);
}

void AudioPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    juce::ignoreUnused (data, sizeInBytes);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}

juce::AudioProcessorValueTreeState::ParameterLayout AudioPluginAudioProcessor::createParameters() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    params.push_back(std::make_unique<juce::AudioParameterFloat>("ENV1_ATTACK", "Env1_Attack", 0.001f, 5.0f, 0.01f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("ENV1_DECAY", "Env1_Decay", 0.001f, 60.0f, 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("ENV1_SUSTAIN", "Env1_Sustain", 0.0f, 1.0f, 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("ENV1_RELEASE", "Env1_Release", 0.001f, 10.0f, 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "GRAIN_SIZE",  // parameter ID
        "Grain Size",  // parameter name
        juce::NormalisableRange<float> (0.001f, 0.5f, 0.001f),  // range
        0.5f,         // default value
        "Length of grain in seconds", // parameter label (description?)
        juce::AudioProcessorParameter::Category::genericParameter,
        [](float value, int maximumStringLength) {return juce::String (value) + " seconds";},
        [](juce::String text) {return text.trimCharactersAtEnd (" seconds").getFloatValue();}
    ));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "GRAIN_SCAN",  // parameter ID
        "Grain Scan",  // parameter name
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.0f),  // range
        0.0f,         // default value
        "Position of grain in [0, 1]", // parameter label (description?)
        juce::AudioProcessorParameter::Category::genericParameter,
        [](float value, int maximumStringLength) {return juce::String (value) + " position";},
        [](juce::String text) {return text.trimCharactersAtEnd (" position").getFloatValue();}
    ));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "SPRAY",  // parameter ID
        "Spray",  // parameter name
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.0f),  // range
        0.0f,         // default value
        "Jitter of grain in [0, 1]", // parameter label (description?)
        juce::AudioProcessorParameter::Category::genericParameter,
        [](float value, int maximumStringLength) {return juce::String (value) + " entropy";},
        [](juce::String text) {return text.trimCharactersAtEnd (" entropy").getFloatValue();}
    ));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "DENSITY",  // parameter ID
        "Density",  // parameter name
        juce::NormalisableRange<float> (2.0f, 1000.0f, 0.0f, 0.3f),  // range
        2.0f,         // default value
        "Average Rate of grain playback per voice in hz", // parameter label (description?)
        juce::AudioProcessorParameter::Category::genericParameter,
        [](float value, int maximumStringLength) {return juce::String (value) + " hz";},
        [](juce::String text) {return text.trimCharactersAtEnd (" hz").getFloatValue();}
    ));

    auto grain_env_types = juce::StringArray({"Expodec", "Gaussian", "Rexpodec"});
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "GRAIN_ENV_TYPE",  // parameter ID
        "Grain Env Type",  // parameter name
        grain_env_types, //String Array of options
        0, //default index
        "type of grain envelope (expodec, gaussian, rexpodec)", // parameter label (description?)
        [grain_env_types](int index, int maximumStringLength) {return grain_env_types[index] + " grain env type";},
        [](juce::String text) {return text.trimCharactersAtEnd (" grain env type").getFloatValue();}
    ));
    
    return {params.begin(), params.end()};
}