/*
	Copyright (C) 2019- TheTrustedComputer

	A small structure for obtaining the result of two perfect opponents. Minimax returns a signed integer which may not be useful for the end-user.
	The structure, for example, will display W50 where W is won for the player to move, and 50 is the number of plies to that winning move.
*/

#ifndef RESULT_H
#define RESULT_H

// Static string constants to know which side wins or loses
static const char WIN_TEXT[] = "WIN";
static const char LOSS_TEXT[] = "LOSS";
static const char DRAW_TEXT[] = "DRAW";
static const char NONE_TEXT[] = "-- ";
static const char SEARCHING_STRING[] = "\rSolving...%d\r";
static const char FOUND_STRING[] = "Result found!\n";
static const char NOT_FOUND_STRING[] = "Result not found under %d %s; a draw is assumed.\n";
static const char REPETITION_STRING[] = "Encountered move repetition";

// Custom defined literals for the result character in the Result data structure
enum ResultChar {
	UNKNOWN_CHAR = '?', LOSS_CHAR = 'L', DRAW_CHAR = 'D', WIN_CHAR = 'W'
};

// Common macro definitions for a resultant value
#define UNKNOWN_RESULT (Result) {UNKNOWN_CHAR, -1}
#define DRAW_RESULT (Result) {DRAW_CHAR, -1}

typedef struct {
	char wdl;		// A single character: 'W', 'D', or 'L'
	uint8_t dtc;	// Depth to winning connection
} Result;

void Result_print(Result*, Result*);                // Prints this result. A winning result is in green, a drawing result yellow, and a losing result red.
void Result_increment(Result*);                     // Increments a result by one ply
Result Result_getBestResult(Result*, unsigned);     // Obtains the best result, meaning the fastest path to a win, or a slowest path to a loss
unsigned Result_getBestMove(Result*, unsigned);		// Returns the best move given a result

void Result_normal_reverse(Result*);	// Reverses a Normal result array
void Result_popout_reverse(Result*);	// Reverses a PopOut result array


#endif
