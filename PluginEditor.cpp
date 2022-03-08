#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    juce::ignoreUnused (processorRef);
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (400, 600);

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
    // addAndMakeVisible(grain_env_type_combo_box);
    // grain_env_type_combo_box.addItemList({"Expodec", "Gaussian", "Rexpodec"}, 1);
    // grain_env_type_combo_box_attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processorRef.apvts, "GRAIN_ENV_TYPE", grain_env_type_combo_box);

    // Add open file button
    addAndMakeVisible (&open_button);
    open_button.setButtonText ("Open...");
    open_button.onClick = [this] { openButtonClicked(); };

    addAndMakeVisible (&play_button);
    play_button.setButtonText ("Play...");
    play_button.onClick = [this] { playButtonClicked(); };

    format_manager.registerBasicFormats();

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
}

void AudioPluginAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    env1_attack_slider.setBounds(getWidth() / 2.0 - 100., 50, 200, 50);
    env1_decay_slider.setBounds(getWidth() / 2.0 - 100., 100, 200, 50);
    env1_sustain_slider.setBounds(getWidth() / 2.0 - 100., 150, 200, 50);
    env1_release_slider.setBounds(getWidth() / 2.0 - 100., 200, 200, 50);

    grain_size_slider.setBounds(getWidth() / 2.0 - 100., 250, 200, 50);
    grain_scan_slider.setBounds(getWidth() / 2.0 - 100., 300, 200, 50);

    spray_slider.setBounds(getWidth() / 2.0 - 100., 350, 200, 50);
    density_slider.setBounds(getWidth() / 2.0 - 100., 400, 200, 50);
    // grain_env_type_combo_box.setBounds(getWidth() / 2.0 - 100., 450, 200, 50);
    open_button.setBounds(int(getWidth() / 2.0f - 100.f), 450, 200, 50);
    play_button.setBounds(int(getWidth() / 2.0f - 100.f), 500, 200, 50);

}

void AudioPluginAudioProcessorEditor::openButtonClicked () {
    // adapted from https://docs.juce.com/master/tutorial_playing_sound_files.html
    file_chooser = std::make_unique<juce::FileChooser> ("Select a Wave file to play...", juce::File{}, "*.wav");
    auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

    file_chooser->launchAsync (chooserFlags, [this] (const FileChooser& fc)
    {
        auto file = fc.getResult();

        if (file != File{})
        {
            auto* reader = format_manager.createReaderFor(file);

            if (reader != nullptr)
            {
                const int num_channels = int(reader->numChannels);
                const int num_samples = int(reader->lengthInSamples);
                // get audio from file into an audio buffer
                auto audio = new juce::AudioBuffer<float>(num_channels, num_samples);
                reader->read(audio, 0, num_samples, 0, true, true);

                // inform processor of the sample rate, num_samples, and audio_channels.
                processorRef.morph_buf.queue_new_buffer(audio); 
                processorRef.file1_sample_rate.store(reader->sampleRate);

                // Change text to indicate process finished!    
                open_button.setButtonText (file.getFileName());
                processorRef.file1_loaded.store(true);
            }
        }
    });
}

void AudioPluginAudioProcessorEditor::playButtonClicked () {
    // adapted from https://docs.juce.com/master/tutorial_playing_sound_files.html
    std::cout << "Play audio file";
    processorRef.play_sample.store(true);
}