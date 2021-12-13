/*
	Copyright (C) 2019- TheTrustedComputer

	Some various Connect Four artifical intelligence applications of the widely used minimax algorithm.
	Minimax searches for the absolute best move(s) by brute-forcing all legal moves in any two-player game.
	A positive score is returned if a position is won by the maximizer, a negative score for the minimizer, and zero for a draw game.

	This code uses the negamax version of minimax, where the maximizer equals to the negative of the minimizer.
	In this case, negamax always maximize. It negates the current best score after a player change, reducing the need to type code for both sides.
	Further enhancements like alpha-beta pruning tranposition tables, iterative deepening, and static move ordering were used to increase the performance of negamax.

	The current negamax code is a depth-limited weak solver. It only knows which player wins, or is a draw. It does not know how many moves needed to achieve the win.
	However, I have created a different, experimental implementation of negamax that provides the calculated best move at the given root position.
	Future improvments may include assigning a weighted score for Connect Four positions, but chances of coding this is very slim as I do not have any plans for improvment.

*/

#ifndef ALPHABETA_H
#define ALPHABETA_H

// Static string constants to know which side wins or loses
static const char WIN_TEXT[] = "WIN ";
static const char LOSS_TEXT[] = "LOSS ";
static const char DRAW_TEXT[] = "DRAW ";
static const char NONE_TEXT[] = "-- ";
static const char SEARCHING_STRING[] = "Searching for a result at depth %d...\n";
static const char FOUND_STRING[] = "Result found!\n";
static const char NOT_FOUND_STRING[] = "Result not found under %d %s; a draw is assumed.\n";
static const char REPETITION_STRING[] = "Encountered a move repetition during search.\n";

// Some of the few constants used for negamax
enum AlphaBetaResult {
	DRAW, DRAW_OR_WIN, PLAYER_PROGRESS, PLAYER_WIN
};

// Custom defined literals for the result character in the Result data structure
enum ResultChar {
	UNKNOWN_CHAR = '?', LOSS_CHAR = 'L', DRAW_CHAR = 'D', WIN_CHAR = 'W'
};

// Common macro definitions for a resultant value
#define UNKNOWN_RESULT (Result) {UNKNOWN_CHAR, -1}
#define DRAW_RESULT (Result) {DRAW_CHAR, -1}

// Macro for an upper bound win score
#define WIN_SCORE 10000

// The transposition table used to cache previously searched positions.
static TranspositionTable table;

// Counter for the number of explored nodes or positions.
static unsigned long long nodes;

// Current hash code of the currently searched position.
static Position hashKey;

// Various variables for alpha-beta enhancement.
static int *moveOrder, tableScore, repetitionFlag;

RepetitionTable rTable;

/*
	A small structure for obtaining the result of negamax to the user. Negamax itself returns a signed integer which may not be useful for the end-user.
	The structure, for example, will display itself as W50 where W is a winning condition and 50 is the number of plies to that winning move.
*/
typedef struct {
	char wdl;		// Win, draw, and loss
	uint8_t dtc;	// Depth to connection
} Result;

/*
	Structure for sorting move values of a given position. It does insertion sort after every addition as this sorting algorithm processes best when the input is sorted.
*/
typedef struct {
	struct MoveSorter_Entries { int move, score; } *moveEntries;
	uint8_t size;
} MoveSorter;

void AlphaBeta_getColumnMoveOrder(void);								// Initialize the column move order from the center to the edges.
bool AlphaBeta_normal_checkWin(ConnectFour*);							// Check for a four-in-a-row connection during alpha-beta search.
bool AlphaBeta_popout_checkWin(ConnectFour*);							// The same function but with PopOut moves and ruleset.
bool AlphaBeta_powerup_checkWin(ConnectFour*);							// Now let's deal with the highly complex Power Up variant this time.

int AlphaBeta_negamax_normal(ConnectFour*, int, int, int);				// Main alpha-beta searching function that plays the original rules.
int AlphaBeta_negamax_popout(ConnectFour*, int, int, int);				// An alpha-beta PopOut searcher with almost exactly identical purposes.
int AlphaBeta_negamax_powerup(ConnectFour*, int, int, int);
int AlphaBeta_negamax_popten(ConnectFour*, int, int, int);

void AlphaBeta_normal_getMoveScores(ConnectFour*, Result*, bool);
void AlphaBeta_popout_getMoveScores(ConnectFour*, Result*, bool);
void AlphaBeta_popten_getMoveScores(ConnectFour*, Result*, bool);
int AlphaBeta_normal_getBestMove(ConnectFour*, Result*, bool);
int AlphaBeta_popout_getBestMove(ConnectFour*, Result*, bool);
int AlphaBeta_popten_getBestMove(ConnectFour*, Result*, bool);
void AlphaBeta_normal_printPV(ConnectFour*);
void AlphaBeta_popout_printPV(ConnectFour*);

Result AlphaBeta_normal_solve(ConnectFour*);
Result AlphaBeta_popout_solve(ConnectFour*);
Result AlphaBeta_powerup_solve(ConnectFour*);
Result AlphaBeta_popten_solve(ConnectFour*);

void MoveSorter_addToEntry(MoveSorter*, const int, const int);
int MoveSorter_obtainNextMove(MoveSorter*);

void Result_print(Result*);
void Result_increment(Result*);
void Result_normal_reverse(Result*);
void Result_popout_reverse(Result*);
Result Result_getBestResult(Result*, unsigned);
unsigned Result_getBestMove(Result*, unsigned);

#endif
