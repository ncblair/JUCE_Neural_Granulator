#pragma once

#include <JuceHeader.h>
#include <torch/torch.h>

#include "PluginEditor.h"


class TorchThread : public juce::Thread {
    private: 
        torch::jit::script::Module model;

    public:
        AudioPluginAudioProcessorEditor& editor;
        std::atomic<float> x{0.75}; // 0 to 1
        std::atomic<float> y{0.5}; // 0 to 1
        // std::atomic<bool> ready_to_update;
        std::unique_ptr<at::Tensor> latent_shape;

        TorchThread(AudioPluginAudioProcessorEditor& e, const juce::String &threadName);
        void gen_new_grain();
        void run() override;
};