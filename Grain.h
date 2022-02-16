#pragma once
#include <JuceHeader.h>

class Grain {
  public: 
    void noteStarted(float size, float scan);
    void prepareToPlay(double sampleRate, juce::AudioBuffer<float>* grain);
    float getNextSample(int numSamplesInSourceBuffer);
    bool isActive();
  private:
    //Actual Source
    juce::AudioBuffer<float>* source{nullptr};

    // For now, we will use an ADSR window on each grain
    // TODO: Consider switching this to Expodec, Gaussian, Rexpodec. 
    //  OR: Even better, connect this to a GUI envelope. 
    juce::ADSR grain_env;
    

    //Voice Housekeeping
    bool note_on{false};
    float percent_elapsed{0.0f};
    int cur_sample{0};

    //Grain Parameters
    float grain_size; // length of grain in seconds
    float grain_scan; // position in buffer normalized from 0 to 1

    // int grain_env_type; //Expodec = 0, Gaussian = 1, Rexpodec = 2
};
