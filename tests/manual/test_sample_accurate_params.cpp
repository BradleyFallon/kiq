#include "include/AudioEngine.h"
#include "parameters/ParameterEventQueue.h"
#include "parameters/ParameterManager.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <cassert>

using namespace KickDrum;

void testParameterEventQueue() {
    std::cout << "Testing ParameterEventQueue..." << std::endl;
    
    ParameterEventQueue queue;
    
    // Test: Empty queue
    assert(queue.isEmpty());
    assert(queue.getEventCount() == 0);
    std::cout << "  ✓ Empty queue test passed" << std::endl;
    
    // Test: Add events
    queue.addEvent("basePitch", 50.0f, 0);
    queue.addEvent("sineLevel", 0.8f, 100);
    queue.addEvent("harmonicRatio", 2.0f, 50);
    
    assert(queue.getEventCount() == 3);
    assert(!queue.isEmpty());
    std::cout << "  ✓ Add events test passed" << std::endl;
    
    // Test: Retrieve and sort events
    std::vector<ParameterEvent> events;
    queue.getEventsForBuffer(events);
    
    assert(events.size() == 3);
    assert(queue.isEmpty());
    
    // Events should be sorted by sample offset
    assert(events[0].sampleOffset == 0);
    assert(events[1].sampleOffset == 50);
    assert(events[2].sampleOffset == 100);
    std::cout << "  ✓ Retrieve and sort events test passed" << std::endl;
    
    // Test: Clear queue
    queue.addEvent("param1", 1.0f, 0);
    queue.clear();
    assert(queue.isEmpty());
    std::cout << "  ✓ Clear queue test passed" << std::endl;
    
    std::cout << "ParameterEventQueue tests passed!" << std::endl << std::endl;
}

void testAudioEngineParameterUpdates() {
    std::cout << "Testing AudioEngine parameter updates..." << std::endl;
    
    AudioEngine engine;
    engine.initialize(48000.0f);
    
    // Test: Get parameter event queue
    ParameterEventQueue* queue = engine.getParameterEventQueue();
    assert(queue != nullptr);
    assert(queue->isEmpty());
    std::cout << "  ✓ Parameter event queue accessible" << std::endl;
    
    // Test: setParameter schedules event
    engine.setParameter("basePitch", 60.0f);
    assert(queue->getEventCount() == 1);
    std::cout << "  ✓ setParameter schedules event" << std::endl;
    
    // Process buffer to consume event
    std::vector<float> buffer(512, 0.0f);
    engine.processBlock(buffer.data(), 512, 1);
    
    assert(queue->getEventCount() == 0);
    
    // Check parameter was updated
    ParameterManager* paramManager = engine.getParameterManager();
    assert(paramManager != nullptr);
    float basePitch = paramManager->getParameterValue("basePitch");
    assert(std::abs(basePitch - 60.0f) < 0.001f);
    std::cout << "  ✓ Parameter updated in manager" << std::endl;
    
    // Test: Multiple events in single buffer
    queue->addEvent("basePitch", 50.0f, 0);
    queue->addEvent("sineLevel", 80.0f, 128);
    queue->addEvent("harmonicRatio", 3.0f, 256);
    
    assert(queue->getEventCount() == 3);
    
    engine.noteOn(60, 1.0f);
    engine.processBlock(buffer.data(), 512, 1);
    
    assert(queue->getEventCount() == 0);
    
    // Check all parameters were updated
    assert(std::abs(paramManager->getParameterValue("basePitch") - 50.0f) < 0.001f);
    assert(std::abs(paramManager->getParameterValue("sineLevel") - 80.0f) < 0.001f);
    assert(std::abs(paramManager->getParameterValue("harmonicRatio") - 3.0f) < 0.001f);
    std::cout << "  ✓ Multiple events processed correctly" << std::endl;
    
    // Test: Events are processed in order
    queue->addEvent("basePitch", 100.0f, 256);
    queue->addEvent("basePitch", 50.0f, 0);
    queue->addEvent("basePitch", 75.0f, 128);
    
    engine.processBlock(buffer.data(), 512, 1);
    
    // Final value should be from last event (at offset 256)
    assert(std::abs(paramManager->getParameterValue("basePitch") - 100.0f) < 0.001f);
    std::cout << "  ✓ Events processed in order" << std::endl;
    
    // Test: Parameter changes affect audio
    engine.noteOn(60, 1.0f);
    
    std::vector<float> buffer1(512, 0.0f);
    engine.processBlock(buffer1.data(), 512, 1);
    
    // Calculate RMS of first buffer
    float rms1 = 0.0f;
    for (float sample : buffer1) {
        rms1 += sample * sample;
    }
    rms1 = std::sqrt(rms1 / buffer1.size());
    
    // Trigger another note and reduce sine level
    engine.noteOn(60, 1.0f);
    engine.setParameter("sineLevel", 0.0f);
    
    std::vector<float> buffer2(512, 0.0f);
    engine.processBlock(buffer2.data(), 512, 1);
    
    float rms2 = 0.0f;
    for (float sample : buffer2) {
        rms2 += sample * sample;
    }
    rms2 = std::sqrt(rms2 / buffer2.size());
    
    // Second buffer should have lower RMS
    assert(rms2 < rms1);
    std::cout << "  ✓ Parameter changes affect audio output" << std::endl;
    
    std::cout << "AudioEngine parameter update tests passed!" << std::endl << std::endl;
}

