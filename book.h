/*
	Copyright (C) 2019 TheTrustedComputer

	The file structure of Four the Win's opening book or database is as follows:

	Book file signature - 4 bytes
	Column and row count of the board - 1 byte each
	Game variant - 1 byte
	Number of book entries - 4 bytes
	Hash value of that position - 8 bytes
	Move value of the same - 1 byte
		The hexadecimal values of these moves are given below.
		Positive values are wins, and negative values are losses. Zeroes are considered a draw.
			00 - Draw (D)
			01 - Win immediately (W0)
			02 - Win in the next move (W2)
			03 - Win in 2 moves and so on (W4)
			...
			7E - Win in 125 moves (W250)
			7F - Win in 126 moves (W252)
			80 - Lose in 128 turns (L255)
			81 - Lose in 127 turns (L253)
			...
			FC - Lose in 4 turns (L7)
			FD - Lose in 3 turns (L5)
			FE - Lose in 2 turns (L3)
			FF - Lose in the next turn (L1)

		In PopOut, there is a possibility of losing instantly after popping, and the normal book does not take into account of this possibility.
		Therefore, all losses are shifted by one to compensate it. Instead of 0xFF being a loss in the next turn, it becomes lost immediately.

		The move values can only handle boards smaller than 16x16. Boards at least 16x16 will require a structural change from a byte to 2 bytes.
		Positions that is the mirror of existing entries, containing full columns, or having four-in-a-row connections are not added to the book.

		This format is not supported in Pop Ten as there are many different starting positions, and wins and losses can have different parity.

	Example file hexdump:
	46 54 57 42 07 06 00 01 00 00 00 81 40 20 10 08 04 00 00 15

	The first four hexadecimal pairs correspond to "FTWB" in ASCII.
*/

#ifndef BOOK_H
#define BOOK_H

// Various strings to tell the user roughly what went wrong

#define BOOK_SIGNATURE "FTWB"

#define BOOK_UNABLE_OPEN_FILE "Unable to open the book file for "
#define BOOK_UNABLE_CLOSE_FILE "Unable to close the book file for "
#define BOOK_UNABLE_GENERATE "Unable to generate the book file for "

#define BOOK_READING_STRING "reading.\n"
#define BOOK_WRITING_STRING "writing.\n"
#define BOOK_APPENDING_STRING "appending.\n"
#define BOOK_UPDATING_STRING "updating the number of records.\n"
#define BOOK_GENERATING_STRING "generating.\n"
#define BOOK_CREATING_STRING "creating.\n"
#define BOOK_EOF_REACHED_STRING "End of file reached during book lookup."

#define BOOKFILE_WRONG "Book contains the wrong "
#define BOOKFILE_APPENDED  "Book entry appended."
#define BOOKFILE_EXISTS "Book entry already exists."
#define BOOKFILE_TERMINAL "Terminal position."

#define BOOKFILE_NORMAL "normal_"
#define BOOKFILE_POPOUT "popout_"
#define BOOKFILE_POWERUP "powerup_"
#define BOOKFILE_POPTEN "popten_"
#define	BOOKFILE_FIVEINAROW "fiveinarow_"

#define BOOKFILE_FTWB_EXTENSION ".ftw"

#define BOOKENTRY_NUMRECORDS_OFFSET 7
#define BOOKENTRY_OFFSET 11
#define BOOKENTRY_EXACT 1
#define BOOKENTRY_MIRRORED 2

// Opening book file header translated to a struct
typedef struct {
	char signature[4];				// A file signature enforcement -- "FTWB"
	uint8_t columns, rows, variant; // One byte corresponding to the columns, rows, and variant
	unsigned numRecords;			// Books at its current state can have up to about 4 billion entries
} BookFile;

typedef struct UniqueTable { Position *entry; long long size; } UniqueTable;

// Global variables to control opening book generation
BookFile bookFile;			// An instance of the BookFile structure that does all the literacy work
unsigned currentEntries;    // The number of entries that is currently in the book
size_t hashBytes;			// A indicator of how many bytes required to encode a Connect Four position
bool subMovesNotInBook;

// Functions on file operations
bool BookFile_create(char*);									// Open a file for creating an opening book or database
bool BookFile_readFromDrive(char*);								// Read then parse the opening book from a file on a drive
bool BookFile_write(char*);										// Write header information to the book containing zero entries
bool BookFile_append(char*, ConnectFour*, Position, Result*);	// Append computed solutions to the opening book depending on the variant
void BookFile_normal_append(FILE*, ConnectFour*, Result*);		// Append solutions from the standard game to the opening book
void BookFile_popout_append(FILE*, ConnectFour*, Result*);		// Append solutions from the PopOut variant to the opening book

// Searching functions
bool BookFile_retrieve(char*, ConnectFour*, Position, Result*, Result*);					// Retrieve an entry from the book and store that result to a variable
bool BookFile_normal_retrieve(FILE*, ConnectFour*, Position, Result*, int*);				// Retrieve a normal ruleset entry from the book if it exists
bool BookFile_popout_retrieve(FILE*, ConnectFour*, Position, Result*, int*, int, bool);		// Retrieve a PopOut ruleset entry from the book if it exists
int BookFile_hashLookup(FILE*, ConnectFour*, Position, bool);								// Search through the book for matching entries given a Connect Four hash

// Book generation functions
bool BookFile_generateBook(char*, ConnectFour*, unsigned);												// Generate book entries depending on the Connect Four variant
void BookFile_normal_generateEntries(FILE*, ConnectFour*, unsigned);									// Generate normal entries and append them to the book against limited moves
void BookFile_popout_generateEntries(FILE*, ConnectFour*, Result*, unsigned);							// Generate PopOut entries and append them to the book against limited moves
bool BookFile_storeToTranspositionTable(char*, ConnectFour*, TranspositionTable*, const bool);			// Store book entries to the transposition table memory for faster access time
int BookFile_loadFromTranspositionTable(char*, TranspositionTable*, Position, Result*);
bool BookFile_loadSubMovesFromTranspositionTable(char*, ConnectFour*, TranspositionTable*, Result*);
bool BookFile_checkVaildEntry(char*, ConnectFour*, TranspositionTable*, unsigned);							// Check whether the book file is valid: all unique moves have their entries
bool BookFile_normal_checkVaildEntry(char*, ConnectFour*, TranspositionTable*, unsigned);
bool BookFile_popout_checkVaildEntry(char*, ConnectFour*, TranspositionTable*, unsigned);

// The book stores game solutions in a different manner; convert it to a Result that the solver can understand
int8_t BookFile_normal_convertToEntry(Result*);		// Convert a Result to a book entry (Normal)
Result BookFile_normal_convertToResult(int8_t);		// Convert a book entry to a Result	(Normal)
int8_t BookFile_popout_convertToEntry(Result*);		// Convert a Result to a book entry (PopOut)
Result BookFile_popout_convertToResult(int8_t);		// Convert a book entry to a Result (PopOut)

#endif
