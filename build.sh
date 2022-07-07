#!/bin/bash
# Build and test script for the Bassie C Compiler
# ./build.sh # Compile and run selected tests in this file
# ./build.sh format # Run the clang formatter over the whole code base
# ./build.sh debug # Compile and run selected tests via lldb
# ./build.sh test # Compile and run selected tests on ARM64 and x86_64
# ./build.sh test all # Compile and run all tests on ARM64 and x86_64

if [[ $1 = "format" ]]; then
    clang-format -i $(find . -name *.h -o -name *.c)
    exit
fi

if [[ $1 == "debug" ]]; then
    debug="debug"
fi
if [[ $1 == "test" ]]; then
    test="test"
fi

if [[ $debug = "debug" ]]; then
    gcc -g -Wall -Wextra -Wpedantic --std=c11 -Iinclude $(find src -name *.c) -o bcc
else
    gcc  -Ofast -Wall -Wextra -Wpedantic --std=c11 -Iinclude $(find src -name *.c) -o bcc
    rm -fr bcc.dSYM
fi

assert() {
    expected=$1
    input=$2

    # Compile for ARM64
cat <<EOF | gcc -xc -c -o b.o -
#include <stdint.h>
int64_t ret3(void) { return 3; }
int64_t ret5(void) { return 5; }
int64_t add(int64_t x, int64_t y) { return x+y; }
int64_t sub(int64_t x, int64_t y) { return x-y; }
int64_t add6(int64_t a, int64_t b, int64_t c, int64_t d, int64_t e, int64_t f) {
  return a+b+c+d+e+f;
}
EOF

    if [[ $debug = "debug" ]]; then
        lldb -- ./bcc "$input"
    else
        ./bcc "$input" || exit
    fi
    ./a.out
    actual=$?
    if [[ $actual = $expected ]]; then
        echo " arm64 ->  $actual == $expected OK"
    else
        echo " arm64 ->  $actual != $expected ERROR"
        exit 1
    fi

    # Compile for x86_64
cat <<EOF | arch -x86_64 gcc -xc -c -o b.o -
#include <stdint.h>
int64_t ret3(void) { return 3; }
int64_t ret5(void) { return 5; }
int64_t add(int64_t x, int64_t y) { return x+y; }
int64_t sub(int64_t x, int64_t y) { return x-y; }
int64_t add6(int64_t a, int64_t b, int64_t c, int64_t d, int64_t e, int64_t f) {
  return a+b+c+d+e+f;
}
EOF

    if [[ $test = "test" ]]; then
        ./bcc "$input" --arch=x86_64 || exit
        ./a.out
        actual=$?
        if [[ $actual = $expected ]]; then
            echo " x86_64 ->  $actual == $expected OK"
        else
            echo " x86_64 ->  $actual != $expected ERROR"
            exit 1
        fi
    fi
}

if [[ $2 = "all" ]]; then
    assert 0 'int main() { return 0; }'
    assert 42 'int main() { return 42; }'
    assert 21 'int main() { return 5+20-4; }'
    assert 41 'int main() { return  12 + 34 - 5 ; }'
    assert 47 'int main() { return 5+6*7; }'
    assert 15 'int main() { return 5*(9-6); }'
    assert 4 'int main() { return (3+5)/2; }'
    assert 10 'int main() { return -10+20; }'
    assert 10 'int main() { return - -10; }'
    assert 10 'int main() { return - - +10; }'

    assert 2 'int main() { return 56%6; }'
    assert 1 'int main() { return !(4 > 5); }'

    assert 0 'int main() { return 0==1; }'
    assert 1 'int main() { return 42==42; }'
    assert 1 'int main() { return 0!=1; }'
    assert 0 'int main() { return 42!=42; }'

    assert 1 'int main() { return 0<1; }'
    assert 0 'int main() { return 1<1; }'
    assert 0 'int main() { return 2<1; }'
    assert 1 'int main() { return 0<=1; }'
    assert 1 'int main() { return 1<=1; }'
    assert 0 'int main() { return 2<=1; }'

    assert 1 'int main() { return 1>0; }'
    assert 0 'int main() { return 1>1; }'
    assert 0 'int main() { return 1>2; }'
    assert 1 'int main() { return 1>=0; }'
    assert 1 'int main() { return 1>=1; }'
    assert 0 'int main() { return 1>=2; }'
