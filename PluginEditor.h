#pragma once

#include "PluginProcessor.h"
#include "GranLookAndFeel.h"

//==============================================================================
class AudioPluginAudioProcessorEditor  : public juce::AudioProcessorEditor, public juce::ChangeListener
{
public:
    explicit AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor&);
    ~AudioPluginAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    void changeListenerCallback (juce::ChangeBroadcaster* source) override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    AudioPluginAudioProcessor& processorRef;

    //==============================================================================
    //LOOK AND FEEL 
    //==============================================================================
    GranLookAndFeel look_and_feel;

    //==============================================================================
    //SET UP USER INTERFACE ELEMENTS
    //==============================================================================

    //Voice ADSR Sliders
    juce::Slider env1_attack_slider{juce::Slider::Rotary, juce::Slider::TextEntryBoxPosition::TextBoxBelow};
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> env1_attack_slider_attachment;
    juce::Slider env1_decay_slider{juce::Slider::Rotary, juce::Slider::TextEntryBoxPosition::TextBoxBelow};
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> env1_decay_slider_attachment;
    juce::Slider env1_sustain_slider{juce::Slider::Rotary, juce::Slider::TextEntryBoxPosition::TextBoxBelow};
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> env1_sustain_slider_attachment;
    juce::Slider env1_release_slider{juce::Slider::Rotary, juce::Slider::TextEntryBoxPosition::TextBoxBelow};
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> env1_release_slider_attachment;

    //Grain Shape Parameters
    juce::Slider grain_size_slider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> grain_size_slider_attachment;
    juce::Slider grain_scan_slider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> grain_scan_slider_attachment;

    //Granulator Parameters
    juce::Slider spray_slider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> spray_slider_attachment;
    juce::Slider density_slider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> density_slider_attachment;
    // juce::ComboBox grain_env_type_combo_box;
    // std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> grain_env_type_combo_box_attachment;

    //Morphing Slider
    juce::Slider morph_slider{juce::Slider::LinearHorizontal, juce::Slider::TextEntryBoxPosition::TextBoxBelow};
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> morph_slider_attachment;

    // File Loading
    juce::TextButton file_open_buttons[2];

    // File Position
    juce::Slider file_scan_sliders[2];
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> file_scan_slider_attachments[2];

    // Waveforms
    juce::Rectangle<int> waveform_1_bounds;
    juce::Rectangle<int> waveform_2_bounds;

    // std::unique_ptr<juce::FileChooser> file_chooser;
    // juce::AudioFormatManager format_manager;
    void open_file(int file_index);

    // juce::TextButton play_button;
    // void playButtonClicked();
    //==============================================================================

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};

