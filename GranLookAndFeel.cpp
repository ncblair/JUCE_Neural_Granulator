#include "GranLookAndFeel.h"

GranLookAndFeel::GranLookAndFeel() {
    setColour(juce::ResizableWindow::backgroundColourId, juce::Colours::white);
}

//https://docs.juce.com/master/tutorial_look_and_feel_customisation.html
void GranLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                        const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider&) {
    auto radius = (float) juce::jmin (width / 2, height / 2) - 4.0f;
    auto centreX = (float) x + (float) width  * 0.5f;
    auto centreY = (float) y + (float) height * 0.5f;
    auto rx = centreX - radius;
    auto ry = centreY - radius;
    auto rw = radius * 2.0f;
    auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    auto pointerLength = radius * 0.5f;
    auto pointerThickness = 2.0f;
    auto lineThickness = 1.0f;

    // fill circle
    g.setColour (juce::Colours::white);
    g.fillEllipse (rx, ry, rw, rw);
    // outline circle
    g.setColour (juce::Colours::black);
    g.drawEllipse (rx, ry, rw, rw, lineThickness);

    // pointer
    juce::Path p;
    p.addRectangle (-pointerThickness * 0.5f, -radius, pointerThickness, pointerLength);

    // p.addEllipse(-pointerLength / 2.0f, -radius * 0.7f, pointerLength, pointerLength);
    p.applyTransform (juce::AffineTransform::rotation (angle).translated (centreX, centreY));

    // draw pointer
    g.setColour (juce::Colours::black);
    g.fillPath (p);
}

void GranLookAndFeel::drawLabel(juce::Graphics& g, juce::Label& label) {
    g.fillAll (label.findColour (Label::backgroundColourId));

    auto alpha = label.isEnabled() ? 1.0f : 0.5f;
    // const Font font (getLabelFont (label));

    g.setColour (juce::Colours::black.withMultipliedAlpha(alpha));
    g.setFont (font);

    auto textArea = getLabelBorderSize (label).subtractedFrom (label.getLocalBounds());

    g.drawFittedText (label.getText(), textArea, label.getJustificationType(),
                        jmax (1, (int) ((float) textArea.getHeight() / font.getHeight())),
                        label.getMinimumHorizontalScale());
}

void GranLookAndFeel::setFontSize(float fontSize) {
    font.setHeight(fontSize);
}