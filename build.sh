#!/bin/bash
# Build and test script for the Bassie C Compiler
# ./build.sh # Compile and run selected tests in this file
# ./build.sh format # Run the clang formatter over the whole code base
# ./build.sh debug # Compile and run selected tests via lldb
# ./build.sh # Compile and run selected tests on ARM64 and x86_64
# ./build.sh all # Compile and run all tests on ARM64 and x86_64

if [[ $1 = "format" ]]; then
    clang-format -i $(find . -name *.h -o -name *.c)
    exit
fi

if [[ $1 = "debug" ]]; then
    debug="debug"
fi

if [[ $debug = "debug" ]]; then
    gcc -g -Wall -Wextra -Wpedantic --std=c11 -Iinclude $(find src -name *.c) -o bcc || exit
else
    gcc -Ofast -Wall -Wextra -Wpedantic --std=c11 -Iinclude $(find src -name *.c) -o bcc || exit
    rm -fr bcc.dSYM
fi

# Compile shared libs
cat <<EOF | gcc -Os -xc -c -o lib-arm64.o -
#include <stdint.h>
int ret3(void) { return 3; }
int ret5(void) { return 5; }
int add(int x, int y) { return x+y; }
int sub(int x, int y) { return x-y; }
int add6(int a, int b, int c, int d, int e, int f) {
  return a+b+c+d+e+f;
}
EOF

cat <<EOF | arch -x86_64 gcc -Os -xc -c -o lib-x86_64.o -
#include <stdint.h>
int ret3(void) { return 3; }
int ret5(void) { return 5; }
int add(int x, int y) { return x+y; }
int sub(int x, int y) { return x-y; }
int add6(int a, int b, int c, int d, int e, int f) {
  return a+b+c+d+e+f;
}
EOF

assert() {
    expected=$1
    input=$2

    # Dump parsed node
    if [[ $debug = "debug" ]]; then
        lldb -- ./bcc -d "$input"
    else
        ./bcc -d "$input" || exit
    fi

    # Compile for x86_64
    if [[ $debug = "debug" ]]; then
        lldb -- ./bcc -arch x86_64 "$input" lib-x86_64.o -o test
    else
        ./bcc -arch x86_64 "$input" lib-x86_64.o -o test || exit
    fi
    ./test
    actual=$?
    if [[ $actual != $expected ]]; then
        echo "Program:"
        echo $input
        echo "Arch: x86_64 | Return: $actual | Correct: $expected"
        exit 1
    fi

    # Compile for arm64
    ./bcc "$input" lib-arm64.o -o test || exit
    ./test
    actual=$?
    if [[ $actual != $expected ]]; then
        echo "Program:"
        echo $input
        echo "Arch: arm64 | Return: $actual | Correct: $expected"
        exit 1
    fi

    echo
}

if [[ $1 = "all" || $1 = "basic"  ]]; then
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

    assert 1 'int main() { return !(4 > 5); }'
    assert 1 'int main() { return 5 > 4 && 3 > 2; }'
    assert 0 'int main() { return 5 > 4 && 3 < 2; }'
    assert 1 'int main() { return 5 > 4 || 3 > 2; }'
    assert 0 'int main() { return 5 < 4 || 3 < 2; }'

    assert 1 'int main() { return 1; 2; 3; }'
    assert 2 'int main() { 1; return 2; 3; }'
    assert 3 'int main() { 1; 2; return 3; }'
    assert 3 'int main() { {1; {2;} return 3;} }'
    assert 5 'int main() { ;;; return 5; }'
fi

if [[ $1 = "all" || $1 = "local" ]]; then
    assert 3 'int main() { int a; a=3; return a; }'
    assert 3 'int main() { int a=3; return a; }'
    assert 8 'int main() { int a=3; int z=5; return a+z; }'
    assert 3 'int main() { int a=3; return a; }'
    assert 8 'int main() { int a=3; int z=5; return a+z; }'
    assert 6 'int main() { int a; int b; a=b=3; return a+b; }'
    assert 3 'int main() { int foo=3; return foo; }'
    assert 8 'int main() { int foo123=3; int bar=5; return foo123+bar; }'
    assert 8 'int main() { int x, y; x=3, y=5; return x+y; }'
    assert 8 'int main() { int x=3, y=5; return x+y; }'
fi

if [[ $1 = "all" || $1 = "if" ]]; then
    assert 3 'int main() { if (0) return 2; return 3; }'
    assert 3 'int main() { if (1-1) return 2; return 3; }'
    assert 2 'int main() { if (1) return 2; return 3; }'
    assert 2 'int main() { if (2-1) return 2; return 3; }'
    assert 4 'int main() { if (0) { 1; 2; return 3; } else { return 4; } }'
    assert 3 'int main() { if (1) { 1; 2; return 3; } else { return 4; } }'
