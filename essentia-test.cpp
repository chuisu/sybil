#include <essentia/algorithmfactory.h>
#include <essentia/essentiamath.h>
#include <essentia/pool.h>

using namespace std;
using namespace essentia;

int main() {
    // Initialize Essentia
    essentia::init();

    // Create an algorithm factory
    standard::AlgorithmFactory& factory = standard::AlgorithmFactory::instance();

    // Use the factory to create an instance of the RMS algorithm
    standard::Algorithm* rms = factory.create("RMS");

    // Use the algorithm
    std::vector<Real> inputArray = {0.0, 1.0, 0.0, 1.0, 0.0, 1.0};
    Real rmsValue;
    rms->input("array").set(inputArray);
    rms->output("rms").set(rmsValue);
    rms->compute();

    // Print the result
    std::cout << "RMS: " << rmsValue << std::endl;

    // Clean up
    delete rms;
    essentia::shutdown();

    return 0;
}