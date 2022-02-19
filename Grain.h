#pragma once
#include <JuceHeader.h>

class Grain {
  public:

    /**
     * Initialization
     */
    void prepareToPlay(double sampleRate, std::atomic<juce::AudioBuffer<float>*> buf_atomic);

    /**
     * Called at the grain rate.
     * 
     * size: size of morph_buf used in ms
     * start: offset from start of morph_buf, percentage [0,1]
     * 
     */ 
    void noteStarted(float size, float start);


    /**
     * Called at the audio sample rate.
     * 
     * playback_rate: how fast we'll move through the buffer
     */
    float getNextSample(float playback_rate);

    /**
     * Return true if the grain is playing, o.w. return false.
     */
    bool isActive();
  private:
    //Actual Source
    std::atomic<juce::AudioBuffer<float>*> buf_atomic{nullptr};
    float sample_rate_ms{48};


    //Voice Housekeeping
    bool note_on{false};
    float elapsed_ms{0.0f};
    float cur_sample{0.0f};

    //Grain Parameters
    float grain_size; // length of grain in seconds
    float grain_start; // position in buffer normalized from 0 to 1

    // int grain_env_type; //Expodec = 0, Gaussian = 1, Rexpodec = 2
};