fi

if [[ $1 = "all" || $1 = "loop" ]]; then
    assert 10 'int main() { int i=0; while(i<10) i=i+1; return i; }'
    assert 55 'int main() { int i=0; int j=0; while(i<=10) {j=i+j; i=i+1;} return j; }'
    assert 3 'int main() { for (;;) {return 3;} return 5; }'
fi

if [[ $1 = "all" || $1 = "ptr" ]]; then
    assert 3 'int main() { int x=3; return *&x; }'
    assert 3 'int main() { int x=3; int *y=&x; int **z=&y; return **z; }'
    assert 5 'int main() { int x=3; int y=5; return *(&x+1); }'
    assert 3 'int main() { int x=3; int y=5; return *(&y-1); }'
    assert 5 'int main() { int x=3; int y=5; return *(&x-(-1)); }'
    assert 5 'int main() { int x=3; int *y=&x; *y=5; return x; }'
    assert 7 'int main() { int x=3; int y=5; *(&x+1)=7; return y; }'
    assert 7 'int main() { int x=3; int y=5; *(&y-2+1)=7; return x; }'
    assert 5 'int main() { int x=3; return (&x+2)-&x+3; }'
fi

if [[ $1 = "all" || $1 = "funccall" ]]; then
    assert 3 'int main() { return ret3(); }'
    assert 5 'int main() { return ret5(); }'
    assert 8 'int main() { return add(3, 5); }'
    assert 2 'int main() { return sub(5, 3); }'
    assert 21 'int main() { return add6(1,2,3,4,5,6); }'
    assert 15 'int main() { return add(add(1, 2), add(add(3, 4), 5)); }'
    assert 66 'int main() { return add6(1,2,add6(3,4,5,6,7,8),9,10,11); }'
    assert 136 'int main() { return add6(1,2,add6(3,add6(4,5,6,7,8,9),10,11,12,13),14,15,16); }'
fi

if [[ $1 = "all" || $1 = "funcdef" ]]; then
    assert 33 'int main() { return ret32() + 1; } int ret32() { return 32; }'
    assert 10 'int main() { return (a() + b()) / 3; } int a() { return 18; } int b() { return 12; }'
    assert 7 'int main() { return add2(3,4); } int add2(int x, int y) { return x+y; }'
    assert 1 'int main() { return sub2(4,3); } int sub2(int x, int y) { return x-y; }'
    assert 55 'int main() { return fib(9); } int fib(int x) { if (x<=1) return 1; return fib(x-1) + fib(x-2); }'
fi

if [[ $1 = "all" || $1 = "array" ]]; then
    assert 3 'int main() { int x[2]; int *y=&x; *y=3; return *y; }'
    assert 3 'int main() { int x[3]; *x=3; *(x+1)=4; return *x; }'
    assert 4 'int main() { int x[3]; *x=3; *(x+1)=4; *(x+2)=5; return *(x+1); }'
    assert 5 'int main() { int x[3]; *x=3; *(x+1)=4; *(x+2)=5; return *(x+2); }'

    assert 0 'int main() { int x[2][3]; int *y=x; *y=0; return **x; }'
    assert 1 'int main() { int x[2][3]; int *y=x; *(y+1)=1; return *(*x+1); }'
    assert 2 'int main() { int x[2][3]; int *y=x; *(y+2)=2; return *(*x+2); }'
    assert 3 'int main() { int x[2][3]; int *y=x; *(y+3)=3; return **(x+1); }'
    assert 4 'int main() { int x[2][3]; int *y=x; *(y+4)=4; return *(*(x+1)+1); }'
    assert 4 'int main() { int x[2][3]; int *y=x; *(y+4)=4; return *(*(x+1)+1); }'
    assert 5 'int main() { int x[2][3]; int *y=x; *(y+5)=5; return *(*(x+1)+2); }'

    assert 3 'int main() { int x[3]; *x=3; x[1]=4; x[2]=5; return *x; }'
    assert 4 'int main() { int x[3]; *x=3; x[1]=4; x[2]=5; return *(x+1); }'
    assert 5 'int main() { int x[3]; *x=3; x[1]=4; x[2]=5; return *(x+2); }'
    assert 5 'int main() { int x[3]; *x=3; x[1]=4; x[2]=5; return *(x+2); }'
    assert 5 'int main() { int x[3]; *x=3; x[1]=4; 2[x]=5; return *(x+2); }'
    assert 0 'int main() { int x[2][3]; int *y=x; y[0]=0; return x[0][0]; }'
    assert 1 'int main() { int x[2][3]; int *y=x; y[1]=1; return x[0][1]; }'
    assert 2 'int main() { int x[2][3]; int *y=x; y[2]=2; return x[0][2]; }'
    assert 3 'int main() { int x[2][3]; int *y=x; y[3]=3; return x[1][0]; }'
    assert 4 'int main() { int x[2][3]; int *y=x; y[4]=4; return x[1][1]; }'
    assert 5 'int main() { int x[2][3]; int *y=x; y[5]=5; return x[1][2]; }'
