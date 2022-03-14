#pragma once

#include <JuceHeader.h>
#include "utils.h"

class Soundfile {
  public:
    // A soundfile is a circular buffer that has a region with current position and length
    Soundfile();

    void load_file(juce::TextButton* button, double sr);
    void update_parameters(float region_scan_arg);
    void queue_region_buffer();

    bool get_file_loaded();
    int get_num_samples();
    int get_num_channels();
    void paint(juce::Graphics& g, const juce::Rectangle<int>& thumbnailBounds);
    // juce::String file_name{"Open..."};

    // buffer and region
    SafeBuffer file_buffer;
    SafeBuffer region_buffer;
    // // allocated on background thread on file load
    juce::AudioBuffer<float> region_buf_temp;
    std::atomic<float> scan_changed;

    std::atomic<bool> file_loaded{false};

    // drawing and thumbnail
    juce::AudioThumbnailCache thumbnail_cache{5};
    juce::AudioThumbnail thumbnail{512, format_manager, thumbnail_cache};


  private:
    // file loading
    std::unique_ptr<juce::FileChooser> file_chooser;
    juce::AudioFormatManager format_manager;
    
    // CONST length of region internally
    std::atomic<float> region_num_seconds{1.0f};

    // file internal sample rate
    std::atomic<int> num_samples;
    std::atomic<int> num_channels{0};

    std::atomic<float> region_scan{-1.0f};
    std::atomic<bool> new_region{false};

    //interpolate soundfile to GRAIN_SAMPLE_RATE
    juce::Interpolators::Lagrange interpolators[2];
};