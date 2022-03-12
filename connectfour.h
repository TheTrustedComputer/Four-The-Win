/*
	Copyright (C) 2019- TheTrustedComputer

	This is a data structure implementation of the famous Connect Four game and its rules.
	Connect Four is represented as a 64-bit bitboard in memory; it is a faster alternative to an array of integers.
	It currently supports the original rules and all four official Hasbro variants: PopOut, Power Up, Pop Ten, and Five-in-a-Row.

	Visual representation of the bitboard for 7x6:
	.. .. .. .. .. .. ..
	05 12 19 26 33 40 47
	04 11 18 25 32 39 46
	03 10 17 24 31 38 45
	02 09 16 23 30 37 44
	01 08 15 22 29 36 43
	00 07 14 21 28 35 42

	Numbers indicate the bit index of where each disk may be located. Dots are where these bits are unused but useful for hash encoding.

	Originally, two players respectively drop their colored disks on a seven-by-six gridded board.
	Players cannot drop another disk on a full column and must use an empty or partially filled column.
	The player who connects their disks four-in-a-row vertically, horizontally, or diagonally wins the game.
	If neither player has a four-in-a-row before all of the columns were filled, the game is declared drawn.

	In PopOut, the game plays exactly like normal except players are granted an additional move called a pop.
	A pop is when a player removes one of their disks from the bottom, causing the others on top to fall.
	This makes simultaneous four-in-a-rows and repetitions possible in this variant.
	To handle the first scenario, the player who popped at that turn is chosen as the winner.
	The second scenario will be treated as a draw as it does for chess--a longer, more strategical game than Connect Four.

	Power Up is a lot more complex than PopOut as players are introduced four extra disks called Power Checkers.
	Players may play at most one specific Power Checker per each game. They cannot play any more once they are all dropped.
	Here are the basic descriptions on how each Power Checker works after it is dropped:
	Anvils - The player immediately pops all of the disks below it.
	Bombs - The player pops one of the opponent's disks on the bottom.
	Walls* - The player gains a second turn, but it cannot be dropped where the player can create a Connect Four.
	x2* - Virtually identical functionality to walls, but without the aforementioned restriction.
	*Players cannot play another Power Checker with any of the wall or x2 disks.

	Pop Ten is different from PopOut and Power Up. The starting position is setup unlike the other three.
	Players are required to fill the bottom row first, then the next row until the board is filled.
	There is no order where players can drop their disks at. The variant starts when both players pop their disks from the bottom.
	If the popped disk is part of a four-in-a-row connection, then that player gets to keep it and recieves another turn.
	Otherwise, the player must put it back not in the same column whenever possible, and the turn switches to the other player.
	In some incredibly rare circumstances, the player to move cannot make a single pop move when it is their turn!
	That player must pass his turn unless the player already made some pop moves. If the latter case, the player must place that disk back to the board.
	The objective is to be the first player to collect at least ten disks.

	Solvability of Connect Four games:
	Original: Solved in 1988 by James Allen and Victor Allis. The first player wins. John Tromp showed different players win on different board sizes.
	PopOut: Solved in 2014 by Jukka Pukkala. The first player wins. The solution is half as long than with the original rules.
	Power Up: Unsolved. This variant has a huge branching factor. For example, there are 105 (7 normal, 7*7 walls and 7*7 x2's) possible moves in the starting position!
	Pop Ten: Unsolved. In order for this variant to be solved, all permutations of the starting position full of disks must be solved!
	Five-In-A-Row: Solved by me in 2016 editing John Tromp's Fhourstones program. The game ends in a draw when both sides play perfectly.

*/

#ifndef CONNECTFOUR_H
#define CONNECTFOUR_H

// Connect Four board or grid size dimensions and common constants.
unsigned COLUMNS, ROWS;
unsigned COLUMNS_M1, COLUMNS_X2, COLUMNS_X2_P1, COLUMNS_D2;
unsigned ROWS_M1, ROWS_P1, ROWS_P2;
unsigned AREA, HISTORYSIZE, MOVESIZE, MOVESIZE_M1;

