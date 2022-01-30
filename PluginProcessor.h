#pragma once

// #include <juce_audio_processors/juce_audio_processors.h>
#include <JuceHeader.h>

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

    GRAIN_SAMPLE_RATE{48000.}; // constant playback rate of nn grains

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

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessor)
};
