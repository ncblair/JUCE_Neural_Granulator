#include "WaveformDisplay.h"

WaveformDisplay::WaveformDisplay(SafeBuffer* buffer) {
    display_buffer = buffer;
}

void WaveformDisplay::paint(juce::Graphics& g) {
    // std::cout << bounds.getX() << bounds.getY() << bounds.getWidth() << bounds.getHeight() << std::endl;
    g.setColour (juce::Colours::white);
    g.fillRect (getLocalBounds());
    g.setColour (juce::Colours::black);
    g.drawRect (getLocalBounds());
    // if (display_buffer != nullptr) { // buffer loaded

    // draw the waveform
    juce::Path p;

    // dereference pointer / copy memory here
    // we need a copy because the underlying memory could technically 
    // change on the audio thread while this function is getting called
    auto buffer = *(display_buffer->load()); 
    
    auto scale_w = float(buffer.getNumSamples()) / float(getWidth());

    auto read_pointer = buffer.getReadPointer(0);

    p.startNewSubPath(0, getHeight() / 2);

    for (float i = 0.0f; i < buffer.getNumSamples(); i += scale_w) {
        // points.push_back());
        p.lineTo(float(i) / scale_w, juce::jmap<float>(read_pointer[int(i)], -1.f, 1.f, float(getHeight()), 0.f));
    }

    g.strokePath(p, juce::PathStrokeType(2));
    // }
    // else {
    //     g.drawFittedText ("No File Loaded", getBounds(), juce::Justification::centred, 1);
    // }
}