fi

if [[ $2 = "all" || $2 = "locals" ]]; then
    assert 3 'int main() { int a; a=3; return a; }'
    assert 3 'int main() { int a=3; return a; }'
    assert 8 'int main() { int a=3; int z=5; return a+z; }'

    assert 3 'int main() { int a=3; return a; }'
    assert 8 'int main() { int a=3; int z=5; return a+z; }'
    assert 6 'int main() { int a; int b; a=b=3; return a+b; }'
    assert 3 'int main() { int foo=3; return foo; }'
    assert 8 'int main() { int foo123=3; int bar=5; return foo123+bar; }'

    assert 1 'int main() { return 1; 2; 3; }'
    assert 2 'int main() { 1; return 2; 3; }'
    assert 3 'int main() { 1; 2; return 3; }'

    assert 3 'int main() { {1; {2;} return 3;} }'
    assert 5 'int main() { ;;; return 5; }'
fi

if [[ $2 = "all" || $2 = "ifs" ]]; then
    assert 3 'int main() { if (0) return 2; return 3; }'
    assert 3 'int main() { if (1-1) return 2; return 3; }'
    assert 2 'int main() { if (1) return 2; return 3; }'
    assert 2 'int main() { if (2-1) return 2; return 3; }'
    assert 4 'int main() { if (0) { 1; 2; return 3; } else { return 4; } }'
    assert 3 'int main() { if (1) { 1; 2; return 3; } else { return 4; } }'
fi

if [[ $2 = "all" || $2 = "loops" ]]; then
    assert 10 'int main() { int i=0; while(i<10) i=i+1; return i; }'
    assert 55 'int main() { int i=0; int j=0; while(i<=10) {j=i+j; i=i+1;} return j; }'
    assert 3 'int main() { for (;;) {return 3;} return 5; }'
fi

if [[ $2 = "all" || $2 = "ptrs" ]]; then
    assert 3 'int main() { int x=3; return *&x; }'
    assert 3 'int main() { int x=3; int *y=&x; int **z=&y; return **z; }'
    assert 5 'int main() { int x=3; int y=5; return *(&x+1); }'
    assert 3 'int main() { int x=3; int y=5; return *(&y-1); }'
    assert 5 'int main() { int x=3; int y=5; return *(&x-(-1)); }'
    assert 5 'int main() { int x=3; int *y=&x; *y=5; return x; }'
    assert 7 'int main() { int x=3; int y=5; *(&x+1)=7; return y; }'
    assert 7 'int main() { int x=3; int y=5; *(&y-2+1)=7; return x; }'
    assert 5 'int main() { int x=3; return (&x+2)-&x+3; }'
    assert 8 'int main() { int x, y; x=3, y=5; return x+y; }'
    assert 8 'int main() { int x=3, y=5; return x+y; }'
fi

if [[ $2 = "all" || $2 = "funcs" ]]; then
    assert 3 'int main() { return ret3(); }'
    assert 5 'int main() { return ret5(); }'
    assert 8 'int main() { return add(3, 5); }'
    assert 2 'int main() { return sub(5, 3); }'
    assert 21 'int main() { return add6(1,2,3,4,5,6); }'
    assert 66 'int main() { return add6(1,2,add6(3,4,5,6,7,8),9,10,11); }'
    assert 136 'int main() { return add6(1,2,add6(3,add6(4,5,6,7,8,9),10,11,12,13),14,15,16); }'
fi

assert 33 'int main() { return ret32() + 1; } int ret32() { return 32; }'
