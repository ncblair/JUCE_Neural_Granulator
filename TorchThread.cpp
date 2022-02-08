#include "TorchThread.h"

TorchThread::TorchThread(AudioPluginAudioProcessorEditor& e, const juce::String &threadName) : juce::Thread(threadName), editor(e) {
    at::init_num_threads();
    std::cout << "Attempting load model" << std::endl;
    model = torch::jit::load("/Users/ncblair/COMPSCI/NeuralGranulator/JUCE_CPP/MODELS/rave_nylon_guitar_01.ts");
    latent_shape = std::make_unique<at::Tensor>(model.run_method("encode", torch::zeros({1, 1, 32768})).toTensor());
    // init grain buffer
    // ready_to_update = true;
}

void TorchThread::gen_new_grain() {
    // auto time = juce::Time::getMillisecondCounter();
    auto mean = torch::zeros_like(*latent_shape);
    // mean[0] = 3.5;
    // mean[1] = 1;
    at::Tensor normal = torch::randn_like(*latent_shape)*0.1;
    mean[0][0] = 6. * x() - 3;
    mean[0][1] = 6. * y() - 3;
    // mean[0][2] = 4. * x + 2;
    // mean[0][4] = 4. * y + 2;
    std::vector<torch::jit::IValue> inputs;
    inputs.push_back(normal + mean);
    std::cout << "Attempt Gen New Grain " << std::endl;

    auto result = model.run_method("decode", normal + mean);
    std::cout << "TENSOR SIZE " << result.toTensor().dim() << std::endl;
    auto output = result.toTensor()[0][0];

    // NORMALIZE GRAIN ??? Might not need to with better model
    float max = *at::max(at::abs(output)).data_ptr<float>();
    if (max > 0.0) { //dont divide by 0
        output = output / max; 
        output = output / 2.0;
    }
    //END NORMALIZE GRAIN
    editor.processorRef.granulator.replace_grain(output);
}

void TorchThread::run() {
    std::cout << "ml thread started" << std::endl;
    while (!threadShouldExit()) {
        auto temp_x = -1;
        auto temp_y = -1;
        /*
        if x and y changed during call to gen_new_grain()
        then re-run gen_new_grain() on the new position
        */
        while ((temp_x != x()) || (temp_y != y())) {
            // note x and y values before generating the grain
            temp_x = x(); 
            temp_y = y();
            gen_new_grain();
        }
        /*
        if gen_new_grain() runs and position hasn't changed, 
        sleep until thread is notified
        */ 
        wait(-1);
        // if (ready_to_update) {
        //     ready_to_update = false;
        //     gen_new_grain();
        // }
    }
    std::cout << "exit ML thread" << std::endl;
}
