# NEURAL GRANULATOR VST (second repository as of 1/29/2022)

### About:

Neural Granulator is a granular synthesizer with neural grain synthesis and interpolation.


### Installation (CPP, cmake):
 - Download JUCE and install it locally
 ```
# Go to JUCE directory
cd /path/to/clone/JUCE
# Configure build with library components only
cmake -B cmake-build-install -DCMAKE_INSTALL_PREFIX=/path/to/JUCE/install
# Run the installation
cmake --build cmake-build-install --target install
```
 - Download cmake (see: https://github.com/juce-framework/JUCE/blob/master/docs/CMake%20API.md)
 - Download Pytorch for c++ (Libtorch, see: https://pytorch.org/get-started/locally/, https://pytorch.org/cppdocs/installing.html) 
 - Download my torch model and put it in the MODELS folder: https://drive.google.com/drive/folders/1i2Yseex_LyaXYA8ape5NQVdFgWrMm0yo?usp=sharing
 - Change the code in the PluginEditor.cpp constructor to match the paths on your filesystem.
 - Build the project
```
mkdir build 
cd build 
# include both path to JUCE and libtorch in your DCMAKE_PREFIX_PATH
cmake -DCMAKE_PREFIX_PATH="~/JUCE/install;/absolute/path/to/libtorch" ..
cmake --build .
```
