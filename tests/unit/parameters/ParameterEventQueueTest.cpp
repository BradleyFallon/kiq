#include <gtest/gtest.h>
#include "../../../src/audio_engine/parameters/ParameterEventQueue.h"
#include <thread>
#include <chrono>

using namespace KickDrum;

class ParameterEventQueueTest : public ::testing::Test {
protected:
    ParameterEventQueue queue;
};

// Test: Adding and retrieving events
TEST_F(ParameterEventQueueTest, AddAndRetrieveEvents) {
    // Add some events
    queue.addEvent("pitch0Hz", 220.0f, 0);
    queue.addEvent("outputGain", 0.8f, 100);
    queue.addEvent("airDecayMs", 7.0f, 50);
    
    EXPECT_EQ(queue.getEventCount(), 3);
    EXPECT_FALSE(queue.isEmpty());
    
    // Retrieve events
    std::vector<ParameterEvent> events;
    queue.getEventsForBuffer(events);
    
    // Queue should be empty after retrieval
    EXPECT_EQ(queue.getEventCount(), 0);
    EXPECT_TRUE(queue.isEmpty());
    
    // Events should be sorted by sample offset
    ASSERT_EQ(events.size(), 3);
    EXPECT_EQ(events[0].parameterId, "pitch0Hz");
    EXPECT_EQ(events[0].sampleOffset, 0);
    EXPECT_EQ(events[1].parameterId, "airDecayMs");
    EXPECT_EQ(events[1].sampleOffset, 50);
    EXPECT_EQ(events[2].parameterId, "outputGain");
    EXPECT_EQ(events[2].sampleOffset, 100);
}

// Test: Events are sorted by sample offset
TEST_F(ParameterEventQueueTest, EventsSortedBySampleOffset) {
    // Add events in random order
    queue.addEvent("param3", 3.0f, 300);
    queue.addEvent("param1", 1.0f, 100);
    queue.addEvent("param4", 4.0f, 400);
    queue.addEvent("param2", 2.0f, 200);
    
    std::vector<ParameterEvent> events;
    queue.getEventsForBuffer(events);
    
    // Should be sorted by sample offset
    ASSERT_EQ(events.size(), 4);
    EXPECT_EQ(events[0].sampleOffset, 100);
    EXPECT_EQ(events[1].sampleOffset, 200);
    EXPECT_EQ(events[2].sampleOffset, 300);
    EXPECT_EQ(events[3].sampleOffset, 400);
}

// Test: Clear queue
TEST_F(ParameterEventQueueTest, ClearQueue) {
    queue.addEvent("param1", 1.0f, 0);
    queue.addEvent("param2", 2.0f, 100);
    
    EXPECT_EQ(queue.getEventCount(), 2);
    
    queue.clear();
    
    EXPECT_EQ(queue.getEventCount(), 0);
    EXPECT_TRUE(queue.isEmpty());
}

// Test: Empty queue
TEST_F(ParameterEventQueueTest, EmptyQueue) {
    EXPECT_TRUE(queue.isEmpty());
    EXPECT_EQ(queue.getEventCount(), 0);
    
    std::vector<ParameterEvent> events;
    queue.getEventsForBuffer(events);
    
    EXPECT_TRUE(events.empty());
}

// Test: Multiple retrievals
TEST_F(ParameterEventQueueTest, MultipleRetrievals) {
    // First batch
    queue.addEvent("param1", 1.0f, 0);
    queue.addEvent("param2", 2.0f, 100);
    
    std::vector<ParameterEvent> events1;
    queue.getEventsForBuffer(events1);
    EXPECT_EQ(events1.size(), 2);
    EXPECT_TRUE(queue.isEmpty());
    
    // Second batch
    queue.addEvent("param3", 3.0f, 0);
    
    std::vector<ParameterEvent> events2;
    queue.getEventsForBuffer(events2);
    EXPECT_EQ(events2.size(), 1);
    EXPECT_TRUE(queue.isEmpty());
}

// Test: Thread safety (basic test)
TEST_F(ParameterEventQueueTest, ThreadSafety) {
    const int numEvents = 100;
    
    // Add events from multiple threads
    std::thread t1([this]() {
        for (int i = 0; i < numEvents; ++i) {
            queue.addEvent("param1", static_cast<float>(i), i);
        }
    });
    
    std::thread t2([this]() {
        for (int i = 0; i < numEvents; ++i) {
            queue.addEvent("param2", static_cast<float>(i), i);
        }
    });
    
    t1.join();
    t2.join();
    
    // Should have all events
    EXPECT_EQ(queue.getEventCount(), numEvents * 2);
    
    std::vector<ParameterEvent> events;
    queue.getEventsForBuffer(events);
    EXPECT_EQ(events.size(), numEvents * 2);
}

// Test: ParameterEvent construction
TEST(ParameterEventTest, Construction) {
    ParameterEvent event1("pitch0Hz", 220.0f, 100);
    EXPECT_EQ(event1.parameterId, "pitch0Hz");
    EXPECT_EQ(event1.value, 220.0f);
    EXPECT_EQ(event1.sampleOffset, 100);
    
    ParameterEvent event2("outputGain", 0.8f);
    EXPECT_EQ(event2.parameterId, "outputGain");
    EXPECT_EQ(event2.value, 0.8f);
    EXPECT_EQ(event2.sampleOffset, 0);  // Default offset
    
    ParameterEvent event3;
    EXPECT_EQ(event3.parameterId, "");
    EXPECT_EQ(event3.value, 0.0f);
    EXPECT_EQ(event3.sampleOffset, 0);
}

// Test: ParameterEvent comparison
TEST(ParameterEventTest, Comparison) {
    ParameterEvent event1("param1", 1.0f, 100);
    ParameterEvent event2("param2", 2.0f, 200);
    ParameterEvent event3("param3", 3.0f, 100);
    
    EXPECT_TRUE(event1 < event2);
    EXPECT_FALSE(event2 < event1);
    EXPECT_FALSE(event1 < event3);  // Same offset, not less than
}
