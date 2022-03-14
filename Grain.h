#pragma once

#include <JuceHeader.h>

#include "utils.h"

class Grain {
  public:

    /**
     * Initialization
     */
    void prepareToPlay(double sampleRate, SafeBuffer* safe_buf_ptr);

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
    float getNextSample(float playback_rate, int c=0);

    /**
     * Return true if the grain is playing, o.w. return false.
     */
    bool isActive();
  private:
    //Actual Source
    SafeBuffer* morph_buf_ptr{nullptr};
    float sample_rate_ms{48};


    //Voice Housekeeping
    bool note_on{false};
    float elapsed_ms[2]{0.0f, 0.0f};
    float cur_sample[2]{0.0f, 0.0f};

    //Grain Parameters
    float grain_size; // length of grain in seconds
    float grain_start; // position in buffer normalized from 0 to 1

    tukey env;
};

