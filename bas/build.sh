gcc -Wall -Wextra -Wpedantic -Werror --std=c2x -Iinclude $(find src -name *.c) -o bas || exit
./bas test.s -o test.exe
objdump -S -Mintel test.exe
./test
echo $?
