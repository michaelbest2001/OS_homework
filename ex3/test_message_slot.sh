#!/bin/bash

# Constants
MODULE="message_slot"
DEVICE_PATH="/dev/slot"
MAJOR=235
MINOR=0
CHANNEL=1
MESSAGE="Hello, World!"
LONG_MESSAGE=$(head -c 129 < /dev/zero | tr '\0' 'A') # 129 bytes long
INVALID_CHANNEL=0
BUF_LEN=128

# Arrays to store test results
PASSED_TESTS=()
FAILED_TESTS=()

# Function to load the module
load_module() {
    sudo insmod ${MODULE}.ko || { echo "Failed to load module"; exit 1; }
    echo "Module loaded successfully"
}

# Function to unload the module
unload_module() {
    sudo rmmod ${MODULE} || { echo "Failed to unload module"; exit 1; }
    echo "Module unloaded successfully"
}

# Function to create a device file
create_device() {
    local minor=$1
    sudo mknod ${DEVICE_PATH}${minor} c ${MAJOR} ${minor} || { echo "Failed to create device file"; exit 1; }
    sudo chmod 666 ${DEVICE_PATH}${minor}
    echo "Device file created: ${DEVICE_PATH}${minor}"
}

# Function to remove a device file
remove_device() {
    local minor=$1
    sudo rm -f ${DEVICE_PATH}${minor} || { echo "Failed to remove device file"; exit 1; }
    echo "Device file removed: ${DEVICE_PATH}${minor}"
}

# Function to test message sender
test_message_sender() {
    local path=$1
    local channel=$2
    local message=$3
    local test_name=$4

    echo "Testing message_sender with path: $path, channel: $channel, message: $message"
    ./message_sender $path $channel "$message"
    if [ $? -ne 0 ]; then
        echo "Test failed: $test_name"
        FAILED_TESTS+=("$test_name")
        return 1
    else
        PASSED_TESTS+=("$test_name")
    fi
    return 0
}

# Function to test message reader
test_message_reader() {
    local path=$1
    local channel=$2
    local expected_message=$3
    local test_name=$4

    echo "Testing message_reader with path: $path, channel: $channel"
    output=$(./message_reader $path $channel)
    if [ $? -ne 0 ]; then
        echo "Test failed: $test_name"
        FAILED_TESTS+=("$test_name")
        return 1
    fi
    if [ "$output" != "$expected_message" ]; then
        echo "Test failed: $test_name - expected \"$expected_message\", got \"$output\""
        FAILED_TESTS+=("$test_name")
        return 1
    else
        PASSED_TESTS+=("$test_name")
    fi
    return 0
}

# Load the module and create device files
load_module
create_device ${MINOR}
create_device $((${MINOR} + 1))

# Test 1: Write and read a message
test_message_sender ${DEVICE_PATH}${MINOR} ${CHANNEL} "${MESSAGE}" "Write and read a message"
test_message_reader ${DEVICE_PATH}${MINOR} ${CHANNEL} "${MESSAGE}" "Write and read a message"

# Test 2: Write and read from another channel
test_message_sender ${DEVICE_PATH}${MINOR} $((${CHANNEL} + 1)) "Another message" "Write and read from another channel"
test_message_reader ${DEVICE_PATH}${MINOR} $((${CHANNEL} + 1)) "Another message" "Write and read from another channel"

# Test 3: Write and read from a different device
test_message_sender ${DEVICE_PATH}$((${MINOR} + 1)) ${CHANNEL} "Different device" "Write and read from a different device"
test_message_reader ${DEVICE_PATH}$((${MINOR} + 1)) ${CHANNEL} "Different device" "Write and read from a different device"

# Test 4: Overwrite a message and read
test_message_sender ${DEVICE_PATH}${MINOR} ${CHANNEL} "New message" "Overwrite a message and read"
test_message_reader ${DEVICE_PATH}${MINOR} ${CHANNEL} "New message" "Overwrite a message and read"

# Test 5: Read without writing (expect EWOULDBLOCK)
output=$(./message_reader ${DEVICE_PATH}$((${MINOR} + 1)) $((${CHANNEL} + 2)))
if [ $? -eq 0 ]; then
    echo "Test failed: Read without writing (expect EWOULDBLOCK)"
    FAILED_TESTS+=("Read without writing (expect EWOULDBLOCK)")
else
    echo "Test passed: Read without writing (expect EWOULDBLOCK)"
    PASSED_TESTS+=("Read without writing (expect EWOULDBLOCK)")
fi

# Test 6: Write a long message (expect EMSGSIZE)
output=$(./message_sender ${DEVICE_PATH}${MINOR} ${CHANNEL} "${LONG_MESSAGE}")
if [ $? -eq 0 ]; then
    echo "Test failed: Write a long message (expect EMSGSIZE)"
    FAILED_TESTS+=("Write a long message (expect EMSGSIZE)")