// The Connect Four bitboard using a 128-bit (recompile with -DUINT_128 on GCC; MSVC uses LargePosition instead) or a 64-bit integer.
#if defined(UINT_128) && defined(__GNUC__)
typedef unsigned __int128 Position;
#else
typedef uint_fast64_t Position;
#endif

// Connect Four static bitmaps.
static Position BOT, ALL, TOP, COLS, ALLCOLS;

// Larger versions of the above.
static LargePosition L_BOT, L_ALL, L_TOP, L_COLS, L_ALLCOLS;

// Bitmaps for the odd and even rows.
static Position ODDROWS, EVENROWS;

// Larger versions of the odd/even rows from above.
static LargePosition L_ODDROWS, L_EVENROWS;

// The game variant to use as defined in GameVariant or macros.
static unsigned GAME_VARIANT;

// Positions to hold the state of the column before the anvil disk was dropped (Power Up).
static Position anvilColumnNormals[2][2], anvilColumnAnvils[2], anvilColumnWalls[2], anvilColumnBombs[2], anvilColumnX2s[2];

// Position to store the heights of disks before an anvil disk was played (Power Up).
static unsigned anvilHeight[2];

// An 8-bit integer to save the type of Power Checker before being popped by a Bomb disk (Power Up).
static uint8_t bombPoppedType[2];

// A bitmap to save current Pop Ten starting position after a reset (Pop Ten).
static Position popTenBitmap[2];

// Variable to describe the Pop Ten pop status from PopTenPopStatus (Pop Ten).
static uint8_t popTenFlags;

// Counter for the number of moves played while benchmarking.
static unsigned counter;

// The principal variation of the currently searched best move.
static char *pv;

// The disk type to choose when the user inputs a move.
static int userPowerUpDiskType;

// Move buffers for Power Checker disks that take two inputs to perform an action.
static int userPowerCheckerColumnBuffer, userPowerCheckerReadyToMoveFlag;

// Yellow and red are one of the most common colors for Connect Four disks; let's name the players after those colors.
static const char PLAYER1_NAME[] = "Yellow";
static const char PLAYER2_NAME[] = "Red";

// Single bit to hold whether a Power Checker has been played; there are no binary literals in C, only C++14 and above have them
enum PowerCheckerBit {
	PLAYER1_ANVILBIT = 0x1, PLAYER1_BOMBBIT = 0x2, PLAYER1_WALLBIT = 0x4, PLAYER1_X2BIT = 0x8,
	PLAYER2_ANVILBIT = 0x10, PLAYER2_BOMBBIT = 0x20, PLAYER2_WALLBIT = 0x40, PLAYER2_X2BIT = 0x80
};

// The program needs to perform bit shifting in order to obtain the number of Power Checkers played.
enum PowerCheckerShifter {
	PLAYER1_ANVILSHIFT, PLAYER1_BOMBSHIFT, PLAYER1_WALLSHIFT, PLAYER1_X2SHIFT, PLAYER2_ANVILSHIFT, PLAYER2_BOMBSHIFT, PLAYER2_WALLSHIFT, PLAYER2_X2SHIFT
};

// Indices to encode the type of disk that has been dropped in Power Up.
enum DiskDroppedType {
	DROPTYPE_NORMAL, DROPTYPE_ANVIL, DROPTYPE_BOMB, DROPTYPE_WALL, DROPTYPE_X2
};

// Flags to indicate the status of the number of collected disks after a pop in Pop Ten.
enum PopTenPopStatus {
	POPTEN_POP_NO_CONNECTION, POPTEN_POP_CONNECTION = 0xc0, POPTEN_DROP = 0x80, POPTEN_PASS = 0x40
};

// Flags of what type of Power Checker was dropped into the board
enum PowerUpDropStatus {
	POWERUP_DROP_NORMAL_OR_ANVIL = 0x1, POWERUP_DROP_BOMB = 0x2, POWERUP_DROP_BOMB_POP = 0x10, POWERUP_DROP_WALL = 0x4, POWERUP_DROP_X2 = 0x8
};

