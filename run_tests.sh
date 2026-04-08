#!/bin/bash

# 1. Stay in current directory (where source files are)
cd "$(dirname "$0")"

# 2. Define variables
EXECUTABLE="./mysh"
TEST_DIR="./A3-test-cases"
OUTPUT_DIR="./my_results"
BACKING_STORE="./backing_store"

# Ensure the output directory exists
mkdir -p "$OUTPUT_DIR"

# Test cases with their specific compile parameters
declare -A TEST_PARAMS
TEST_PARAMS["tc1.txt"]="framesize=18 varmemsize=10"
TEST_PARAMS["tc2.txt"]="framesize=18 varmemsize=10"
TEST_PARAMS["tc3.txt"]="framesize=21 varmemsize=10"
TEST_PARAMS["tc4.txt"]="framesize=18 varmemsize=10"
TEST_PARAMS["tc5.txt"]="framesize=6 varmemsize=10"

passed=0
failed=0
total=0

echo "Starting COMP 310 Assignment 3 Test Suite..."
echo "-------------------------------------------"

# Initial cleanup
echo "Performing initial cleanup..."
make clean > /dev/null 2>&1
rm -f prog[0-9] prog[0-9][0-9] 2>/dev/null
rm -rf "$BACKING_STORE" 2>/dev/null

# Check if any test files exist
shopt -s nullglob
test_files=("$TEST_DIR"/tc*.txt)

if [ ${#test_files[@]} -eq 0 ]; then
    echo "ERROR: No test files found in $TEST_DIR"
    exit 1
fi

# Step 2: Loop through test files
for test_file in "${test_files[@]}"; do
    if [[ "$test_file" == *"_result"* ]]; then continue; fi

    total=$((total + 1))
    test_name=$(basename "$test_file")
    
    expected_result="${test_file%.txt}_result.txt"
    actual_output="$OUTPUT_DIR/${test_name%.txt}_actual.txt"
    compile_log="$OUTPUT_DIR/${test_name%.txt}_compile.log"

    # Get compile parameters for this test
    compile_params="${TEST_PARAMS[$test_name]}"
    
    if [ -z "$compile_params" ]; then
        echo "[ SKIP ] $test_name (no compile parameters defined)"
        total=$((total - 1))
        continue
    fi

    echo ""
    echo "=========================================="
    echo "Running $test_name"
    echo "=========================================="

    # Clean up everything before starting
    echo "Cleaning..."
    make clean > /dev/null 2>&1
    rm -f prog[0-9] prog[0-9][0-9] 2>/dev/null
    rm -rf "$BACKING_STORE" 2>/dev/null

    # Recompile with appropriate parameters
    echo "Compiling with $compile_params..."
    make mysh $compile_params > "$compile_log" 2>&1
    COMPILE_STATUS=$?
    
    if [ $COMPILE_STATUS -ne 0 ]; then
        echo "[ FAIL ] $test_name (compilation failed)"
        echo "See compilation errors in: $compile_log"
        cat "$compile_log"
        failed=$((failed + 1))
        continue
    fi

    # Verify executable was created
    if [ ! -f "$EXECUTABLE" ]; then
        echo "[ FAIL ] $test_name (executable mysh not found after compilation)"
        failed=$((failed + 1))
        continue
    fi

    # Copy program files from test directory
    echo "Copying test programs..."
    cp "$TEST_DIR"/prog* . 2>/dev/null
    
    # Show what we're about to run
    echo "Running: $EXECUTABLE < $test_file"

    # Run the shell in batch mode
    "$EXECUTABLE" < "$test_file" > "$actual_output" 2>&1
    EXIT_CODE=$?

    if [ $EXIT_CODE -ne 0 ]; then
        echo "[ FAIL ] $test_name (program crashed with exit code $EXIT_CODE)"
        echo "Output before crash:"
        head -20 "$actual_output"
        failed=$((failed + 1))
        continue
    fi

    MATCH=0

    # Compare outputs (whitespace and case insensitive)
    if [ ! -f "$expected_result" ]; then
        echo "[ FAIL ] $test_name (expected result file not found: $expected_result)"
        failed=$((failed + 1))
        continue
    fi

    diff -wbB "$actual_output" "$expected_result" > /dev/null 2>&1 && MATCH=1

    if [ $MATCH -eq 1 ]; then
        # TEST PASSED: Delete the output file to keep folder clean
        rm -f "$actual_output"
        rm -f "$compile_log"
        echo "[ PASS ] $test_name"
        passed=$((passed + 1))
    else
        # TEST FAILED: Keep the file for manual inspection
        echo "[ FAIL ] $test_name (output mismatch)"
        echo ""
        echo "Expected output (first 20 lines):"
        head -20 "$expected_result"
        echo ""
        echo "Actual output (first 20 lines):"
        head -20 "$actual_output"
        echo ""
        echo "Full diff saved. To see differences, run:"
        echo "  diff -u $expected_result $actual_output"
        failed=$((failed + 1))
    fi
done

# Final cleanup
echo ""
echo "Performing final cleanup..."
rm -f prog[0-9] prog[0-9][0-9] 2>/dev/null
rm -rf "$BACKING_STORE" 2>/dev/null

# Step 3: Remove the results folder if it is empty (all tests passed)
if [ -z "$(ls -A "$OUTPUT_DIR" 2>/dev/null)" ]; then
    rmdir "$OUTPUT_DIR"
fi

echo ""
echo "==========================================="
echo "Test Summary:"
echo "==========================================="
echo "Total Tests Run: $total"
echo "Tests Passed:    $passed"
echo "Tests Failed:    $failed"
echo ""

score=$((passed * 16))
echo "Estimated Score: $score / 80"
echo ""

if [ $failed -gt 0 ]; then
    echo "Failed test outputs are in: $OUTPUT_DIR"
fi