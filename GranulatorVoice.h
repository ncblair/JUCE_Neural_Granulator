#pragma once

#include <JuceHeader.h>

class GranulatorVoice : public juce::MPESynthesiserVoice {
  public: 
    //MPESynthesiserVoice Methods
    void noteStarted() override;
    void noteStopped(bool allowTailOff) override;
    void notePitchbendChanged() override;
    void notePressureChanged() override;
    void noteTimbreChanged() override;
    void noteKeyStateChanged() override;
    void prepareToPlay(double sampleRate, int samplesPerBlock, int outputChannels, AudioPluginAudioProcessor& p);
    void setCurrentSampleRate (double newRate) override;
    void renderNextBlock (juce::AudioBuffer< float > &outputBuffer, int startSample, int numSamples) override;
    bool isActive() const override;

    //Custom Methods
    void update_parameters(juce::AudioProcessorValueTreeState& apvts);
    // void replace_audio(const at::Tensor& grain); //MOVE TO PLUGIN PROCESSOR

  private:
    // Reference to Audio Processor (Which points to the current grain buffer)
    AudioPluginAudioProcessor& processorRef;

    // Audio Buffers
    juce::AudioBuffer<float> pitched_grain_buffer; // stores the current pitched note version of grain
    juce::AudioBuffer<float> internal_playback_buffer; // to apply envelopes non-destructively

    // Pitching/Interpolation
    juce::Interpolators::Lagrange interp;
    int pitched_samples;

    //Grain list
    const int N_GRAINS{100};
    std::array<Grain, N_GRAINS> grains; // Each voice has a list of grains
    int grain_index = 0.0;

    //Voice Housekeeping
    bool note_on{false};
    bool finger_down{false};

    //Envelopes
    juce::ADSR env1;

    //Grain User Parameters
    float grain_size; // percentage of grain to read in 0 to 1
    float grain_scan;

    //Granulator User Parameters
    float spray; //randomness (0-1)
    float density; //ms (could scale with note, not sure)
    int grain_env_type; //Expodec = 0, Gaussian = 1, Rexpodec = 2

    //Granulator Internal Variables
    float gain; // 0 to 1, based on velocity
};
