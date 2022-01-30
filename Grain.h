#pragma once
#include <JuceHeader.h>

class Grain {
  public: 
    void noteStarted() override;
    void noteStopped(bool allowTailOff) override;
    void notePitchbendChanged() override;
    void prepareToPlay(double sampleRate, int samplesPerBlock, int outputChannels);
    void setCurrentSampleRate (double newRate) override;
    void renderNextBlock (juce::AudioBuffer< float > &outputBuffer, int startSample, int numSamples) override;
    bool isActive() const override;
    void updateParameters(juce::AudioProcessorValueTreeState& apvts);
  private:
    //Actual Source
    std::shared_ptr<juce::AudioBuffer<float>> source{nullptr};

    //Voice Housekeeping
    bool note_on{false};

    //Grain Parameters
    float grain_size;
    float grain_scan;

    //Granulator Parameters
    float spray; //randomness (0-1)
    float density; //ms (could scale with note, not sure)
    int grain_env_type; //Expodec = 0, Gaussian = 1, Rexpodec = 2

};
