#!/bin/sh
# ./build.sh      | Build bcc
# ./build.sh test | Build and run tests

if [ "$(uname -s)" = Darwin ]; then
    clang --target=x86_64-macos -Wall -Wextra -Wpedantic --std=c11 -Icompiler/include $(find compiler -name "*.c") -o bcc-x86_64 || exit
    if [ "$(arch)" = arm64 ]; then
        clang --target=arm64-macos -Wall -Wextra -Wpedantic --std=c11 -Icompiler/include $(find compiler -name "*.c") -o bcc-arm64 || exit
    fi
fi
if [ "$(uname -s)" = Linux ]; then
    gcc -Wall -Wextra -Wpedantic --std=gnu11 -Icompiler/include $(find compiler -name "*.c") -ldl -o bcc-x86_64 || exit
fi
if [ "$(uname -o)" = Msys ]; then
    gcc -Wall -Wextra -Wpedantic --std=c11 -Icompiler/include $(find compiler -name "*.c") -o bcc-x86_64 || exit
fi

# Function that runs a test
assert() {
    expected=$1
    input="$2"

    # Compile and run for x86_64
    if [ -e "./bcc-x86_64" ]; then
        if [ "$(uname -o)" = Msys ]; then
            echo -e "$input" | ./bcc-x86_64 $(find stdlib -name "*.h") -
        else
            echo "$input" | ./bcc-x86_64 $(find stdlib -name "*.h") -
        fi
        actual=$?
        if [ $actual != "$expected" ]; then
            echo "[FAIL] Program:"
            echo "$input"
            echo "Dump:"
            echo "$input" | ./bcc-x86_64 -d $(find stdlib -name "*.h") -
            echo "Arch: x86_64 | Return: $actual | Correct: $expected"
            exit 1
        fi
    fi

    # Compile and run for arm64
    if [ -e "./bcc-arm64" ]; then
        echo "$input" | ./bcc-arm64 $(find stdlib -name "*.h") -
        actual=$?
        if [ $actual != "$expected" ]; then
            echo "[FAIL] Program:"
            echo "$input"
            echo "Dump:"
            echo "$input" | ./bcc-arm64 -d $(find stdlib -name "*.h") -
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

    assert 56 "unsigned int main() { return 56u; }"
    assert 40 "unsigned long main() { return 40ull; }"
    assert 97 "char main() { return 'a'; }"
    assert 53 "char main() { return '0' + 5; }"

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
    assert 5 "unsigned long main() { int x=3; return (&x+2)-&x+3; }"

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

    assert 4 "unsigned long main() { int x=3; return sizeof x; }"
    assert 2 "unsigned long main() { int x=3; return sizeof(x) - 2; }"
    assert 12 "unsigned long main() { int *x=3; return sizeof x + 4; }"
    assert 16 "unsigned long main() { int x[4]; return sizeof x; }"
    assert 32 "unsigned long main() { int *x[4]; return sizeof x; }"
    assert 32 "unsigned long main() { int x[4][2]; return sizeof x; }"

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

    assert 10 "int a() { return 4; } int main() { return 10; }"
    assert 8 "int a() { return 4; } int main() { return a() + 4; }"
    assert 4 "int a() { return 4; } int main() { return 20 - (a() * 4); }"
    assert 6 "int a() { return 4; } int b() { return 6; } int c() { return 2 + 2; } int main() { return a() + (b() - c()); }"
    assert 32 "int ret32() { return 32; } int main() { return ret32(); }"
    assert 7 "int add2(int x, int y) { return x+y; } int main() { return add2(3,4); }"
    assert 1 "int sub2(int x, int y) { return x-y; } int main() { return sub2(4,3); }"
    assert 55 "int fib(int x) { if (x<=1) return 1; return fib(x-1) + fib(x-2); } int main() { return fib(9); }"

    assert 0 "int x; int main() { return x; }"
    assert 3 "int x; int main() { x=3; return x; }"
    assert 7 "int x; int y; int main() { x=3; y=4; return x+y; }"
    assert 7 "int x, y; int main() { x=3; y=4; return x+y; }"
    assert 0 "int x[4]; int main() { x[0]=0; x[1]=1; x[2]=2; x[3]=3; return x[0]; }"
    assert 1 "int x[4]; int main() { x[0]=0; x[1]=1; x[2]=2; x[3]=3; return x[1]; }"
    assert 2 "int x[4]; int main() { x[0]=0; x[1]=1; x[2]=2; x[3]=3; return x[2]; }"
    assert 3 "int x[4]; int main() { x[0]=0; x[1]=1; x[2]=2; x[3]=3; return x[3]; }"
    assert 4 "int x; unsigned long main() { return sizeof(x); }"
    assert 16 "int x[4]; unsigned long main() { return sizeof(x); }"

    assert 0 'char main() { return ""[0]; }'
    assert 1 'unsigned long main() { return sizeof(""); }'
    assert 97 'char main() { return "abc"[0]; }'
    assert 98 'char main() { return "abc"[1]; }'
    assert 99 'char main() { return "abc"[2]; }'
    assert 0 'char main() { return "abc"[3]; }'
    assert 4 'unsigned long main() { return sizeof("abc"); }'

    assert 7 "char main() { return '\\a'; }"
    assert 7 'char main() { return "\\a"[0]; }'
    assert 8 'char main() { return "\\b"[0]; }'
    assert 9 'char main() { return "\\t"[0]; }'
    assert 10 'char main() { return "\\n"[0]; }'
    assert 11 'char main() { return "\\v"[0]; }'
    assert 12 'char main() { return "\\f"[0]; }'
    assert 13 'char main() { return "\\r"[0]; }'
    assert 27 'char main() { return "\\e"[0]; }'
    assert 106 'char main() { return "\\j"[0]; }'
    assert 107 'char main() { return "\\k"[0]; }'
    assert 108 'char main() { return "\\l"[0]; }'
    assert 7 'char main() { return "\\ax\\ny"[0]; }'
    assert 120 'char main() { return "\\ax\\ny"[1]; }'
    assert 10 'char main() { return "\\ax\\ny"[2]; }'
    assert 121 'char main() { return "\\ax\\ny"[3]; }'
    assert 0 'char main() { return "\\0"[0]; }'
    assert 16 'char main() { return "\\20"[0]; }'
    assert 65 'char main() { return "\\101"[0]; }'
    assert 104 'char main() { return "\\150"[0]; }'
    assert 104 "char main() { return '\\150'; }"
    assert 0 'char main() { return "\\x00"[0]; }'
    assert 119 'char main() { return "\\x77"[0]; }'
    assert 165 'char main() { return "\\xA5"[0]; }'
    assert 255 'char main() { return "\\x00ff"[0]; }'

    assert 32 "int ret32(); int main() { return ret32(); } int ret32() { return 32; }"
    assert 7 "int add2(int x, int y); int main() { return add2(3,4); } int add2(int x, int y) { return x+y; }"
    assert 1 "int sub2(int x, int y); int main() { return sub2(4,3); } int sub2(int x, int y) { return x-y; }"

    assert 3 'unsigned long main() { return strlen("Hoi"); }'
    assert 1 'char main() { return !strcmp("Hoi", "Hoi"); }'
    assert 0 'char main() { return !strcmp("Hoi", "Hoi2"); }'
    assert 0 'int main() { puts("Hello Bassie C Compiler!"); return 0; }'

    echo "[OK] All tests pass"
fi
