#pragma once

#include <JuceHeader.h>

class GranLookAndFeel : public juce::LookAndFeel_V4 {
  private: 
    juce::Font font{juce::Typeface::createSystemTypefaceFor(BinaryData::AndaleMono_ttf,
                                                            BinaryData::AndaleMono_ttfSize)};
  public:
    GranLookAndFeel();

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                           const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider&) override;

    void drawLabel (juce::Graphics& g, juce::Label& label) override;

    void setFontSize(float fontSize);
};