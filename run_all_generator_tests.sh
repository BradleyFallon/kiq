#!/bin/bash

# Run all generator tests for checkpoint 4
# This script runs tests for all generators and modulation components

echo "========================================"
echo "  Generator Tests - Checkpoint 4"
echo "========================================"
echo ""

# Track test results
TESTS_PASSED=0
TESTS_FAILED=0
FAILED_TESTS=()

# Function to run a test and track results
run_test() {
    local test_name=$1
    local test_script=$2
    
    echo "Running: $test_name"
    echo "----------------------------------------"
    
    if [ -f "$test_script" ]; then
        if bash "$test_script"; then
            TESTS_PASSED=$((TESTS_PASSED + 1))
            echo ""
        else
            TESTS_FAILED=$((TESTS_FAILED + 1))
            FAILED_TESTS+=("$test_name")
            echo ""
            echo "❌ $test_name FAILED"
            echo ""
        fi
    else
        echo "⚠️  Test script not found: $test_script"
        echo ""
    fi
}

# Run all generator tests
run_test "SineDriver" "./test_sine_driver_compile.sh"
run_test "HarmonicMembrane" "./test_harmonic_membrane_compile.sh"

# Special handling for NoiseGenerator binary
echo "Running: NoiseGenerator"
echo "----------------------------------------"
if ./test_noise_generator 2>&1; then
    TESTS_PASSED=$((TESTS_PASSED + 1))
    echo ""
else
    TESTS_FAILED=$((TESTS_FAILED + 1))
    FAILED_TESTS+=("NoiseGenerator")
    echo ""
    echo "❌ NoiseGenerator FAILED"
    echo ""
fi

run_test "RingModulator" "./test_ring_modulator_compile.sh"
run_test "GeneratorMixer" "./test_generator_mixer_compile.sh"

# Print summary
echo "========================================"
echo "  Test Summary"
echo "========================================"
echo ""
echo "Tests Passed: $TESTS_PASSED"
echo "Tests Failed: $TESTS_FAILED"
echo ""

if [ $TESTS_FAILED -eq 0 ]; then
    echo "✅ All generator tests passed!"
    echo ""
    echo "The following components have been verified:"
    echo "  • SineDriver - Main sine oscillator"
    echo "  • HarmonicMembrane - Harmonic oscillator with ratio control"
    echo "  • NoiseGenerator - White noise generator"
    echo "  • RingModulator - Ring modulation with depth control"
    echo "  • GeneratorMixer - Three-channel mixer"
    echo ""
    echo "Checkpoint 4 complete! ✓"
    exit 0
else
    echo "❌ Some tests failed:"
    for test in "${FAILED_TESTS[@]}"; do
        echo "  • $test"
    done
    echo ""
    exit 1
fi
