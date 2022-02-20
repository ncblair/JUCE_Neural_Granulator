#pragma once
#include <JuceHeader.h>

/**
 * Envelope generator for creating a tukey envelope.
 * A tukey window is like a fatter Hann envelope.
 */
class tukey {
  public:
    tukey() {}
    /**
    * @brief Generate tukey envelope in real-time.
    *
    * @return Amplitude value at a point in time.
    */
    float operator()(float playback_rate);

    /**
    * @brief Getter and setter for sampling rate of envelope.
    */
    void setSamplingRate(float samplingRate) { mSamplingRate = samplingRate; }
    float getSamplingRate() const { return mSamplingRate; }

    /**
    * Used to reset values to original position.
    */
    void set();

    /**
    * Set parameters of envelope.
    *
    * @param[in] The duration of the envelope in seconds.
    * @param[in] FROM 0 to 1. A coefficient that determines the ratio of the
    * sustain to the attack and release. 0 being a rectangular envelope and 1
    * being a Hann envelope.
    *  https://www.mathworks.com/help/signal/ref/tukeywin.html
    */

    void set(float seconds, float alpha);
    void set(float seconds);

    /**
    * @brief Check if the line function is complete.
    *
    * @param[in] Return true if reached target, false otherwise.
    */
    bool done() const { return totalS == currentS; }

    /**
    * @brief Increment in the time axis/
    */
    void increment() { currentS++; }

  private:
    float value = 0, alpha = 0.6, mSamplingRate = 48000;
    float currentS = 0, totalS = 1;
    float alpha_totalS;
};


class Grain {
  public:

    /**
     * Initialization
     */
    void prepareToPlay(double sampleRate, std::atomic<juce::AudioBuffer<float>*>* buf_atomic_ptr_arg);

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
    std::atomic<juce::AudioBuffer<float>*>* buf_atomic_ptr{nullptr};
    float sample_rate_ms{48};


    //Voice Housekeeping
    bool note_on{false};
    float elapsed_ms{0.0f};
    float cur_sample{0.0f};

    //Grain Parameters
    float grain_size; // length of grain in seconds
    float grain_start; // position in buffer normalized from 0 to 1

    tukey env;
};