// An enumerator for official Hasbro Connect Four game variants.
enum GameVariant {
	NORMAL_VARIANT, POPOUT_VARIANT, POWERUP_VARIANT, POPTEN_VARIANT, FIVEINAROW_VARIANT
};

// Structure of positions to store a bitboard of played Power Checkers.
typedef struct PowerCheckers {
	Position anvil, bomb, wall, x2;
} PowerCheckers;

// The same for bigger boards larger than 64 bits but less than 128 bits.
typedef struct LargePowerCheckers {
	LargePosition l_anvil, l_bomb, l_wall, l_x2;
} LargePowerCheckers;

// Structure containing a Power Up move.
// There are five different types -- 3 bits minimum
typedef struct PowerUpMove {
	int status : 5, diskType : 4, normalColumn : 4, powerColumn : 4;
} PowerUpMove;

// Vector or dynamic array containing Pop Ten moves.
typedef struct PopTenMove {
	uint8_t *pops, size;
} PopTenMove;

// Data structure holding the popular Connect Four game. Unions repurpose memory for bigger boards and required data for other variants.
typedef struct ConnectFour {
	union {		// Universal
		Position *board;															// Position to store the bitmap of all disks (all variants)
		LargePosition *l_board;														// Larger version of the above as boards larger than 8x8 are unsupported (all variants)
	};
	union {		// Variant dependent
		Position *history;															// Recorded position history for identifying move repetitions (PopOut, Pop Ten)
		LargePosition *l_history;													// Larger version of the above (PopOut, Pop Ten)
		PowerCheckers *pc;															// Position to store the bitboard of played Power Checkers (Power Up)
		LargePowerCheckers *l_pc;													// Larger version of the above (Power Up)
	};
	union {		// Moves container
		uint8_t *moves;																// The moves made during a Normal, PopOut or Five-In-A-Row game
		PowerUpMove *pum;															// The moves made during a Power Up game
		PopTenMove *ptm;															// The moves made during a Pop Ten game
	};
	uint8_t *height; //, historyIndex;													// Bit indexes containing the height of each column, and the move history index (PopOut)
	unsigned plyNumber;																// The number of half-moves during a game
	union {		// Counters
		uint8_t playedPowerCheckers;												// An 8-bit integer storing played Power Checkers (Power Up) Format: P2(X2 WA BO AN) P1(X2 WA BO AN)
		int8_t historyIndex, collectedDisks;										// The number of collected disks when popping a disc (Pop Ten)
	};
} ConnectFour;

// Miscellaneous helper functions
Position intpow(Position, Position);												// Integer exponentiation for printing the game board
int intlog2(Position);																// Integer binary logarithm for correct column output
unsigned popcount(Position);														// Common bit counter to list legal moves

// Memory functions
void ConnectFour_setSizeAndVariant(unsigned, unsigned, unsigned);					// Set the rows and columns of the board and also set the variant
void ConnectFour_setupBitmaps(void);												// Setup bitmaps to manipulate bits on ConnectFour bitboards
void ConnectFour_initialize(ConnectFour*);											// Initialize the ConnectFour data structure. Some variants need more or less memory for required data
void ConnectFour_reset(ConnectFour*, const bool);									// Reset the ConnectFour state to the starting position
void ConnectFour_popten_reset(ConnectFour*, const bool);							// Reset the ConnectFour state, and set up the Pop Ten game by filling up the board
void ConnectFour_destroy(ConnectFour*);												// Deallocate memory used by the ConnectFour structure

// Printing functions
void ConnectFour_printBoard(const ConnectFour*);									// Print out a colored board to console like the real game
void ConnectFour_printMoves(const ConnectFour*);									// Print the game's moves to console as a series of characters

