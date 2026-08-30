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
    engine.initialize(48000.0f);
    engine.setSoftClippingEnabled(false);
    engine.noteOn(36, 1.0f);
    engine.getParameterEventQueue()->addEvent("outputGain", 0.0f, 128);

    std::vector<float> buffer(256);
    engine.processBlock(buffer.data(), buffer.size(), 1);
    bool audibleBeforeEvent = false;
    for (int sample = 0; sample < 128; ++sample) {
        audibleBeforeEvent = audibleBeforeEvent || std::abs(buffer[sample]) > 0.0f;
    }
    assert(audibleBeforeEvent);
    for (int sample = 128; sample < 256; ++sample) {
        assert(buffer[sample] == 0.0f);
    }
    assert(engine.getParameterManager()->getParameterValue("outputGain") == 0.0f);
    std::cout << "Sample-accurate trajectory parameter test passed\n";
    return 0;
}
