#pragma once

#include "PluginProcessor.h"
#include "GranLookAndFeel.h"
#include "WaveformDisplay.h"
#include "utils.h"
#include "GranulatorVoice.h"

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
    double sample_rate = processorRef.getSampleRate();

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

    // Voice ADSR (Env 1) Visualizer
    int num_adsr_viewer_points = 1000;
    SafeBuffer adsr_buf_1{1, num_adsr_viewer_points};
    juce::AudioBuffer<float> temp_adsr_buf_1{1, num_adsr_viewer_points};
    WaveformDisplay adsr_display_1{&adsr_buf_1};
    void repaint_adsr_1();

    //Grain Shape Parameters
    juce::Slider grain_duration_slider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> grain_duration_slider_attachment;
    juce::Slider grain_scan_slider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> grain_scan_slider_attachment;

    //Granulator Parameters
    juce::Slider jitter_slider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> jitter_slider_attachment;
    juce::Slider grain_rate_slider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> grain_rate_slider_attachment;

    //Morphing Slider
    juce::Slider morph_slider{juce::Slider::LinearHorizontal, juce::Slider::TextEntryBoxPosition::TextBoxBelow};
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> morph_slider_attachment;

    // File Loading
    juce::TextButton file_open_buttons[2];

    // File Position
    juce::Slider file_scan_sliders[2];
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> file_scan_slider_attachments[2];

    // Soundfile Waveforms
    juce::Rectangle<int> waveform_1_bounds;
    juce::Rectangle<int> waveform_2_bounds;
    // Morph Waveform
    WaveformDisplay morph_waveform{&processorRef.morph_buf};

    // Grain modified tukey window with width and center
    juce::Slider grain_env_width_slider{juce::Slider::Rotary, juce::Slider::TextEntryBoxPosition::TextBoxBelow};
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> grain_env_width_slider_attachment;
    juce::Slider grain_env_center_slider{juce::Slider::Rotary, juce::Slider::TextEntryBoxPosition::TextBoxBelow};
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> grain_env_center_slider_attachment;

    // Grain Envelope Display
    SafeBuffer grain_env_buffer{1, int(sample_rate / 2.0)};
    juce::AudioBuffer<float> temp_grain_env_buffer{1, int(sample_rate / 2.0)};
    WaveformDisplay grain_env_waveform{&grain_env_buffer};
    juce::SmoothedValue<float> cur_grain_env_width;
    juce::SmoothedValue<float> cur_grain_env_center;
    TriangularRangeTukey grain_env_compute{&cur_grain_env_width, &cur_grain_env_center};
    void repaint_grain_env();

    // Filter
    juce::Slider filter_cutoff_slider{juce::Slider::Rotary, juce::Slider::TextEntryBoxPosition::TextBoxBelow};
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filter_cutoff_slider_attachment;
    juce::Slider filter_q_slider{juce::Slider::Rotary, juce::Slider::TextEntryBoxPosition::TextBoxBelow};
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filter_q_slider_attachment;
    juce::ComboBox filter_type_combo_box;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> filter_type_combo_box_attachment;
    juce::ToggleButton filter_on_button;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> filter_on_button_attachment;

    // Filter Visualizer
    int num_filter_visualizer_samples = int(sample_rate / 20.0);
    SafeBuffer mag_response_buf{1, num_filter_visualizer_samples};
    juce::AudioBuffer<float> temp_mag_response_buf{1, num_filter_visualizer_samples};
    WaveformDisplay filter_display{&mag_response_buf};
    juce::dsp::IIR::Coefficients<float>::Ptr helper_filter_coefficients;
    void repaint_filter_visualizer();

    // std::unique_ptr<juce::FileChooser> file_chooser;
    // juce::AudioFormatManager format_manager;
    void open_file(int file_index);

    // juce::TextButton play_button;
    // void playButtonClicked();
    //==============================================================================

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};

