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
    assert 0 '{ return 0; }'
    assert 42 '{ return 42; }'
    assert 21 '{ return 5+20-4; }'
    assert 41 '{ return  12 + 34 - 5 ; }'
    assert 47 '{ return 5+6*7; }'
    assert 15 '{ return 5*(9-6); }'
    assert 4 '{ return (3+5)/2; }'
    assert 10 '{ return -10+20; }'
    assert 10 '{ return - -10; }'
    assert 10 '{ return - - +10; }'

    assert 2 '{ return 56%6; }'
    assert 1 '{ return !(4 > 5); }'

    assert 0 '{ return 0==1; }'
    assert 1 '{ return 42==42; }'
    assert 1 '{ return 0!=1; }'
    assert 0 '{ return 42!=42; }'

    assert 1 '{ return 0<1; }'
    assert 0 '{ return 1<1; }'
    assert 0 '{ return 2<1; }'
    assert 1 '{ return 0<=1; }'
    assert 1 '{ return 1<=1; }'
    assert 0 '{ return 2<=1; }'

    assert 1 '{ return 1>0; }'
    assert 0 '{ return 1>1; }'
    assert 0 '{ return 1>2; }'
    assert 1 '{ return 1>=0; }'
    assert 1 '{ return 1>=1; }'
    assert 0 '{ return 1>=2; }'
fi

if [[ $2 = "all" || $2 = "locals" ]]; then
    assert 3 '{ a=3; return a; }'
    assert 8 '{ a=3; z=5; return a+z; }'

    assert 3 '{ a=3; return a; }'
    assert 8 '{ a=3; z=5; return a+z; }'
    assert 6 '{ a=b=3; return a+b; }'
    assert 3 '{ foo=3; return foo; }'
    assert 8 '{ foo123=3; bar=5; return foo123+bar; }'
    assert 6 '{ x = 3; y = 4; z = 5; 6; }'
    assert 15 '{ {{{ x = 3; }} foo123=3;} bar=5; return 3*bar; }'

    assert 1 '{ return 1; 2; 3; }'
    assert 2 '{ 1; return 2; 3; }'
    assert 3 '{ 1; 2; return 3; }'

    assert 3 '{ {1; {2;} return 3;} }'
    assert 5 '{ ;;; return 5; }'

    assert 3 '{ if (0) return 2; return 3; }'
    assert 3 '{ if (1-1) return 2; return 3; }'
    assert 2 '{ if (1) return 2; return 3; }'
    assert 2 '{ if (2-1) return 2; return 3; }'
    assert 4 '{ if (0) { 1; 2; return 3; } else { return 4; } }'
    assert 3 '{ if (1) { 1; 2; return 3; } else { return 4; } }'
fi

if [[ $2 = "all" || $2 = "loops" ]]; then
    assert 10 '{ i=0; while(i<10) { i=i+1; } return i; }'
    assert 55 '{ i=0; j=0; for (i=0; i<=10; i=i+1) j=i+j; return j; }'
    assert 3 '{ for (;;) {return 3;} return 5; }'
fi

if [[ $2 = "all" || $2 = "ptrs" ]]; then
    assert 3 '{ x=3; return *&x; }'
    assert 3 '{ x=3; return *&x; }'
    assert 5 '{ x=3; y=5; return *(&x+1); }'
    assert 3 '{ x=3; y=5; return *(&y-1); }'
    assert 5 '{ x=3; y=5; return *(&x-(-1)); }'
    assert 7 '{ x=3; y=5; *(&x+1)=7; return y; }'
    assert 7 '{ x=3; y=5; *(&y-2+1)=7; return x; }'
    assert 5 '{ x=3; return (&x+2)-&x+3; }'
fi