// Assessment functions
Position ConnectFour_connection(const Position);									// Check whether a given Connect Four position has a four-in-a-row connection
Position ConnectFour_connectionNoVertical(const Position);							// Identical to the one above, but without checking for a vertical connection--used for PopOut as it is impossible
Position ConnectFour_popten_connection(const Position);								// Check to see if that bottom disk is part of a four-in-a-row connection (Pop Ten)
Position ConnectFour_fiveinarow_connection(const Position);							// Returns non-zero when a five-in-a-row connection occurs
LargePosition ConnectFour_LargePosition_connection(const LargePosition);			// Larger board version of the connection detection function
LargePosition ConnectFour_LargePosition_connectionNoVertical(const LargePosition);	// Larger board version to test for a connection without testing vertical wins (PopOut)
LargePosition ConnectFour_LargePosition_popten_connection(const LargePosition);		// Larger board version to see if the bottom disk is part of a connection (Pop Ten)
bool ConnectFour_hasTenDisks(const ConnectFour*);									// Check if a player has collected ten disks (Pop Ten)
int8_t ConnectFour_repeatIndex(const int8_t);										// Return the previous index where a possible duplication may occur (PopOut, Pop Ten)
bool ConnectFour_repetition(const ConnectFour*);									// True if the position was repeated three times, false otherwise. Another possibility was when the same position is reached but mirrored
bool ConnectFour_gameOver(const ConnectFour*);										// Determine if the game is over depending on the ruleset

// Normal move functions
bool ConnectFour_normal_drop(ConnectFour*, const int);								// Drop a disk if the certain column is not full
void ConnectFour_normal_undrop(ConnectFour*);										// Undo the drop move--only during alpha-beta
bool ConnectFour_LargePosition_normal_drop(ConnectFour*, const int);				// Drop a disk in a larger board size
void ConnectFour_LargePosition_normal_undrop(ConnectFour*);							// Undo the drop move in a larger board size

// PopOut move functions
bool ConnectFour_popout_drop(ConnectFour*, const int);								// Drop a disk and save that position to the PopOut move history
void ConnectFour_popout_undrop(ConnectFour*);										// Undo the drop move and decrement the move history pointer
bool ConnectFour_popout_pop(ConnectFour*, const int);								// Pop if the bottom disk is the player's disk
void ConnectFour_popout_unpop(ConnectFour*);										// Undo the pop move from the current player
bool ConnectFour_popout_popWinCheck(ConnectFour*, const int);						// Simplified version to check a connection during alpha-beta
void ConnectFour_popout_unpopWinCheck(ConnectFour*, const int);						// Simplified version of the above, used wit alpha-beta

// Power Up move functions
bool ConnectFour_powerup_drop(ConnectFour*, const int);								// Drop a normal disk and store it into the Power Up move history
void ConnectFour_powerup_undrop(ConnectFour*);										// Undo the normal drop--identical to the original variant
bool ConnectFour_powerup_dropAnvil(ConnectFour*, const int);						// Drop an anvil if the certain column is not full, and then isolate it
void ConnectFour_powerup_undropAnvil(ConnectFour*);									// Undo the anvil disk isolation and drop
bool ConnectFour_powerup_dropBomb(ConnectFour*, const int);							// Drop a bomb if the certain column is not full, then pop an opponent's disk
void ConnectFour_powerup_undropBomb(ConnectFour*);									// Undo the popping of the opponent disk and bomb drop
bool ConnectFour_powerup_dropWall(ConnectFour*, const int);							// Drop a wall, then drop a non-winning normal disk if the certain column is not full
void ConnectFour_powerup_undropWall(ConnectFour*);									// Undo the wall drop
bool ConnectFour_powerup_dropX2(ConnectFour*, const int);							// Drop an x2, then drop a normal disk if the certain column is not full
void ConnectFour_powerup_undropX2(ConnectFour*);									// Undo the x2 drop
bool ConnectFour_powerup_pop(ConnectFour*, const int);		// Pop if the bottom disk is the opponent's disk (Power Up exclusive)
void ConnectFour_powerup_unpop(ConnectFour*);										// Undo the pop move (opponent, Power Up variant only)
void ConnectFour_powerup_incrementPlayedPowerCheckers(ConnectFour*, const int);
void ConnectFour_powerup_decrementPlayedPowerCheckers(ConnectFour*);

