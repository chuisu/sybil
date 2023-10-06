#include <iostream>
#include <vector>
#include <tensorflow/core/public/session.h>
#include <tensorflow/core/platform/env.h>

int main() {
    // Initialize a TensorFlow session
    tensorflow::Session* session;
    tensorflow::Status status = tensorflow::NewSession(tensorflow::SessionOptions(), &session);
    if (!status.ok()) {
        std::cerr << status.ToString() << std::endl;
        return 1;
    }

    // Load the model
    tensorflow::GraphDef graph_def;
    status = ReadBinaryProto(tensorflow::Env::Default(), "path_to_your_model/saved_model.pb", &graph_def);
    if (!status.ok()) {
        std::cerr << "Error reading the model: " << status.ToString() << std::endl;
        return 1;
    }
    
    // Add the graph to the session
    status = session->Create(graph_def);
    if (!status.ok()) {
        std::cerr << "Error adding the graph to the session: " << status.ToString() << std::endl;
        return 1;
    }

    // Create a dummy input tensor
    tensorflow::Tensor input_tensor(tensorflow::DT_FLOAT, tensorflow::TensorShape({1, 12}));
    auto input_tensor_mapped = input_tensor.tensor<float, 2>();
    for (int i = 0; i < 12; i++) {
        input_tensor_mapped(0, i) = 0.5; // or any dummy value
    }

    // Get model prediction
    std::vector<tensorflow::Tensor> outputs;
    status = session->Run({{"dense_input:0", input_tensor}}, {"dense_6:0"}, {}, &outputs);
    if (!status.ok()) {
        std::cerr << "Error running the model: " << status.ToString() << std::endl;
        return 1;
    }

    // Extract the output and print it
    auto output = outputs[0].tensor<float, 2>();
    std::cout << "Predicted output: " << output(0, 0) << std::endl;

    // Clean up
    session->Close();
    delete session;

    return 0;
}
