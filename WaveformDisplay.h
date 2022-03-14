#pragma once

#include <JuceHeader.h>
#include "utils.h"

class WaveformDisplay : public juce::Component {
  public:
    WaveformDisplay(SafeBuffer* buffer);
    void paint (juce::Graphics& g) override;

    SafeBuffer* display_buffer{nullptr};
    // std::vector<float> points;
};