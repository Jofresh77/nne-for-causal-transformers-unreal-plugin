## Overview
This project showcase an attempt to provide a Unreal Engine 5 plugin using the NNE plugin with a locally running causal transformer model imported as onxx.

_The onnx models are not in this project since they are too large, but any .onnx files can be dragged and dropped into the Unreal Engine Content folder if the NNEOnnxruntime plugin is installed inside a Unreal project._

## Notes
The plugin integrates the [tokenizers-cpp](https://github.com/mlc-ai/tokenizers-cpp) library as Third-Party library aside with the implementation to perform manual token encoding and decoding following the BPE (Bytes-Per-Level) method analogally to what the Qwen Instruct architecture expects.

Before the encoding, a softmax algorithm is used to process the output logits.