void testSampleAccurateProcessing() {
    std::cout << "Testing sample-accurate processing..." << std::endl;
    
    AudioEngine engine;
    engine.initialize(48000.0f);
    
    ParameterEventQueue* queue = engine.getParameterEventQueue();
    
    // Add events at different sample positions
    queue->addEvent("basePitch", 50.0f, 0);
    queue->addEvent("basePitch", 75.0f, 128);
    queue->addEvent("basePitch", 100.0f, 256);
    queue->addEvent("basePitch", 125.0f, 384);
    
    // Trigger note
    engine.noteOn(60, 1.0f);
    
    // Process buffer
    std::vector<float> buffer(512, 0.0f);
    engine.processBlock(buffer.data(), 512, 1);
    
    // All events should be consumed
    assert(queue->getEventCount() == 0);
    
    // Final parameter value should be from last event
    ParameterManager* paramManager = engine.getParameterManager();
    assert(std::abs(paramManager->getParameterValue("basePitch") - 125.0f) < 0.001f);
    
    // Buffer should contain audio
    bool hasAudio = false;
    for (float sample : buffer) {
        if (std::abs(sample) > 0.0001f) {
            hasAudio = true;
            break;
        }
    }
    assert(hasAudio);
    
    std::cout << "  ✓ Sample-accurate processing works correctly" << std::endl;
    std::cout << "Sample-accurate processing tests passed!" << std::endl << std::endl;
}

void testEnvelopeParameterUpdates() {
    std::cout << "Testing envelope parameter updates..." << std::endl;
    
    AudioEngine engine;
    engine.initialize(48000.0f);
    
    ParameterEventQueue* queue = engine.getParameterEventQueue();
    
    // Update envelope parameters
    queue->addEvent("attack", 10.0f, 0);
    queue->addEvent("decay", 200.0f, 0);
    queue->addEvent("sustain", 50.0f, 0);
    queue->addEvent("release", 50.0f, 0);
    queue->addEvent("pitchEnvelopeDepth", 1000.0f, 0);
    
    std::vector<float> buffer(512, 0.0f);
    engine.processBlock(buffer.data(), 512, 1);
    
    ParameterManager* paramManager = engine.getParameterManager();
    assert(std::abs(paramManager->getParameterValue("attack") - 10.0f) < 0.001f);
    assert(std::abs(paramManager->getParameterValue("decay") - 200.0f) < 0.001f);
    assert(std::abs(paramManager->getParameterValue("sustain") - 50.0f) < 0.001f);
    assert(std::abs(paramManager->getParameterValue("release") - 50.0f) < 0.001f);
    assert(std::abs(paramManager->getParameterValue("pitchEnvelopeDepth") - 1000.0f) < 0.001f);
    
    std::cout << "  ✓ Envelope parameters updated correctly" << std::endl;
    std::cout << "Envelope parameter update tests passed!" << std::endl << std::endl;
}

void testEffectsParameterUpdates() {
    std::cout << "Testing effects parameter updates..." << std::endl;
    
    AudioEngine engine;
    engine.initialize(48000.0f);
    
    ParameterEventQueue* queue = engine.getParameterEventQueue();
    
    // Update effects parameters
    queue->addEvent("compressorThreshold", -20.0f, 0);
    queue->addEvent("compressorRatio", 8.0f, 0);
    queue->addEvent("reverbRoomSize", 75.0f, 0);
    queue->addEvent("reverbMix", 30.0f, 0);
    queue->addEvent("masterLevel", 90.0f, 0);
    
    std::vector<float> buffer(512, 0.0f);
    engine.processBlock(buffer.data(), 512, 1);
    
    ParameterManager* paramManager = engine.getParameterManager();
    assert(std::abs(paramManager->getParameterValue("compressorThreshold") - (-20.0f)) < 0.001f);
    assert(std::abs(paramManager->getParameterValue("compressorRatio") - 8.0f) < 0.001f);
    assert(std::abs(paramManager->getParameterValue("reverbRoomSize") - 75.0f) < 0.001f);
    assert(std::abs(paramManager->getParameterValue("reverbMix") - 30.0f) < 0.001f);
    assert(std::abs(paramManager->getParameterValue("masterLevel") - 90.0f) < 0.001f);
    
    std::cout << "  ✓ Effects parameters updated correctly" << std::endl;
    std::cout << "Effects parameter update tests passed!" << std::endl << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "Sample-Accurate Parameter Update Tests" << std::endl;
    std::cout << "========================================" << std::endl << std::endl;
    
    try {
        testParameterEventQueue();
        testAudioEngineParameterUpdates();
        testSampleAccurateProcessing();
        testEnvelopeParameterUpdates();
        testEffectsParameterUpdates();
        
        std::cout << "========================================" << std::endl;
        std::cout << "All tests passed!" << std::endl;
        std::cout << "========================================" << std::endl;
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
