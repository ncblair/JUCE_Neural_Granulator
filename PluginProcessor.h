#pragma once

// #include <juce_audio_processors/juce_audio_processors.h>
#include <JuceHeader.h>
#include "utils.h"
// #include <torch/torch.h>

//==============================================================================
class AudioPluginAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    AudioPluginAudioProcessor();
    ~AudioPluginAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts; // create audio processor value tree state for parameters
    // std::unique_ptr<juce::UndoManager> undo_manager;
    juce::FileLogger logger;

    double GRAIN_SAMPLE_RATE{48000.}; // constant playback rate of nn grains

    //Replace audio grain 
    // void replace_grain(const at::Tensor& tensor);
    void replace_morph_buf(juce::AudioBuffer<float>* new_buffer);

    // // audio file1 stuff
    // std::atomic<bool> file1_loaded{false};
    // std::atomic<juce::AudioBuffer<float>*> file1_pointer{nullptr};
    // std::atomic<float> file1_sample_rate;

    SafeBuffer morph_buf;
    juce::AudioBuffer<float> temp_morph_buf;

    // Two Sound Files can be loaded in
    Soundfile sounds[2];

    //allow sample playback
    // std::atomic<bool> play_sample{false};

    
private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();


    
    // Granulator MPE
    juce::MPESynthesiser granulator;
    juce::MPEZoneLayout zone_layout;
    
    // Interpolators to go from NN sample rate to Hardware Sample Rate
    juce::Interpolators::Lagrange interpolators[2];
    juce::AudioBuffer<float> resample_buffer; //resample from grain rate to hardware rate

    // temporary/utilites
    double grain_sample_rate_ratio; //GRAIN_SAMPLE_RATE/getCurrentSampleRate();

    // float file_playback_counter{0};

    float morph_amt;
    

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessor)
};
