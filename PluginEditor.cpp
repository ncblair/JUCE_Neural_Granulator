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

    // ADSR Visualizer
    addAndMakeVisible(adsr_display_1);
    repaint_adsr_1();
    env1_attack_slider.onValueChange = [this] {repaint_adsr_1();};
    env1_decay_slider.onValueChange = [this] {repaint_adsr_1();};
    env1_sustain_slider.onValueChange = [this] {repaint_adsr_1();};
    env1_release_slider.onValueChange = [this] {repaint_adsr_1();};

    //Grain Shape Parameters
    addAndMakeVisible(grain_duration_slider);
    grain_duration_slider_attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.apvts, "GRAIN_DURATION", grain_duration_slider);
    addAndMakeVisible(grain_scan_slider);
    grain_scan_slider_attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.apvts, "GRAIN_SCAN", grain_scan_slider);

    //Granulator Parameters
    addAndMakeVisible(jitter_slider);
    jitter_slider_attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.apvts, "JITTER", jitter_slider);
    addAndMakeVisible(grain_rate_slider);
    grain_rate_slider_attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.apvts, "GRAIN_RATE", grain_rate_slider);

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

    // Add morph buffer waveform display
    addAndMakeVisible(morph_waveform);
    // repaint waveform on slider movements
    morph_slider.onValueChange = [this] { morph_waveform.repaint(); }; // morph slider repaints waveform component
    file_scan_sliders[0].onValueChange = [this] { morph_waveform.repaint(); }; // file pos sliders repaints waveform component
    file_scan_sliders[1].onValueChange = [this] { morph_waveform.repaint(); };

    processorRef.sounds[0].thumbnail.addChangeListener (this);
    processorRef.sounds[1].thumbnail.addChangeListener (this);

    // GRAIN ENVELOPE
    addAndMakeVisible(grain_env_width_slider);
    grain_env_width_slider_attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.apvts, "GRAIN_ENV_WIDTH", grain_env_width_slider);
    addAndMakeVisible(grain_env_center_slider);
    grain_env_center_slider_attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.apvts, "GRAIN_ENV_CENTER", grain_env_center_slider);
    grain_env_compute.setSamplingRate(sample_rate);
    addAndMakeVisible(grain_env_waveform);
    cur_grain_env_width.reset(sample_rate, 0.0);
    cur_grain_env_center.reset(sample_rate, 0.0);
    repaint_grain_env();
    grain_env_width_slider.onValueChange = [this] { repaint_grain_env(); };
    grain_env_center_slider.onValueChange = [this] { repaint_grain_env(); };

    // Filter
    addAndMakeVisible(filter_cutoff_slider);
    filter_cutoff_slider_attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.apvts, "FILTER_CUTOFF", filter_cutoff_slider);
    addAndMakeVisible(filter_q_slider);
    filter_q_slider_attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.apvts, "FILTER_Q", filter_q_slider);
    addAndMakeVisible(filter_type_combo_box);
    filter_type_combo_box.addItemList(processorRef.filter_types, 1);
    filter_type_combo_box_attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processorRef.apvts, "FILTER_TYPE", filter_type_combo_box);
    addAndMakeVisible(filter_on_button);
    filter_on_button.setColour(juce::ToggleButton::ColourIds::tickColourId, juce::Colours::black);
    filter_on_button.setColour(juce::ToggleButton::ColourIds::tickDisabledColourId, juce::Colours::black);
    filter_on_button_attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processorRef.apvts, "FILTER_ON", filter_on_button);

    // Filter Visualizer
    addAndMakeVisible(filter_display);
    repaint_filter_visualizer();
    filter_cutoff_slider.onValueChange = [this] {repaint_filter_visualizer();};
    filter_q_slider.onValueChange = [this] {repaint_filter_visualizer();};
    filter_type_combo_box.onChange = [this] {repaint_filter_visualizer();};
    
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
    env1_attack_slider.setBounds(30., 630., 55, 50);
    env1_decay_slider.setBounds(75., 630., 55, 50);
    env1_sustain_slider.setBounds(125., 630., 55, 50);
    env1_release_slider.setBounds(170., 630., 55, 50);

    adsr_display_1.setBounds(50., 520., 180., 100.);

    grain_duration_slider.setBounds(getWidth() / 2.0 - 100., 350, 200, 40);
    grain_scan_slider.setBounds(getWidth() / 2.0 - 100., 390, 200, 40);
    jitter_slider.setBounds(getWidth() / 2.0 - 100., 430, 200, 40);
    grain_rate_slider.setBounds(getWidth() / 2.0 - 100., 470, 200, 40);    

    // set waveform bounds
    waveform_1_bounds = juce::Rectangle<int>(50., 50., 280., 80.);
    waveform_2_bounds = juce::Rectangle<int>(570., 50., 280., 80.);
    // file scan within waveforms
    file_scan_sliders[0].setBounds(50., 130., 280., 50.);
    file_scan_sliders[1].setBounds(570., 130., 280., 50.);

    file_open_buttons[0].setBounds(50, 30., 140., 20.);
    morph_slider.setBounds(340., 75., 190., 50.);
    file_open_buttons[1].setBounds(570., 30., 140., 20.);

    morph_waveform.setBounds(310., 250., 280., 80.);

    grain_env_width_slider.setBounds(70., 360., 50., 50.);
    grain_env_center_slider.setBounds(120., 360., 50., 50.);
    grain_env_waveform.setBounds(50., 250., 140., 60.);

    // filter
    filter_cutoff_slider.setBounds(695., 360., 70., 50.);
    filter_q_slider.setBounds(765., 360., 70., 50.);
    filter_type_combo_box.setBounds(670., 225., 100., 25.);
    filter_display.setBounds(670., 250., 180., 80.);
    filter_on_button.setBounds(800., 225., 25., 25.);
}

