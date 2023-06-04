#!/bin/sh
# ./build.sh      | Build bcc
# ./build.sh test | Build and run tests
# For now only works on macOS and Linux

if [ "$(uname -s)" = Darwin ]; then
    clang --target=x86_64-macos -Wall -Wextra -Wpedantic --std=c11 -Iinclude $(find src -name *.c) -o bcc-x86_64 || exit
    if [ "$(arch)" = arm64 ]; then
        clang --target=arm64-macos -Wall -Wextra -Wpedantic --std=c11 -Iinclude $(find src -name *.c) -o bcc-arm64 || exit
    fi
fi
if [ "$(uname -s)" = Linux ]; then
    gcc -Wall -Wextra -Wpedantic --std=gnu11 -Iinclude $(find src -name *.c) -lm -o bcc-x86_64 || exit
fi

# Function that runs a test
assert() {
    expected=$1
    input=$2

    # Compile and run for x86_64
    if [ -e "./bcc-x86_64" ]; then
        echo "$input" | ./bcc-x86_64 -
        actual=$?
        if [ $actual != $expected ]; then
            echo "[FAIL] Program:"
            echo $input
            echo "Dump:"
            echo $input | ./bcc-x86_64 -d -
            echo "Arch: x86_64 | Return: $actual | Correct: $expected"
            exit 1
        fi
    fi

    # Compile and run for arm64
    if [ -e "./bcc-arm64" ]; then
        echo "$input" | ./bcc-arm64 -
        actual=$?
        if [ $actual != $expected ]; then
            echo "[FAIL] Program:"
            echo $input
            echo "Dump:"
            echo $input | ./bcc-arm64 -d -
            echo "Arch: arm64 | Return: $actual | Correct: $expected"
            exit 1
        fi
    fi
}

