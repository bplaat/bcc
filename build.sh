#!/bin/sh
# Build and run test suite for now only works on macOS

# Compile bcc to both targets on macos
clang --target=x86_64-macos -Wall -Wextra -Wpedantic --std=c11 -Iinclude $(find src -name *.c) -o bcc-x86_64 || exit
clang --target=arm64-macos -Wall -Wextra -Wpedantic --std=c11 -Iinclude $(find src -name *.c) -o bcc-arm64 || exit

# Function that runs a test
assert() {
    expected=$1
    input=$2

    # Compile and run for x86_64
    echo $input | ./bcc-x86_64 -
    actual=$?
    if [[ $actual != $expected ]]; then
        echo "[FAIL] Program:"
        echo $input
        echo "Dump:"
        echo $input | ./bcc-x86_64 -d -
        echo "Arch: x86_64 | Return: $actual | Correct: $expected"
        exit 1
    fi

    # Compile and run for arm64
    echo $input | ./bcc-arm64 -
    actual=$?
    if [[ $actual != $expected ]]; then
        echo "[FAIL] Program:"
        echo $input
        echo "Dump:"
        echo $input | ./bcc-arm64 -d -
        echo "Arch: arm64 | Return: $actual | Correct: $expected"
        exit 1
    fi
}

# Tests
assert 0 "0;"
assert 42 "42;"
assert 7 "4 + 3;"
assert 5 "8 - 3;"
assert 5 "4 + 3 - 2;"
assert 8 "4 + 4 - 2 + 2;"
assert 12 "4 \* 3;"
assert 2 "14 - 4 \* 3;"
assert 4 "12 / 3;"
assert 2 "12 % 5;"
assert 5 "12 % 5 + 3;"
assert 10 "12 - 12 % 5;"
assert 30 "(10 + 5) \* 2;"
assert 4 "20 / (10 - 5);"
assert 5 "(5 + 5) / (4 - 2);"
assert 10 "+5 + +5;"
assert 20 "--20;"
assert 10 "---20 + +30;"
assert 1 "1000000 == 1000000;"
assert 1 "1000000 != 1000001;"
assert 0 "5 > 6;"
assert 0 "6 < 5;"
assert 0 "5 != (3 + 2);"
assert 1 "10 < 5 \* 3;"
assert 1 "0x0f == 15;"
assert 1 "0x0f - 011 == 6;"
assert 1 "0b1001 == 8 + 1;"
assert 6 "{ 4;5;6; }"
assert 5 "{ ;;45;;6;10 - 5; }"
assert 6 "{ ~~6; }"
assert 0 "{ !!!1; }"
assert 1 "{ !(5 == 4); }"
assert 44 "3245 & 44;"
assert 33 "33 | 1;"
assert 46 "33 ^ 15;"
assert 20 "5 << 2;"
assert 4 "(20 >> 2) - 1;"
assert 0 "0 && 1;"
assert 1 "(20 > 5) && 1;"
assert 1 "0 || 1;"
assert 0 "0 || 0;"
assert 15 "{a = 10; a + 5; }"
assert 14 "{a = 10, b = 4; a + b; }"
assert 2 "{a = 10, b = 4; a - (b \* 2); }"
assert 6 "{ 5; return 6; 7; 8; }"
assert 30 "{ a = 5; return a \* 6; b = 6; }"

echo "[OK] All tests pass"
