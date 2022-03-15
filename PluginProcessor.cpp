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

    // SET UP FILTER CALLBACKS
    filter_cutoff.attachToParameter(apvts.getParameter("FILTER_CUTOFF"));
    filter_cutoff.onParameterChanged = [&] { update_filter_cutoff(); };  
    filter_q.attachToParameter(apvts.getParameter("FILTER_Q"));
    filter_q.onParameterChanged = [&] { update_filter_resonance(); };
    filter_type.attachToParameter(apvts.getParameter("FILTER_TYPE"));
    filter_type.onParameterChanged = [&] { update_filter_type(); };
    filter_on_attach.attachToParameter(apvts.getParameter("FILTER_ON"));
    filter_on_attach.onParameterChanged = [&] {update_filter_on();};
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
    // grain_sample_rate_ratio = GRAIN_SAMPLE_RATE/sampleRate;
    granulator.setCurrentPlaybackSampleRate(sampleRate);
    for (int i = 0; i < granulator.getNumVoices(); ++i) {
        if (auto voice = dynamic_cast<GranulatorVoice*>(granulator.getVoice(i))) {
            //update voice parameters from value tree. Use the Grain sample rate, not the hardware one
            // voice->prepareToPlay(sampleRate, int(ceil(double(samplesPerBlock)*grain_sample_rate_ratio)), getTotalNumOutputChannels(), this);
            voice->prepareToPlay(sampleRate, samplesPerBlock, getTotalNumOutputChannels(), this);
        }
    }

    // resample_buffer = juce::AudioSampleBuffer(1, samplesPerBlock * 2); //allocate twice as many samples defensively

    // interpolators[0] = juce::Interpolators::Lagrange();
    // interpolators[1] = juce::Interpolators::Lagrange();

    temp_morph_buf.setSize(getTotalNumOutputChannels(), sampleRate);

    filters[0].reset();
    filters[1].reset();

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

    update_morph_buf_and_soundfiles();
    // update_filter();

    // // TODO: FOR EVERY MAPPABLE PARAMETER, UPDATE STATE
    // for (int i = 0; i < mappable.size; ++i) {
    //     ma[i].setTarget(apvts);
    // }

    // Update Parameters on Audio Thread in each voice
    for (int i = 0; i < granulator.getNumVoices(); ++i) {
        if (auto voice = dynamic_cast<juce::MPESynthesiserVoice*>(granulator.getVoice(i))) {
            //update voice parameters from value tree
            auto gran_voice = dynamic_cast<GranulatorVoice*>(voice);
            gran_voice->update_parameters(apvts);
        }
    }


    buffer.clear();

    // don't start rendering till we've loaded a sound in. 
    if (sounds[0].file_loaded.load() || sounds[1].file_loaded.load()) {
        // RENDER NEXT AUDIO BLOCK
        granulator.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
    }

    auto block = juce::dsp::AudioBlock<float>(buffer);
    for (int c = 0; c < juce::jmin(totalNumOutputChannels, 2); ++c) {
        // FILTER
        if (filter_on) {
            auto channel_block = block.getSingleChannelBlock(c);
            filters[c].process(juce::dsp::ProcessContextReplacing<float>(channel_block));
        }

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

void AudioPluginAudioProcessor::update_morph_buf_and_soundfiles() {
    // update soundfiles parameters and buffers
    sounds[0].update_parameters(apvts.getRawParameterValue("FILE_SCAN_0")->load());
    sounds[1].update_parameters(apvts.getRawParameterValue("FILE_SCAN_1")->load());
    
    auto morph_changed = morph_amt != apvts.getRawParameterValue("MORPH")->load();
    
    // Update Morph Buf
    // TODO: This goes on background thread in future 
    if (sounds[0].scan_changed.load() || sounds[1].scan_changed.load() || morph_changed) {
        temp_morph_buf.clear();
        // DO MORPH // SIMPLE CROSSFADE
        morph_amt = apvts.getRawParameterValue("MORPH")->load();

        if (sounds[0].file_loaded.load()) {
            // add the correct number of channels from the first sample
            for (int c = 0; c < temp_morph_buf.getNumChannels(); ++c) {
                if (c < sounds[0].get_num_channels()) {
                    temp_morph_buf.copyFrom (c, 0, *(sounds[0].region_buffer.load()), c, 0, sounds[0].get_num_samples());
                }
                else {
                    temp_morph_buf.copyFrom(c, 0, temp_morph_buf, 0, 0, temp_morph_buf.getNumSamples());
                }
            }
            temp_morph_buf.applyGain(1.0f - morph_amt);
        }
        if (sounds[1].file_loaded.load()) {
            // add the correct number of channels from the second sample / if more channels in output than input, copy
            for (int c = 0; c < temp_morph_buf.getNumChannels(); ++c) {
                if (c < sounds[1].get_num_channels()) {
                    temp_morph_buf.addFrom(c, 0, *(sounds[1].region_buffer.load()), c, 0, sounds[1].get_num_samples(), morph_amt);
                }
                else {
                    temp_morph_buf.addFrom(c, 0, *(sounds[1].region_buffer.load()), 0, 0, sounds[1].get_num_samples(), morph_amt);
                }
            }
        }
        
        sounds[0].scan_changed.store(false);
        sounds[1].scan_changed.store(false);
        morph_buf.queue_new_buffer(&temp_morph_buf);
    }

    // update buffer
    morph_buf.update();
}

void AudioPluginAudioProcessor::update_filter_type() {
    switch (int(filter_type.getValue())){
        case LOWPASS:
            filters[0].setType(juce::dsp::StateVariableTPTFilterType::lowpass);
            filters[1].setType(juce::dsp::StateVariableTPTFilterType::lowpass);
            break;
        case HIGHPASS:
            filters[0].setType(juce::dsp::StateVariableTPTFilterType::highpass);
            filters[1].setType(juce::dsp::StateVariableTPTFilterType::highpass);
            break;
        case BANDPASS:
            filters[0].setType(juce::dsp::StateVariableTPTFilterType::bandpass);
            filters[1].setType(juce::dsp::StateVariableTPTFilterType::bandpass);
            break;
    }
}

void AudioPluginAudioProcessor::update_filter_cutoff() {
    auto cut = filter_cutoff.getValue();
    filters[0].setCutoffFrequency (cut);
    filters[1].setCutoffFrequency (cut);
}
void AudioPluginAudioProcessor::update_filter_resonance() {
    auto q = filter_q.getValue();
    filters[0].setResonance (q);
    filters[1].setResonance (q);
}

void AudioPluginAudioProcessor::update_filter_on() {
    filter_on = filter_on_attach.getValue();
    filters[0].reset();
    filters[1].reset();
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
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "ENV1_DECAY",  // parameter ID
        "Env1_Decay",  // parameter name
        juce::NormalisableRange<float> (0.001f, 60.0f, 0.0f, 0.3f),  // range
        1.0f,         // default value
        "Decay of grain", // parameter label (description?)
        juce::AudioProcessorParameter::Category::genericParameter
    ));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("ENV1_DECAY", "Env1_Decay", 0.001f, 60.0f, 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("ENV1_SUSTAIN", "Env1_Sustain", 0.0f, 1.0f, 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("ENV1_RELEASE", "Env1_Release", 0.001f, 10.0f, 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "GRAIN_DURATION",  // parameter ID
        "Grain Duration",  // parameter name
        juce::NormalisableRange<float> (1.0f, 500.0f, 1.0f),  // range
        500.0f,         // default value
        "Length of grain in ms", // parameter label (description?)
        juce::AudioProcessorParameter::Category::genericParameter
    ));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "GRAIN_SCAN",  // parameter ID
        "Grain Scan",  // parameter name
        juce::NormalisableRange<float> (0.0f, 0.5f, 0.0f),  // range
        0.0f,         // default value
        "Position of grain in [0, 0.5]", // parameter label (description?)
        juce::AudioProcessorParameter::Category::genericParameter
    ));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "JITTER",  // parameter ID
        "Jitter",  // parameter name
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.0f),  // range
        0.0f,         // default value
        "Jitter of grain in [0, 1]", // parameter label (description?)
        juce::AudioProcessorParameter::Category::genericParameter
    ));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "GRAIN_RATE",  // parameter ID
        "Grain Rate",  // parameter name
        juce::NormalisableRange<float> (1.0f, 1000.0f, 0.0f, 0.3f),  // range
        1.0f,         // default value
        "Average Rate of grain playback per voice in hz", // parameter label (description?)
        juce::AudioProcessorParameter::Category::genericParameter
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

    params.push_back(std::make_unique<juce::AudioParameterFloat>("GRAIN_ENV_WIDTH", "Grain_Env_Width", -1.0f, 1.0f, 0.6f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("GRAIN_ENV_CENTER", "Grain_Env_Center", 0.0f, 1.0f, 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "FILTER_CUTOFF",  // parameter ID
        "Cutoff",  // parameter name
        juce::NormalisableRange<float> (20.0f, 20000.0f, 0.0f, 0.2f),  // range
        4000.0f,         // default value
        "Filter Cutoff Frequency", // parameter label (description?)
        juce::AudioProcessorParameter::Category::genericParameter
    ));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "FILTER_Q",  // parameter ID
        "Resonance",  // parameter name
        juce::NormalisableRange<float> (0.1f, 2.0f, 0.0f, 1.0f),  // range
        0.71f,         // default value
        "Filter Q / Resonance", // parameter label (description?)
        juce::AudioProcessorParameter::Category::genericParameter
    ));
    params.push_back(std::make_unique<juce::AudioParameterInt>(
        "FILTER_TYPE",  // parameter ID
        "Filter Type",  // parameter name
        0,
        2,
        0
    ));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "FILTER_ON",  // parameter ID
        "Filter On",  // parameter name
        false // default off
    ));
    
    return {params.begin(), params.end()};
}

