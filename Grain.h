#pragma once

#include <JuceHeader.h>

#include "utils.h"

class Grain {
  public:

    /**
     * Initialization
     */
    void prepareToPlay(double sampleRate, SafeBuffer* safe_buf_ptr, 
                      juce::SmoothedValue<float>* env_w, juce::SmoothedValue<float>* env_c);

    /**
     * Called at the grain rate.
     * 
     * size: size of morph_buf used in ms
     * start: offset from start of morph_buf, percentage [0,1]
     * 
     */ 
    void noteStarted(float dur, float start);


    /**
     * Called at the audio sample rate.
     * 
     * playback_rate: how fast we'll move through the buffer
     */
    float getNextSample(float playback_rate, int c=0);

    /**
     * Return true if the grain is playing, o.w. return false.
     */
    bool isActive(int c=0);
  private:
    //Actual Source
    SafeBuffer* morph_buf_ptr{nullptr};
    float sample_rate_ms{48.0f};


    //Voice Housekeeping
    bool note_on[2]{false, false};
    float elapsed_ms[2]{0.0f, 0.0f};
    float cur_sample[2]{0.0f, 0.0f};

    //Grain Parameters
    float grain_dur; // length of grain in ms
    float grain_start; // position in buffer normalized from 0 to 1

    std::shared_ptr<TriangularRangeTukey> env;
};