fi

if [[ $1 = "all" || $1 = "sizeof" ]]; then
    assert 4 'int main() { int x; return sizeof(x); }'
    assert 4 'int main() { int x; return sizeof x; }'
    assert 8 'int main() { int *x; return sizeof(x); }'
    assert 16 'int main() { int x[4]; return sizeof(x); }'
    assert 48 'int main() { int x[3][4]; return sizeof(x); }'
    assert 16 'int main() { int x[3][4]; return sizeof(*x); }'
    assert 4 'int main() { int x[3][4]; return sizeof(**x); }'
    assert 5 'int main() { int x[3][4]; return sizeof(**x) + 1; }'
    assert 5 'int main() { int x[3][4]; return sizeof **x + 1; }'
    assert 4 'int main() { int x[3][4]; return sizeof(**x + 1); }'
    assert 4 'int main() { int x=1; return sizeof(x=2); }'
    assert 1 'int main() { int x=1; sizeof(x=2); return x; }'
fi

if [[ $1 = "all" || $1 = "global" ]]; then
    assert 0 'int x; int main() { return x; }'
    assert 3 'int x; int main() { x=3; return x; }'
    assert 7 'int x; int y; int main() { x=3; y=4; return x+y; }'
    assert 7 'int x, y; int main() { x=3; y=4; return x+y; }'
    assert 0 'int x[4]; int main() { x[0]=0; x[1]=1; x[2]=2; x[3]=3; return x[0]; }'
    assert 1 'int x[4]; int main() { x[0]=0; x[1]=1; x[2]=2; x[3]=3; return x[1]; }'
    assert 2 'int x[4]; int main() { x[0]=0; x[1]=1; x[2]=2; x[3]=3; return x[2]; }'
    assert 3 'int x[4]; int main() { x[0]=0; x[1]=1; x[2]=2; x[3]=3; return x[3]; }'
    assert 4 'int x; int main() { return sizeof(x); }'
    assert 16 'int x[4]; int main() { return sizeof(x); }'
fi

if [[ $1 = "all" || $1 = "char" ]]; then
    assert 1 'int main() { char x=1; return x; }'
    assert 1 'int main() { char x=1; char y=2; return x; }'
    assert 2 'int main() { char x=1; char y=2; return y; }'
    assert 1 'int main() { char x; return sizeof(x); }'
    assert 10 'int main() { char x[10]; return sizeof(x); }'
    assert 1 'int main() { return sub_char(7, 3, 3); } int sub_char(char a, char b, char c) { return a-b-c; }'
fi

if [[ $1 = "all" || $1 = "string" ]]; then
    assert 0 'int main() { return ""[0]; }'
    assert 1 'int main() { return sizeof(""); }'
    assert 97 'int main() { return "abc"[0]; }'
    assert 98 'int main() { return "abc"[1]; }'
    assert 99 'int main() { return "abc"[2]; }'
    assert 0 'int main() { return "abc"[3]; }'
    assert 4 'int main() { return sizeof("abc"); }'

    assert 7 'int main() { return "\a"[0]; }'
    assert 8 'int main() { return "\b"[0]; }'
    assert 9 'int main() { return "\t"[0]; }'
    assert 10 'int main() { return "\n"[0]; }'
    assert 11 'int main() { return "\v"[0]; }'
    assert 12 'int main() { return "\f"[0]; }'
    assert 13 'int main() { return "\r"[0]; }'
    assert 27 'int main() { return "\e"[0]; }'
    assert 106 'int main() { return "\j"[0]; }'
    assert 107 'int main() { return "\k"[0]; }'
    assert 108 'int main() { return "\l"[0]; }'
    assert 7 'int main() { return "\ax\ny"[0]; }'
    assert 120 'int main() { return "\ax\ny"[1]; }'
    assert 10 'int main() { return "\ax\ny"[2]; }'
    assert 121 'int main() { return "\ax\ny"[3]; }'
fi

# Add selected tests below
# ...

echo "OK"
