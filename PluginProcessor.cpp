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

    temp_morph_buf.setSize(1, sampleRate);

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

    // update soundfiles parameters and buffers
    sounds[0].update_parameters(apvts.getRawParameterValue("FILE_SCAN_0")->load());
    sounds[1].update_parameters(apvts.getRawParameterValue("FILE_SCAN_1")->load());
    
    auto morph_changed = morph_amt != apvts.getRawParameterValue("MORPH")->load();
    
    // Update Morph Buf
    // TODO: This goes on background thread in future 
    if (sounds[0].scan_changed.load() || sounds[1].scan_changed.load() || morph_changed) {
        // DO MORPH // SIMPLE CROSSFADE
        morph_amt = apvts.getRawParameterValue("MORPH")->load();
        temp_morph_buf.copyFrom (0, 0, *(sounds[0].region_buffer.load()), 0, 0, sounds[0].get_num_samples());
        temp_morph_buf.applyGain(1.0f - morph_amt);
        temp_morph_buf.addFrom(0, 0, *(sounds[1].region_buffer.load()), 0, 0, sounds[1].get_num_samples(), morph_amt);

        sounds[0].scan_changed.store(false);
        sounds[1].scan_changed.store(false);
    }
    morph_buf.queue_new_buffer(&temp_morph_buf);
    // morph_buf.queue_new_buffer(sounds[0].region_buffer.load());

    // update buffer
    morph_buf.update();
    

    // Update Parameters on Audio Thread in each voice
    for (int i = 0; i < granulator.getNumVoices(); ++i) {
        if (auto voice = dynamic_cast<juce::MPESynthesiserVoice*>(granulator.getVoice(i))) {
            //update voice parameters from value tree
            auto gran_voice = dynamic_cast<GranulatorVoice*>(voice);
            gran_voice->update_parameters(apvts);
        }
    }

    // allocate more to resample buffer if need be. ideally this won't be called
    int num_samples = int(ceil(buffer.getNumSamples() * grain_sample_rate_ratio));
    if (num_samples > resample_buffer.getNumSamples()) {
        //hope this doesn't happen (allocating in audio thread)
        resample_buffer.setSize(resample_buffer.getNumChannels(), num_samples);
    }

    // Render next block for each granulator voice on resample buffer
    resample_buffer.clear();
    granulator.renderNextBlock(resample_buffer, midiMessages, 0, buffer.getNumSamples());

    // if (play_sample.load()) {
    //     auto mbuf = *morph_buf.load();
    //     auto num_samples_now = juce::jmin(float(num_samples), mbuf.getNumSamples() - file_playback_counter);
    //     resample_buffer.addFrom(0, 0, mbuf, 0, file_playback_counter, num_samples_now);
    //     file_playback_counter += num_samples_now;
    //     if (file_playback_counter == mbuf.getNumSamples()) {
    //         file_playback_counter = 0;
    //         play_sample.store(false);
    //     }
    // }
    
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
        juce::NormalisableRange<float> (1.0f, 500.0f, 1.0f),  // range
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
    auto morph_range = juce::NormalisableRange<float> (0.0f, 1.0f, 0.0f, 1.0f);
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "MORPH",  // parameter ID
        "Morph",  // parameter name
        morph_range,  // range
        0.0f,         // default value
        "Morph between file1 and file 2", // parameter label (description?)
        juce::AudioProcessorParameter::Category::genericParameter,
        [morph_range](float value, int maximumStringLength) {std::stringstream ss;ss << std::fixed << std::setprecision(2) << morph_range.convertTo0to1(value);return juce::String(ss.str());},
        [morph_range](juce::String text) {return morph_range.convertFrom0to1(text.getFloatValue());}
    ));

    auto scan_range = juce::NormalisableRange<float> (0.0f, 1.0f, 0.0f, 1.0f);
    for (int i = 0; i < 2; ++i) {
        // per-file parameters
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            "FILE_SCAN_" + std::to_string(i),  // parameter ID
            "File Scan " + std::to_string(i),  // parameter name
            scan_range,  // range
            0.0f,         // default value
            "Position in file in [0, 1]", // parameter label (description?)
            juce::AudioProcessorParameter::Category::genericParameter,
            [scan_range](float value, int maximumStringLength) {std::stringstream ss;ss << std::fixed << std::setprecision(2) << scan_range.convertTo0to1(value);return juce::String(ss.str());},
            [scan_range](juce::String text) {return scan_range.convertFrom0to1(text.getFloatValue());}
        ));
    }
    
    return {params.begin(), params.end()};
}

