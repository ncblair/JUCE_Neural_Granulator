#pragma once

#include <JuceHeader.h>
#include <math.h>

/**
 * Fast Midi To Frequency Function
 */
float mtof(const float note_number);

/**
 * Fast Frequency To Midi Function
 */
float ftom(const float hertz);


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

  protected:
    float value = 0, alpha = 0.6, mSamplingRate = 48000;
    float currentS = 0, totalS = 1;
    float alpha_totalS;
};

class TriangularRangeTukey {
  public:
    TriangularRangeTukey(juce::SmoothedValue<float>* w, juce::SmoothedValue<float>* c);
    float step(float playback_rate);
    float get(float elapsed);
    void set(float seconds);

    void setSamplingRate(float samplingRate) { mSamplingRate = samplingRate; }
    float getSamplingRate() const { return mSamplingRate; }
  private:
    float mSamplingRate{48000};
    juce::SmoothedValue<float>* width; // -1 to 1
    juce::SmoothedValue<float>* center;
    float percent_elapsed{0.0f};
    float total_samples{48000};
};

class SafeBuffer {
  public:
    SafeBuffer(int channels=1, int samples=24000);
    // editor thread
    void queue_new_buffer(juce::AudioBuffer<float>* new_buffer);
    // audio thread
    void update();

    // returns a pointer to the current buffer
    juce::AudioBuffer<float>* load();

    int get_num_samples();
    int get_num_channels();

    // // Run before any buffers are queued
    // void prepareToPlay(int channels, int samples);

    // calling this class returns the cur buffer
    // const juce::AudioBuffer<float> operator()(){return *cur_buf_ptr_atomic.load()};
    
    // an atomic pointer to the current buffer
    std::atomic<juce::AudioBuffer<float>*> cur_buf_ptr_atomic;
    // an atomic pointer to the temporary buffer
    std::atomic<juce::AudioBuffer<float>*> temp_buf_ptr_atomic;
    // two grain buffers. One is always temporary, one is always active
    juce::AudioBuffer<float> buf_1;
    juce::AudioBuffer<float> buf_2;
    // whether there is a buffer queued (i.e. ready to update/swap pointers)
    std::atomic<bool> ready_to_update{false};

  private:
    std::atomic<int> num_samples{0};
    std::atomic<int> num_channels{0};
};
