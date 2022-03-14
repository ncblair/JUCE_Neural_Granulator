#include "WaveformDisplay.h"

WaveformDisplay::WaveformDisplay(SafeBuffer* buffer) {
    display_buffer = buffer;
}

void WaveformDisplay::paint(juce::Graphics& g) {
    auto bounds = getBounds();
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
    
    auto scale_w = buffer.getNumSamples() / getWidth();

    auto read_pointer = buffer.getReadPointer(0);

    p.startNewSubPath(0, getHeight() / 2);

    for (int i = 0; i < buffer.getNumSamples(); i += scale_w) {
        // points.push_back());
        p.lineTo(float(i) / scale_w, juce::jmap<float>(read_pointer[i], -1.f, 1.f, getHeight(), 0.f));
    }

    g.strokePath(p, juce::PathStrokeType(2));
    // }
    // else {
    //     g.drawFittedText ("No File Loaded", getBounds(), juce::Justification::centred, 1);
    // }
}