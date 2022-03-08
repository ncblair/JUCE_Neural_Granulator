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
    // init granulator
    granulator.clearVoices();
    for (auto i = 0; i < 16; ++i) {
        granulator.addVoice(new GranulatorVoice());
    }
    granulator.setVoiceStealingEnabled(true);

    // Disable mpe
    // granulator.enableLegacyMode(2);
    

    zone_layout.clearAllZones();
    zone_layout.setLowerZone(15, 48, 2);
    granulator.setZoneLayout(zone_layout);

    std::cout << "OUTPUT" << std::endl;
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
    grain_sample_rate_ratio = GRAIN_SAMPLE_RATE/sampleRate;
    granulator.setCurrentPlaybackSampleRate(GRAIN_SAMPLE_RATE);
    for (int i = 0; i < granulator.getNumVoices(); ++i) {
        if (auto voice = dynamic_cast<GranulatorVoice*>(granulator.getVoice(i))) {
            //update voice parameters from value tree. Use the Grain sample rate, not the hardware one
            voice->prepareToPlay(GRAIN_SAMPLE_RATE, int(ceil(double(samplesPerBlock)*grain_sample_rate_ratio)), getTotalNumOutputChannels(), this);
        }
    }

    resample_buffer = juce::AudioSampleBuffer(1, samplesPerBlock * 2); //allocate twice as many samples defensively

    interpolators[0] = juce::Interpolators::Lagrange();
    interpolators[1] = juce::Interpolators::Lagrange();

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

    // clear extra output channels if they don't all get overriden
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // // If a new grain is ready, switch the pointers atomically
    // if (new_grain_ready.load()) {
    //     auto temp_ptr = temp_buffer_ptr_atomic.load();
    //     auto morph_ptr = morph_buf_ptr_atomic.load();
    //     morph_buf_ptr_atomic.store(temp_ptr);
    //     temp_buffer_ptr_atomic.store(morph_ptr);
    //     new_grain_ready.store(false);
    // }
    morph_buf.update();
    
    //Update Parameters on Audio Thread in each voice
    for (int i = 0; i < granulator.getNumVoices(); ++i) {
        if (auto voice = dynamic_cast<juce::MPESynthesiserVoice*>(granulator.getVoice(i))) {
            //update voice parameters from value tree
            auto gran_voice = dynamic_cast<GranulatorVoice*>(voice);
            gran_voice->update_parameters(apvts);
        }
    }

    //allocate more to resample buffer if need be. ideally this won't be called
    int num_samples = int(ceil(buffer.getNumSamples() * grain_sample_rate_ratio));
    if (num_samples > resample_buffer.getNumSamples()) {
        //hope this doesn't happen (allocating in audio thread)
        resample_buffer.setSize(resample_buffer.getNumChannels(), num_samples);
    }

    // Render next block for each granulator voice on resample buffer
    resample_buffer.clear();
    granulator.renderNextBlock(resample_buffer, midiMessages, 0, buffer.getNumSamples());

    if (play_sample.load()) {
        auto mbuf = *morph_buf.load();
        auto num_samples_now = juce::jmin(float(num_samples), mbuf.getNumSamples() - file_playback_counter);
        resample_buffer.addFrom(0, 0, mbuf, 0, file_playback_counter, num_samples_now);
        file_playback_counter += num_samples_now;
        if (file_playback_counter == mbuf.getNumSamples()) {
            file_playback_counter = 0;
            play_sample.store(false);
        }
    }
   
    for (int c = 0; c < totalNumOutputChannels; ++c) {
        // resample temporary buffer samples to the sample rate of the output buffer
        interpolators[c].process   (grain_sample_rate_ratio, 
                                    resample_buffer.getReadPointer(0), 
                                    buffer.getWritePointer(c), 
                                    buffer.getNumSamples());
        // internal clipping
        juce::FloatVectorOperations::clip  (buffer.getWritePointer(c), buffer.getWritePointer(c),
                                            -1.0f, 1.0f, buffer.getNumSamples());
    }
              
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

// void AudioPluginAudioProcessor::replace_grain(const at::Tensor& tensor) {
//     // Runs on Torch Thread

//     // Wait until pointers have been swapped in audio thread
//     while (new_grain_ready.load());
    
//     auto temp_ptr = temp_buffer_ptr_atomic.load();
//     for (int channel = 0; channel < temp_ptr->getNumChannels(); ++channel)
//     {
//         temp_ptr->copyFrom(0, 0, tensor.data_ptr<float>(), temp_ptr->getNumSamples());
//     }
//     new_grain_ready.store(true);
// }

// void AudioPluginAudioProcessor::replace_morph_buf(juce::AudioBuffer<float>* new_buffer) {
//     // Runs on Editor Thread

//     // Wait until pointers have been swapped in audio thread
//     while (new_grain_ready.load());
//     temp_buffer_ptr_atomic.store(new_buffer);
//     new_grain_ready.store(true);
// }


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
        juce::NormalisableRange<float> (1.0f, 1000.0f, 1.0f),  // range
        500.0f,         // default value
        "Length of grain in ms", // parameter label (description?)
        juce::AudioProcessorParameter::Category::genericParameter,
        [](float value, int maximumStringLength) {return juce::String (value) + " ms";},
        [](juce::String text) {return text.trimCharactersAtEnd (" ms").getFloatValue();}
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
        juce::NormalisableRange<float> (1.0f, 1000.0f, 0.0f, 0.3f),  // range
        1.0f,         // default value
        "Average Rate of grain playback per voice in hz", // parameter label (description?)
        juce::AudioProcessorParameter::Category::genericParameter,
        [](float value, int maximumStringLength) {return juce::String (value) + " hz";},
        [](juce::String text) {return text.trimCharactersAtEnd (" hz").getFloatValue();}
    ));

    // auto grain_env_types = juce::StringArray({"Expodec", "Gaussian", "Rexpodec"});
    // params.push_back(std::make_unique<juce::AudioParameterChoice>(
    //     "GRAIN_ENV_TYPE",  // parameter ID
    //     "Grain Env Type",  // parameter name
    //     grain_env_types, //String Array of options
    //     0, //default index
    //     "type of grain envelope (expodec, gaussian, rexpodec)", // parameter label (description?)
    //     [grain_env_types](int index, int maximumStringLength) {return grain_env_types[index] + " grain env type";},
    //     [](juce::String text) {return text.trimCharactersAtEnd (" grain env type").getFloatValue();}
    // ));
    
    return {params.begin(), params.end()};
}

