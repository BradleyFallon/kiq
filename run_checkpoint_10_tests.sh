#!/bin/bash

# Checkpoint 10: Run all effects and safety tests
# This script runs:
# - Effects tests: Compressor, Reverb, EffectsChain
# - Safety tests: DSPUtils, AudioEngine (master output safety)

echo "=========================================="
echo "  CHECKPOINT 10: Effects and Safety Tests"
echo "=========================================="
echo ""

# Track test results
TESTS_PASSED=0
TESTS_FAILED=0
FAILED_TESTS=()

# Function to run a test script
run_test() {
    local test_name=$1
    local test_script=$2
    
    echo "----------------------------------------"
    echo "Running: $test_name"
    echo "----------------------------------------"
    
    if [ -f "$test_script" ]; then
        bash "$test_script"
        if [ $? -eq 0 ]; then
            echo "✓ $test_name PASSED"
            ((TESTS_PASSED++))
        else
            echo "✗ $test_name FAILED"
            ((TESTS_FAILED++))
            FAILED_TESTS+=("$test_name")
        fi
    else
        echo "✗ Test script not found: $test_script"
        ((TESTS_FAILED++))
        FAILED_TESTS+=("$test_name (script not found)")
    fi
    echo ""
}

# Run Effects Tests
echo "=== EFFECTS TESTS ==="
echo ""

run_test "Compressor" "./test_compressor_compile.sh"
run_test "Reverb" "./test_reverb_compile.sh"
run_test "Effects Chain" "./test_effects_chain_compile.sh"

# Run Safety Tests
echo "=== SAFETY TESTS ==="
echo ""

run_test "DSP Utils (Soft Clipping & NaN Detection)" "./test_dsp_utils.sh"
run_test "Audio Engine (Master Output Safety)" "./test_audio_engine_master.sh"

# Summary
echo "=========================================="
echo "  TEST SUMMARY"
echo "=========================================="
echo ""
echo "Tests Passed: $TESTS_PASSED"
echo "Tests Failed: $TESTS_FAILED"
echo ""

if [ $TESTS_FAILED -eq 0 ]; then
    echo "✓✓✓ ALL TESTS PASSED! ✓✓✓"
    echo ""
    echo "Checkpoint 10 complete: All effects and safety tests are passing."
    exit 0
else
    echo "✗✗✗ SOME TESTS FAILED ✗✗✗"
    echo ""
    echo "Failed tests:"
    for test in "${FAILED_TESTS[@]}"; do
        echo "  - $test"
    done
    echo ""
    echo "Please review the output above for details."
    exit 1
fi
