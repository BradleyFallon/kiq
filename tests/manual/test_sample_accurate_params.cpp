#include "include/AudioEngine.h"
#include "parameters/ParameterEventQueue.h"
#include "parameters/ParameterManager.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

using namespace KickDrum;

int main() {
    AudioEngine engine;
    AudioEngine baseline;
    engine.initialize(48000.0f);
    baseline.initialize(48000.0f);
    engine.setSoftClippingEnabled(false);
    baseline.setSoftClippingEnabled(false);
    engine.noteOn(36, 1.0f);
    baseline.noteOn(36, 1.0f);
    engine.getParameterEventQueue()->addEvent("outputGain", 0.0f, 128);

    std::vector<float> buffer(256);
    std::vector<float> baselineBuffer(256);
    engine.processBlock(buffer.data(), buffer.size(), 1);
    baseline.processBlock(baselineBuffer.data(), baselineBuffer.size(), 1);
    for (int sample = 0; sample < 128; ++sample) {
        assert(buffer[sample] == baselineBuffer[sample]);
    }

    int firstChangedSample = -1;
    float renderedEnergy = 0.0f;
    float baselineEnergy = 0.0f;
    for (int sample = 128; sample < 256; ++sample) {
        if (firstChangedSample < 0 &&
            std::abs(buffer[sample] - baselineBuffer[sample]) > 1.0e-7f) {
            firstChangedSample = sample;
        }
        renderedEnergy += std::abs(buffer[sample]);
        baselineEnergy += std::abs(baselineBuffer[sample]);
    }
    assert(firstChangedSample == 128);
    assert(renderedEnergy < baselineEnergy);

    // Output gain reaches zero after its intentional 5 ms smoothing ramp.
    std::vector<float> tail(256);
    engine.processBlock(tail.data(), tail.size(), 1);
    for (int sample = 128; sample < 256; ++sample)
        assert(tail[sample] == 0.0f);

    assert(engine.getParameterManager()->getParameterValue("outputGain") == 0.0f);
    std::cout << "Sample-accurate smoothed parameter test passed\n";
    return 0;
}
