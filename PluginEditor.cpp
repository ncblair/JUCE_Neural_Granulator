#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    juce::ignoreUnused (processorRef);
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (900, 700);

    // set look and feel class
    setLookAndFeel(&look_and_feel);

    //==============================================================================
    //ADD AND MAKE VISIBLE USER INTERFACE ELEMENTS
    //==============================================================================

    //Voice ADSR Sliders
    addAndMakeVisible(env1_attack_slider);
    env1_attack_slider_attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.apvts, "ENV1_ATTACK", env1_attack_slider);
    addAndMakeVisible(env1_decay_slider);
    env1_decay_slider_attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.apvts, "ENV1_DECAY", env1_decay_slider);
    addAndMakeVisible(env1_sustain_slider);
    env1_sustain_slider_attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.apvts, "ENV1_SUSTAIN", env1_sustain_slider);
    addAndMakeVisible(env1_release_slider);
    env1_release_slider_attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.apvts, "ENV1_RELEASE", env1_release_slider);

    //Grain Shape Parameters
    addAndMakeVisible(grain_size_slider);
    grain_size_slider_attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.apvts, "GRAIN_SIZE", grain_size_slider);
    addAndMakeVisible(grain_scan_slider);
    grain_scan_slider_attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.apvts, "GRAIN_SCAN", grain_scan_slider);

    //Granulator Parameters
    addAndMakeVisible(spray_slider);
    spray_slider_attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.apvts, "SPRAY", spray_slider);
    addAndMakeVisible(density_slider);
    density_slider_attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.apvts, "DENSITY", density_slider);

    // Morphing Parameters
    addAndMakeVisible(morph_slider);
    morph_slider_attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.apvts, "MORPH", morph_slider);

    
    // Add file parameters
    for (int i = 0; i < 2; ++i) {
        file_scan_sliders[i].setTextBoxStyle (juce::Slider::TextEntryBoxPosition::TextBoxBelow, false, 280, 25);
        addAndMakeVisible (&file_open_buttons[i]);
        file_open_buttons[i].setButtonText ("Open...");
        file_open_buttons[i].onClick = [this, i]{ open_file(i); };

        addAndMakeVisible(&file_scan_sliders[i]);
        file_scan_slider_attachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processorRef.apvts, "FILE_SCAN_" + std::to_string(i), file_scan_sliders[i]
        );
    }

    // addAndMakeVisible (&play_button);
    // play_button.setButtonText ("Play...");
    // play_button.onClick = [this] { playButtonClicked(); };

    // format_manager.registerBasicFormats();
    processorRef.sounds[0].thumbnail.addChangeListener (this);
    processorRef.sounds[1].thumbnail.addChangeListener (this);
}


AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);

    //paint file 1
    processorRef.sounds[0].paint(g, waveform_1_bounds);
    processorRef.sounds[1].paint(g, waveform_2_bounds);
}

void AudioPluginAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    env1_attack_slider.setBounds(100., 600, 200, 50);
    env1_decay_slider.setBounds(100., 650, 200, 50);
    env1_sustain_slider.setBounds(200., 600, 200, 50);
    env1_release_slider.setBounds(200., 650, 200, 50);

    grain_size_slider.setBounds(getWidth() / 2.0 - 100., 250, 200, 50);
    grain_scan_slider.setBounds(getWidth() / 2.0 - 100., 300, 200, 50);

    spray_slider.setBounds(getWidth() / 2.0 - 100., 350, 200, 50);
    density_slider.setBounds(getWidth() / 2.0 - 100., 400, 200, 50);    

    // set waveform bounds
    waveform_1_bounds = juce::Rectangle<int>(10., 50., 280., 80.);
    waveform_2_bounds = juce::Rectangle<int>(390., 50., 280., 80.);
    // file scan within waveforms
    file_scan_sliders[0].setBounds(10., 130., 280., 50.);
    file_scan_sliders[1].setBounds(390., 130., 280., 50.);

    file_open_buttons[0].setBounds(10, 30., 140., 20.);
    morph_slider.setBounds(300., 75., 80., 50.);
    file_open_buttons[1].setBounds(390., 30., 140., 20.);

}

void AudioPluginAudioProcessorEditor::open_file(int file_index) {
    processorRef.sounds[file_index].load_file(&file_open_buttons[file_index]);
}

void AudioPluginAudioProcessorEditor::changeListenerCallback (juce::ChangeBroadcaster* source) {
    if (source == &processorRef.sounds[0].thumbnail || source == &processorRef.sounds[1].thumbnail) {
        repaint(); // thumbnail changed
    }
}