// Pop Ten move functions
bool ConnectFour_popten_drop(ConnectFour*, const int);								// Drop a disk from the top under Pop Ten rules
void ConnectFour_popten_undrop(ConnectFour*);										// Unperform the drop move (Pop Ten)
bool ConnectFour_popten_pop(ConnectFour*, const int);								// Pop a disk from the bottom under Pop Ten rules
void ConnectFour_popten_unpop(ConnectFour*);										// Undo the pop under Pop Ten rules
bool ConnectFour_popten_pass(ConnectFour*);											// If the player cannot make a move (no available pops), then that player must pass to the other player
void ConnectFour_popten_unpass(ConnectFour*);										// Undo the passing move
void ConnectFour_popten_addDisk(ConnectFour*);										// Increment the number of collected disks by one
void ConnectFour_popten_removeDisk(ConnectFour*);									// Do the opposite of the above by decrementing it by one
uint8_t ConnectFour_popten_getPoppedDisks(const ConnectFour*, const bool);			// Return popped disks players have kept
Position ConnectFour_popten_poppable(const ConnectFour*);							// Return true if the current player can pop on their turn
void ConnectFour_popten_copy(ConnectFour*, ConnectFour*);

// Functions relating to move generation
bool ConnectFour_play(ConnectFour*, const char);									// Play a single move from the user
bool ConnectFour_sequence(ConnectFour*, const char*);								// Play a series of moves from the user
unsigned ConnectFour_numberLegalMoves(const ConnectFour*);							// Return the total number of legal moves depending on the variant
unsigned ConnectFour_numberLegalDrops(const ConnectFour*);							// Return the total number of legal drops
unsigned ConnectFour_numberLegalPops(const ConnectFour*);							// Return the total number of legal pops

// Other miscellaneous functions
Position ConnectFour_bottom(void);													// Bitmap corresponding to the bottom of the board
Position ConnectFour_reverse(Position);												// Reverse a Connect Four position--used for transposition tables
Position ConnectFour_getHashKey(const ConnectFour*);								// Hash value of the current Connect Four position--used for transposition tables
Position ConnectFour_getPowerUpHashKey(const ConnectFour*, const Position);			// Hash value for the Power Up game variation--used for transposition tables
unsigned ConnectFour_countBottomDisksFromHashKey(Position);							// Extract the number of bottom disks from the given hash key
void ConnectFour_reverseBoard(ConnectFour*);										// Reverse a Connect Four position then update the heights
unsigned ConnectFour_getMoveSize(void);												// Return the number of moves available for play
void ConnectFour_displayHelpMessage(char*);											// Display the help message if user types "-help" in command-line
bool ConnectFour_symmetrical(const Position);										// Return true if the position is horizontally symmetrical

// Move history functions
int8_t ConnectFour_previousHistory(const int8_t);									// Increment the history index (PopOut)
int8_t ConnectFour_nextHistory(const int8_t);										// Decrement the history index (PopOut)
void ConnectFour_addHistory(ConnectFour*);											// Append the position to the move history and increment the history index (PopOut)
void ConnectFour_clearHistory(ConnectFour*);										// Clear out the move history and reset the history index to zero (PopOut)

// Benchmarking functions
void ConnectFour_benchmark(ConnectFour*, const unsigned);							// Benchmark and print the number of position made per second
void ConnectFour_normal_benchmark(ConnectFour*, const unsigned);					// Main move generation function to test for performance or profiling
void ConnectFour_popout_benchmark(ConnectFour*, const unsigned);					// PopOut move generation for benchmarking performance
void ConnectFour_powerup_benchmark(ConnectFour*, const unsigned);					// Move generation benchmark performance for Power Up
void ConnectFour_popten_benchmark(ConnectFour*, const unsigned);					// Analyze the move performance made in the Pop Ten variant
void ConnectFour_fiveinarow_benchmark(ConnectFour*, const unsigned);				// Try to test for performance in Five-In-A-Row (more or less the same as original)

#endif
