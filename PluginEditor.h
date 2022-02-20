#pragma once

#include "PluginProcessor.h"

//==============================================================================
class AudioPluginAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    explicit AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor&);
    ~AudioPluginAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    AudioPluginAudioProcessor& processorRef;

    //==============================================================================
    //SET UP USER INTERFACE ELEMENTS
    //==============================================================================

    //Voice ADSR Sliders
    juce::Slider env1_attack_slider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> env1_attack_slider_attachment;
    juce::Slider env1_decay_slider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> env1_decay_slider_attachment;
    juce::Slider env1_sustain_slider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> env1_sustain_slider_attachment;
    juce::Slider env1_release_slider;
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

    // File Loading
    juce::TextButton open_button;
    std::unique_ptr<juce::FileChooser> file_chooser;
    juce::AudioFormatManager format_manager;
    void openButtonClicked();

    juce::TextButton play_button;
    void playButtonClicked();
    //==============================================================================

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};