# Tests
if [ "$1" = "test" ]; then
    assert 0 "int main() { return 0; }"
    assert 42 "int main() { return 42; }"
    assert 7 "int main() { return 4 + 3; }"
    assert 5 "int main() { return 8 - 3; }"
    assert 5 "int main() { return 4 + 3 - 2; }"
    assert 8 "int main() { return 4 + 4 - 2 + 2; }"
    assert 12 "int main() { return 4 * 3; }"
    assert 2 "int main() { return 14 - 4 * 3; }"
    assert 4 "int main() { return 12 / 3; }"
    assert 2 "int main() { return 12 % 5; }"
    assert 5 "int main() { return 12 % 5 + 3; }"
    assert 10 "int main() { return 12 - 12 % 5; }"
    assert 30 "int main() { return (10 + 5) * 2; }"
    assert 4 "int main() { return 20 / (10 - 5); }"
    assert 5 "int main() { return (5 + 5) / (4 - 2); }"
    assert 10 "int main() { return +5 + +5; }"
    assert 20 "int main() { return --20; }"
    assert 10 "int main() { return ---20 + +30; }"
    assert 1 "int main() { return 1000000 == 1000000; }"
    assert 1 "int main() { return 1000000 != 1000001; }"
    assert 0 "int main() { return 5 > 6; }"
    assert 0 "int main() { return 6 < 5; }"
    assert 0 "int main() { return 5 != (3 + 2); }"
    assert 1 "int main() { return 10 < 5 * 3; }"
    assert 1 "int main() { return 0x0f == 15; }"
    assert 1 "int main() { return 0x0f - 011 == 6; }"
    assert 1 "int main() { return 0b1001 == 8 + 1; }"
    assert 5 "int main() { 4;return 5;6; }"
    assert 5 "int main() { ;;45;;6; return 10 - 5; }"
    assert 6 "int main() { return ~~6; }"
    assert 0 "int main() { return !!!1; }"
    assert 1 "int main() { return !(5 == 4); }"
    assert 44 "int main() { return 3245 & 44; }"
    assert 33 "int main() { return 33 | 1; }"
    assert 46 "int main() { return 33 ^ 15; }"
    assert 20 "int main() { return 5 << 2; }"
    assert 4 "int main() { return (20 >> 2) - 1; }"
    assert 0 "int main() { return 0 && 1; }"
    assert 1 "int main() { return (20 > 5) && 1; }"
    assert 1 "int main() { return 0 || 1; }"
    assert 0 "int main() { return 0 || 0; }"

    assert 15 "int main() { int a; a = 10; return a + 5; }"
    assert 15 "int main() { int a; a = 10; a += 5; return a; 45; }"
    assert 5 "int main() { int a; a = 10; a -= 5; return a; 45; }"
    assert 50 "int main() { int a; a = 10; a *= 5; return a; 56; }"
    assert 2 "int main() { int a; a = 10; a /= 5; return a; 45; }"
    assert 2 "int main() { int a; a = 10; a %= 4; return a; 34; }"
    assert 2 "int main() { int a; a = 10; a &= 3; return a; 45; }"
    assert 11 "int main() { int a; a = 10; a |= 3; return a; 465; }"
    assert 9 "int main() { int a; a = 10; a ^= 3; return a; 123; }"
    assert 80 "int main() { int a; a = 10; a <<= 3; return a; 123; }"
    assert 2 "int main() { int a; a = 10; a >>= 2; return a; 123; }"
    assert 14 "int main() { int a, b; a = 10, b = 4; return a + b; }"
    assert 2 "int main() { int a, b; a = 10, b = 4; return a - (b * 2); }"
    assert 10 "int main() { int a, b; a = b = 5; return a + b; }"
    assert 27 "int main() { int a, b, c; a = b = c = 5; c = 2; return a * b + c; }"
    assert 30 "int main() { int a, b; a = 5; return a * 6; b = 6; }"

    assert 10 "int main() { if (5 > 4) return 10; else return 5; }"
    assert 5 "int main() { if (5 < 4) return 10; else return 5; }"
    assert 2 "int main() { int a = 4; if (a > 4) { return 1; } else if (a == 4) { return 2; } else return 3; }"
    assert 16 "int main() { int a = 4; if (a == 4) { return 4 * 4; } else return 4 + (8 - 4); }"
    assert 5 "int main() { int i = 0; while (i < 5) i += 1; return i; }"
    assert 5 "int main() { int i = 0; do i += 1; while (i < 5); return i; }"
    assert 5 "int main() { int i; for (i = 0; i < 10; i += 1) { if (i == 5) return i; } }"
    assert 10 "int main() { int i = 0; for (; i < 10; i += 1); return i; }"
    assert 8 "int main() { int i = 0; for (;i < 8;) i += 1; return i; }"
    assert 4 "int main() { int i = 0; for (;;) { i += 1; if (i == 4) return i; } return 45; }"
    assert 2 "int main() { int i = 7; while (i > 2) { i = i - 1; } return i; }"
    assert 16 "int main() { int x, y, q, i, j; x = 4, y = 4, q = 0; for (i = 0; i < y; i += 1)\n for (j = 0; j < x; j += 1)\n q += 1; return q; }"

    assert 10 "int main() { int x = 5; int *y = &x; *y = 10; return x; }"
    assert 3 "int main() { int x=3; return *&x; }"
    assert 3 "int main() { int x=3; int *y=&x; int **z=&y; return **z; }"
    assert 5 "int main() { int x=3; int y=5; return *(&x+1); }"
    assert 3 "int main() { int x=3; int y=5; return *(&y-1); }"
    assert 5 "int main() { int x=3; int y=5; return *(&x-(-1)); }"
    assert 5 "int main() { int x=3; int *y=&x; *y=5; return x; }"
    assert 7 "int main() { int x=3; int y=5; *(&x+1)=7; return y; }"
    assert 7 "int main() { int x=3; int y=5; *(&y-2+1)=7; return x; }"
    assert 5 "int main() { int x=3; return (&x+2)-&x+3; }"

    assert 3 "int main() { int x[2]; int *y=&x; *y=3; return *x; }"
    assert 3 "int main() { int x[3]; *x=3; *(x+1)=4; *(x+2)=5; return *x; }"
    assert 4 "int main() { int x[3]; *x=3; *(x+1)=4; *(x+2)=5; return *(x+1); }"
    assert 5 "int main() { int x[3]; *x=3; *(x+1)=4; *(x+2)=5; return *(x+2); }"

    assert 0 "int main() { int x[2][3]; int *y=x; *y=0; return **x; }"
    assert 1 "int main() { int x[2][3]; int *y=x; *(y+1)=1; return *(*x+1); }"
    assert 2 "int main() { int x[2][3]; int *y=x; *(y+2)=2; return *(*x+2); }"
    assert 3 "int main() { int x[2][3]; int *y=x; *(y+3)=3; return **(x+1); }"
    assert 4 "int main() { int x[2][3]; int *y=x; *(y+4)=4; return *(*(x+1)+1); }"
    assert 5 "int main() { int x[2][3]; int *y=x; *(y+5)=5; return *(*(x+1)+2); }"

    assert 8 "int main() { int x=3; return sizeof x; }"
    assert 4 "int main() { int x=3; return sizeof(x) - 4; }"
    assert 12 "int main() { int *x=3; return sizeof x + 4; }"
    assert 32 "int main() { int x[4]; return sizeof x; }"
    assert 32 "int main() { int *x[4]; return sizeof x; }"
    assert 64 "int main() { int x[4][2]; return sizeof x; }"
    assert 6 "int main() { int x = 10; return x > 5 ? 6 : 7; }"
    assert 32 "int main() { int x = 7 < 4 ? 45 : 32; return x; }"

    assert 3 "int main() { int x[3]; *x=3; x[1]=4; x[2]=5; return *x; }"
    assert 4 "int main() { int x[3]; *x=3; x[1]=4; x[2]=5; return *(x+1); }"
    assert 5 "int main() { int x[3]; *x=3; x[1]=4; x[2]=5; return *(x+2); }"
    assert 5 "int main() { int x[3]; *x=3; x[1]=4; x[2]=5; return *(x+2); }"
    assert 5 "int main() { int x[3]; *x=3; x[1]=4; 2[x]=5; return *(x+2); }"

    assert 0 "int main() { int x[2][3]; int *y=x; y[0]=0; return x[0][0]; }"
    assert 1 "int main() { int x[2][3]; int *y=x; y[1]=1; return x[0][1]; }"
    assert 2 "int main() { int x[2][3]; int *y=x; y[2]=2; return x[0][2]; }"
    assert 3 "int main() { int x[2][3]; int *y=x; y[3]=3; return x[1][0]; }"
    assert 4 "int main() { int x[2][3]; int *y=x; y[4]=4; return x[1][1]; }"
    assert 5 "int main() { int x[2][3]; int *y=x; y[5]=5; return x[1][2]; }"

    assert 10 "int a() { return 4; } int b() { return 4; } int main() { return 10; }"

    echo "[OK] All tests pass"
fi
