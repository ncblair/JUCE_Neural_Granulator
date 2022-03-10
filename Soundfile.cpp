#include "Soundfile.h"

Soundfile::Soundfile() {
    format_manager.registerBasicFormats();
}

void Soundfile::load_file(juce::TextButton* button) {
    // adapted from https://docs.juce.com/master/tutorial_playing_sound_files.html
    file_chooser = std::make_unique<juce::FileChooser> ("Select a Wave file to play...", juce::File{}, "*.wav");
    auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

    file_chooser->launchAsync (chooserFlags, [this, button] (const FileChooser& fc)
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
                file_buffer.queue_new_buffer(audio); 
                sample_rate.store(reader->sampleRate);

                // initialize region buf temp
                region_buf_temp = juce::AudioBuffer<float>(1, sample_rate * region_num_seconds.load());

                thumbnail.setSource (new juce::FileInputSource (file));

                // Change text to indicate process finished!
                button->setButtonText(file.getFileName());
                file_loaded.store(true);
                new_region.store(true);
            }
        }
    });
}

void Soundfile::update_parameters(float region_scan_arg) {
    // called from audio thread

    // update file buffer first
    file_buffer.update();

    // then if a new region is set queue the region into the region_buffer
    if ((region_scan.load() != region_scan_arg) && file_loaded.load()) {
        new_region.store(true);
    }
    if (new_region.load()) {
        region_scan.store(region_scan_arg);
        queue_region_buffer();
        scan_changed.store(true);
        new_region.store(false);
    }
    
    region_buffer.update();
}

float Soundfile::get_num_samples() {
    return region_num_seconds.load() * sample_rate.load();
}

void Soundfile::queue_region_buffer() {
    // assume this happens on the audio thread
    // Fill in our temp audio buffer with the correct samples and then queue it
    // We need to put it into a safe buffer so it can be read from the background thread for morphing
    auto num_samples = region_buf_temp.getNumSamples();
    auto cur_sample = int(region_scan.load() * float(file_buffer.get_num_samples()));
    auto num_samples_left = num_samples;
    while (num_samples_left > 0) {
        // circular buffer
        auto samples_this_time = juce::jmin(num_samples_left, file_buffer.get_num_samples() - cur_sample);
        // TODO: do we need to dereference here?
        region_buf_temp.copyFrom(0, num_samples - num_samples_left, *(file_buffer.load()), 0, cur_sample, samples_this_time);
        num_samples_left -= samples_this_time;
        cur_sample += samples_this_time;
        while (cur_sample >= file_buffer.get_num_samples()) {
            cur_sample -= file_buffer.get_num_samples();
        }
    }
    region_buffer.queue_new_buffer(&region_buf_temp);
}

void Soundfile::paint(juce::Graphics& g, const juce::Rectangle<int>& thumbnailBounds) {
    g.setColour (juce::Colours::white);
    g.fillRect (thumbnailBounds);
    g.setColour (juce::Colours::black);
    g.drawRect (thumbnailBounds);
    if (file_loaded.load()) {
        thumbnail.drawChannels (g,                                      // [9]
                                thumbnailBounds,
                                0.0,                                    // start time
                                thumbnail.getTotalLength(),             // end time
                                1.0f);                                  // vertical zoom
    }
    else {
        g.drawFittedText ("No File Loaded", thumbnailBounds, juce::Justification::centred, 1);
    }
}