void AudioPluginAudioProcessorEditor::repaint_adsr_1() {
    temp_adsr_buf_1.clear();
    auto write_pointer = temp_adsr_buf_1.getWritePointer(0);
    auto attack = processorRef.apvts.getRawParameterValue("ENV1_ATTACK")->load();
    auto decay = processorRef.apvts.getRawParameterValue("ENV1_DECAY")->load();
    auto sustain = processorRef.apvts.getRawParameterValue("ENV1_SUSTAIN")->load();
    auto release = processorRef.apvts.getRawParameterValue("ENV1_RELEASE")->load();

    auto total_time = attack + decay + release;
    float length = float(num_adsr_viewer_points);
    float attack_distance = attack * length / total_time;
    float decay_distance = decay * length / total_time;
    float release_distance = release * length / total_time;


    for (int i = 0; i < length; ++i) {
        if (i < attack_distance) {
            //attack
            write_pointer[i] = float(i) / attack_distance;
        }
        else if (i < attack_distance + decay_distance) {
            // decay
            write_pointer[i] = 1.0 - (1.0 - sustain) * (float(i) - attack_distance) / decay_distance;
        }
        else {
            // release
            write_pointer[i] = sustain*(1 - ((float(i) - attack_distance - decay_distance) / release_distance));
        }
        write_pointer[i] = (write_pointer[i] / 1.2) - 1.0;
    }


    adsr_buf_1.queue_new_buffer(&temp_adsr_buf_1);
    adsr_buf_1.update();
    adsr_display_1.repaint();

}

void AudioPluginAudioProcessorEditor::repaint_grain_env() {
    grain_env_compute.set(0.5f);
    cur_grain_env_width.setCurrentAndTargetValue (processorRef.apvts.getRawParameterValue("GRAIN_ENV_WIDTH")->load());
    cur_grain_env_center.setCurrentAndTargetValue (processorRef.apvts.getRawParameterValue("GRAIN_ENV_CENTER")->load());
    // not using this safebuffer safely, but that's okay, because this is always on editor thread
    // unfortunately, some technical debt is building up
    temp_grain_env_buffer.clear();
    auto write_pointer = temp_grain_env_buffer.getWritePointer(0);
    for (int i = 0; i < grain_env_buffer.get_num_samples(); ++i) {
        write_pointer[i] = grain_env_compute.step(1.0f) * 1.75f - 1.0f;
    }
    grain_env_buffer.queue_new_buffer(&temp_grain_env_buffer);
    grain_env_buffer.update();
    grain_env_waveform.repaint();
}

void AudioPluginAudioProcessorEditor::repaint_filter_visualizer() {
    auto type = int(processorRef.filter_type.getValue());
    switch (type) {
        case AudioPluginAudioProcessor::LOWPASS:
            helper_filter_coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(sample_rate, 
                                                    processorRef.filter_cutoff.getValue(), 
                                                    processorRef.filter_q.getValue());
            break;
        case AudioPluginAudioProcessor::HIGHPASS:
            helper_filter_coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(sample_rate, 
                                                    processorRef.filter_cutoff.getValue(), 
                                                    processorRef.filter_q.getValue());
            break;
        case AudioPluginAudioProcessor::BANDPASS:
            helper_filter_coefficients = juce::dsp::IIR::Coefficients<float>::makeBandPass(sample_rate, 
                                                    processorRef.filter_cutoff.getValue(),
                                                    processorRef.filter_q.getValue());
            break;
    }

    temp_mag_response_buf.clear();
    auto write_pointer = temp_mag_response_buf.getWritePointer(0);

    auto freq = 10.0;
    // hardcode max to be 24000
    auto freq_mult = pow(24000.0 / (2.0* freq), 1.0 / double(num_filter_visualizer_samples));
    for (int i = 0; i < num_filter_visualizer_samples; ++i) {
        freq = freq * freq_mult;
        auto mag = helper_filter_coefficients->getMagnitudeForFrequency(freq, sample_rate);
        write_pointer[i] = float((mag / 1.5) - 1.0);
    }
    mag_response_buf.queue_new_buffer(&temp_mag_response_buf);
    mag_response_buf.update();
    filter_display.repaint();
}

void AudioPluginAudioProcessorEditor::open_file(int file_index) {
    processorRef.sounds[file_index].load_file(&file_open_buttons[file_index], sample_rate);
}

void AudioPluginAudioProcessorEditor::changeListenerCallback (juce::ChangeBroadcaster* source) {
    if (source == &processorRef.sounds[0].thumbnail || source == &processorRef.sounds[1].thumbnail) {
        repaint(); // thumbnail changed
    }
}