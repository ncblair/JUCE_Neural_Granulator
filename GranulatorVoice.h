#pragma once

// #include <juce_audio_processors/juce_audio_processors.h>
#include <JuceHeader.h>

class GranulatorVoice : public juce::MPESynthesiserVoice {
  public: 
    void noteStarted() override;
    void noteStopped(bool allowTailOff) override;
    void notePitchbendChanged() override;
    void notePressureChanged() override;
    void noteTimbreChanged() override;
    void noteKeyStateChanged() override;
    void prepareToPlay(double sampleRate, int samplesPerBlock, int outputChannels);
    void setCurrentSampleRate (double newRate) override;
    void renderNextBlock (juce::AudioBuffer< float > &outputBuffer, int startSample, int numSamples) override;
    bool isActive() const override;
    void updateParameters(juce::AudioProcessorValueTreeState& apvts);
  private:

  //Voice Housekeeping
  bool note_on{false};
  bool finger_down{false};

  //Envelopes
  juce::ADSR env1;

  //Grain Parameters
  float grain_size;
  float grain_scan;

  float spray; //randomness (0-1)
  float density; //ms (could scale with note, not sure)
  int grain_env_type; //Expodec = 0, Gaussian = 1, Rexpodec = 2
};
