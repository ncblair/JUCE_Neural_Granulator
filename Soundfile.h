#pragma once

#include <JuceHeader.h>
#include "utils.h"

class Soundfile {
  public:
    // A soundfile is a circular buffer that has a region with current position and length
    Soundfile();

    void load_file(juce::TextButton* button=nullptr);
    void update_parameters(float region_scan_arg);
    void queue_region_buffer();

    bool get_file_loaded();
    float get_num_samples();
    // juce::String file_name{"Open..."};
    std::atomic<bool> file_loaded{false};

    // buffer and region
    SafeBuffer file_buffer;
    SafeBuffer region_buffer;
    // // allocated on background thread on file load
    juce::AudioBuffer<float> region_buf_temp;
    std::atomic<float> scan_changed;

  private:
    // file loading
    std::unique_ptr<juce::FileChooser> file_chooser;
    juce::AudioFormatManager format_manager;
    
    // CONST length of region internally
    std::atomic<float> region_num_seconds{1.0f};

    // file internal sample rate
    std::atomic<double> sample_rate;

    std::atomic<float> region_scan{-1.0f};
    std::atomic<bool> new_region{false};
};