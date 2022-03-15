#pragma once

#include <JuceHeader.h>

#include "Grain.h"
#include "PluginProcessor.h"
#include "utils.h"

class GranulatorVoice : public juce::MPESynthesiserVoice {
  public: 
    //MPESynthesiserVoice Methods
    void noteStarted() override;
    void noteStopped(bool allowTailOff) override;
    void notePitchbendChanged() override;
    void notePressureChanged() override;
    void noteTimbreChanged() override;
    void noteKeyStateChanged() override;
    void prepareToPlay(double sampleRate, int samplesPerBlock, int outputChannels, AudioPluginAudioProcessor* p);
    void setCurrentSampleRate (double newRate) override;
    void renderNextBlock (juce::AudioBuffer< float > &outputBuffer, int startSample, int numSamples) override;
    bool isActive() const override;
    bool trigger();

    void update_parameters(juce::AudioProcessorValueTreeState& apvts);
    // void replace_audio(const at::Tensor& grain); //MOVE TO PLUGIN PROCESSOR

  private:
    // Reference to Audio Processor (Which points to the current grain buffer)
    AudioPluginAudioProcessor* processor_ptr;

    // Audio Buffers
    // juce::AudioBuffer<float> pitched_grain_buffer; // stores the current pitched note version of grain
    juce::AudioBuffer<float> internal_playback_buffer; // to apply envelopes non-destructively

    // Pitching/Interpolation
    // juce::Interpolators::Lagrange interp;
    // int pitched_samples; // number of samples in pitched grain
    float playback_rate{1.0f}; //TODO: Make this a smoothed value?

    //Grain list
    const int N_GRAINS{100};
    std::vector<Grain> grains; // Each voice has a list of grains
    int grain_index = 0.0;

    //Voice Housekeeping
    bool note_on{false};
    bool finger_down{false};

    //Envelopes
    juce::ADSR env1;

    //Grain User Parameters
    float grain_dur; // length of grain in ms
    float grain_start; // percentage of grain to start at in (0, 1)

    //Granulator User Parameters
    float jitter; //randomness (0-1)
    float grain_rate; //ms (could scale with note, not sure)

    //Granulator Internal Variables
    float gain; // 0 to 1, based on velocity

    //Grain Scheduler
    int trigger_helper{0};
    float spray_factor{0.0f};
    juce::Random random{1111}; // seed 1111

    // WRITE POINTERS
    float* write_pointers[2];


    // Grain Envelope Shape Parameters
    juce::SmoothedValue<float> grain_env_width{0.6f}; // -1 to 1
    juce::SmoothedValue<float> grain_env_center{0.5f}; // 0 to 1
    
};
