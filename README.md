## Overview
This project showcase an attempt to provide a Unreal Engine 5 plugin using the NNE plugin with a locally running causal transformer model imported as onxx. Since the project couldn't be led to a successful implenentation and its implementation had to be stopped, the presented code is only intended for the Qwen/Qwen2.5-0.5B-Instruct transformer model.

_The onnx models are not in this project since they are too large, but any .onnx files can be dragged and dropped into the Unreal Engine Content folder if the NNEOnnxruntime plugin is installed inside a Unreal project **(note that this plugin automatically enables it)**._

## Installation
Simply drag and drop the repository root folder inside the Plugins folder of a Unreal Engine project **(it is not recommended to install it directly inside the engine Plugins folder)**.

### Requirements
- Unreal Engine 5.5
- Having the Qwen/Qwen2.5-0.5B-Instruct exported as .onnx and imported inside a Unreal Engine project as a UNNEModelData asset. 

## Notes
The plugin integrates the [tokenizers-cpp](https://github.com/mlc-ai/tokenizers-cpp) library as Third-Party library aside with the implementation to perform manual token encoding and decoding following the BPE (Bytes-Per-Level) method analogally to what the Qwen Instruct architecture expects.

Before the encoding, a softmax algorithm is used to process the output logits.
