# My processor, an AMD Ryzen 7 5800X, is in the Zen 3 architecture family.
#CPU_ARCH = znver3

# Uncomment to target towards the older AMD K10 architecture (applies to my previous AMD A8-3870K computer).
#CPU_ARCH = amdfam10

# Uncomment to target the Gemini Lake architecture -- the AWOW mini PC uses this processor.
#CPU_ARCH = goldmont-plus

# Uncomment to tell the C compiler to target your computer's processor.
CPU_ARCH = native

# Uncomment to compile for best move instead of depth towards a win (will be removed).
FTW_BEST = -DSCORE_TEST

# Uncomment to use a fixed transposition table size of one gigabyte.
FTW_FIXED = -DUSE_FIXED_TTSIZE

# Use the preprocessor macros defined in the C source code.
FTW_MACROS = -DUSE_MACROS

# Initialize the GCC complier and its arguments assuming they are installed.
CC = gcc
CFLAGS = -Ofast -g -Wall -Wextra -march=${CPU_ARCH} -lpthread -lm
SOURCES = main.c
OUTPUT = -o FourTheWin

# General build if you prefer to build it vanilla.
all:
	${CC} ${CFLAGS} ${SOURCES} ${FTW_FIXED} ${OUTPUT}

# Help message to guide users into building Four The Win.
help:
	@echo "No help message yet. Please open the Makefile to your text editor."

# Builds for specific Connect Four variants.
normal: main.c
ifdef FTW_BEST
	${CC} ${CFLAGS} ${FTW_MACROS} ${FTW_BEST} -DNORMAL_RULESET ${FTW_FIXED} ${SOURCES} -o FourTheWin_normal
else
	${CC} ${CFLAGS} ${FTW_MACROS} ${FTW_BEST} -DNORMAL_RULESET ${FTW_FIXED} ${SOURCES} ${OUTPUT}
endif

popout: main.c
ifdef FTW_BEST
	${CC} ${CFLAGS} ${FTW_MACROS} ${FTW_BEST} -DPOPOUT_RULESET ${FTW_FIXED} ${SOURCES} -o FourTheWin_popout
else
	${CC} ${CFLAGS} ${FTW_MACROS} ${FTW_BEST} -DPOPOUT_RULESET ${FTW_FIXED} ${SOURCES} ${OUTPUT}
endif

powerup: main.c
ifdef FTW_BEST
	${CC} ${CFLAGS} ${FTW_MACROS} ${FTW_BEST} -DPOWERUP_RULESET ${FTW_FIXED} ${SOURCES} -o FourTheWin_powerup
else
	${CC} ${CFLAGS} ${FTW_MACROS} ${FTW_BEST} -DPOWERUP_RULESET ${FTW_FIXED} ${SOURCES} ${OUTPUT}
endif

popten: main.c
ifdef FTW_BEST
	${CC} ${CFLAGS} ${FTW_MACROS} ${FTW_BEST} -DPOPTEN_RULESET ${FTW_FIXED} ${SOURCES} -o FourTheWin_popten
else
	${CC} ${CFLAGS} ${FTW_MACROS} ${FTW_BEST} -DPOPTEN_RULESET ${FTW_FIXED} ${SOURCES} ${OUTPUT}
endif