else
    echo "Test passed: Write a long message (expect EMSGSIZE)"
    PASSED_TESTS+=("Write a long message (expect EMSGSIZE)")
fi

# Test 7: Write with invalid channel (expect EINVAL)
output=$(./message_sender ${DEVICE_PATH}${MINOR} ${INVALID_CHANNEL} "${MESSAGE}")
if [ $? -eq 0 ]; then
    echo "Test failed: Write with invalid channel (expect EINVAL)"
    FAILED_TESTS+=("Write with invalid channel (expect EINVAL)")
else
    echo "Test passed: Write with invalid channel (expect EINVAL)"
    PASSED_TESTS+=("Write with invalid channel (expect EINVAL)")
fi

# Test 8: Read with invalid channel (expect EINVAL)
output=$(./message_reader ${DEVICE_PATH}${MINOR} ${INVALID_CHANNEL})
if [ $? -eq 0 ]; then
    echo "Test failed: Read with invalid channel (expect EINVAL)"
    FAILED_TESTS+=("Read with invalid channel (expect EINVAL)")
else
    echo "Test passed: Read with invalid channel (expect EINVAL)"
    PASSED_TESTS+=("Read with invalid channel (expect EINVAL)")
fi

# Test 9: Write with invalid device (expect ENODEV)
output=$(./message_sender ${DEVICE_PATH}$((${MINOR} + 2)) ${CHANNEL} "${MESSAGE}")
if [ $? -eq 0 ]; then
    echo "Test failed: Write with invalid device (expect ENODEV)"
    FAILED_TESTS+=("Write with invalid device (expect ENODEV)")
else
    echo "Test passed: Write with invalid device (expect ENODEV)"
    PASSED_TESTS+=("Write with invalid device (expect ENODEV)")
fi

# Test 10: Multiple writes and reads on different channels
test_message_sender ${DEVICE_PATH}${MINOR} ${CHANNEL} "Message 1" "Multiple writes and reads on different channels - Write 1"
test_message_sender ${DEVICE_PATH}${MINOR} $((${CHANNEL} + 2)) "Message 2" "Multiple writes and reads on different channels - Write 2"
test_message_reader ${DEVICE_PATH}${MINOR} ${CHANNEL} "Message 1" "Multiple writes and reads on different channels - Read 1"
test_message_reader ${DEVICE_PATH}${MINOR} $((${CHANNEL} + 2)) "Message 2" "Multiple writes and reads on different channels - Read 2"

# Test 11: Write and read with buffer length 0 (expect EINVAL)
output=$(./message_sender ${DEVICE_PATH}${MINOR} ${CHANNEL} "" 0)
if [ $? -eq 0 ]; then
    echo "Test failed: Write and read with buffer length 0 (expect EINVAL)"
    FAILED_TESTS+=("Write and read with buffer length 0 (expect EINVAL)")
else
    echo "Test passed: Write and read with buffer length 0 (expect EINVAL)"
    PASSED_TESTS+=("Write and read with buffer length 0 (expect EINVAL)")
fi
# Test 12: multiple writes and reads on the same channel
test_message_sender ${DEVICE_PATH}${MINOR} ${CHANNEL} "Message 3" "Multiple writes and reads on the same channel - Write 1"
test_message_sender ${DEVICE_PATH}${MINOR} ${CHANNEL} "Message 4" "Multiple writes and reads on the same channel - Write 2"
test_message_reader ${DEVICE_PATH}${MINOR} ${CHANNEL} "Message 4" "Multiple writes and reads on the same channel - Read 1"
test_message_reader ${DEVICE_PATH}${MINOR} ${CHANNEL} "Message 4" "Multiple writes and reads on the same channel - Read 2"

# Test 13: memory leak test with a lot of writes and reads with valgrind
for i in {1..1000}; do
    test_message_sender ${DEVICE_PATH}${MINOR} ${CHANNEL} "Message $i" "Memory leak test - Write $i"
    test_message_reader ${DEVICE_PATH}${MINOR} ${CHANNEL} "Message $i" "Memory leak test - Read $i"
    valgrind --leak-check=full ./message_sender ${DEVICE_PATH}${MINOR} ${CHANNEL} "Message $i"
    valgrind --leak-check=full ./message_reader ${DEVICE_PATH}${MINOR} ${CHANNEL}
done

# Clean up
remove_device ${MINOR}
remove_device $((${MINOR} + 1))
unload_module

# Print test summary
echo "Test Summary:"
echo "============="
echo "Passed Tests:"
for test in "${PASSED_TESTS[@]}"; do
    echo "- $test"
done

echo "Failed Tests:"
for test in "${FAILED_TESTS[@]}"; do
    echo "- $test"
done

if [ ${#FAILED_TESTS[@]} -eq 0 ]; then
    echo "All tests passed successfully!"
else
    echo "Some tests failed."
fi
