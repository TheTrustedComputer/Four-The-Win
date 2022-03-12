## My processor, an AMD Ryzen 7 5800X, is in the Zen 3 architecture family.
#CPU_ARCH = znver3

## Uncomment to target towards the older AMD K10 architecture (applies to my previous AMD A8-3870K computer).
#CPU_ARCH = amdfam10

## Uncomment to target the Intel Gemini Lake architecture -- the AWOW AK41 mini PC uses this processor.
#CPU_ARCH = goldmont-plus

## Uncomment to tell the C compiler to target your computer's processor.
CPU_ARCH = native

## Uncomment to use GCC's 128-bit integer types for larger sizes; defaults to the language standard 64-bit.
#UINT_128 = -DUINT_128

## Initialize the GCC complier and its arguments assuming they are installed.
# Uncomment to use LLVM Clang, and comment the one containg GCC (recommended).
#CC = clang
CC = gcc
CFLAGS = -Ofast -g -Wall -Wextra -march=${CPU_ARCH} ${UINT_128}
SOURCES = main.c
OUTPUT = -o FourTheWin
LIBRARIES = -lpthread -lm

# A general yet simple build script.
build:
	${CC} ${CFLAGS} ${SOURCES} ${LIBRARIES} ${OUTPUT}
