#!/bin/bash

# Comprehensive Voice Tests Runner
# This script runs all voice-related tests to verify Task 7

set -e

echo "=========================================="
echo "  Voice Tests - Task 7 Checkpoint"
echo "=========================================="
echo ""

# Track test results
TESTS_PASSED=0
TESTS_FAILED=0

# Function to run a test
run_test() {
    local test_name=$1
    local test_script=$2
    
    echo "Running: $test_name"
    echo "----------------------------------------"
    
    if bash "$test_script"; then
        echo "✓ $test_name PASSED"
        TESTS_PASSED=$((TESTS_PASSED + 1))
    else
        echo "✗ $test_name FAILED"
        TESTS_FAILED=$((TESTS_FAILED + 1))
    fi
    
    echo ""
}

# Run Voice tests
run_test "Voice Implementation Tests" "test_voice_compile.sh"

# Run VoiceAllocator tests
run_test "VoiceAllocator Implementation Tests" "test_voice_allocator_compile.sh"

# Summary
echo "=========================================="
echo "  Test Summary"
echo "=========================================="
echo "Tests Passed: $TESTS_PASSED"
echo "Tests Failed: $TESTS_FAILED"
echo ""

if [ $TESTS_FAILED -eq 0 ]; then
    echo "✓ All voice tests passed!"
    echo ""
    echo "Task 7 Checkpoint: COMPLETE"
    echo "All voice-related tests are passing."
    exit 0
else
    echo "✗ Some tests failed"
    echo ""
    echo "Task 7 Checkpoint: INCOMPLETE"
    echo "Please review and fix failing tests."
    exit 1
fi
