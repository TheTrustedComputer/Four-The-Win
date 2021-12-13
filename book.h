/*
	Copyright (C) 2019 TheTrustedComputer

	The file structure of Four the Win's opening book is as follows:

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
		Positions that is the mirror of existing entries or have full columns would not be added to the book. The book will check all legal moves before returning the result for consistency.

		This format is not supported in Pop Ten as wins and losses can have different parity than what was defined internally.

	Example file:
	46 54 57 42 07 06 00 01 00 00 00 81 40 20 10 08 04 00 00 15

	The first four hexadecimal pairs correspond to "FTWB" in ASCII.
*/

#ifndef BOOK_H
#define BOOK_H

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

#define BOOKFILE_FTWB_EXTENSION ".ftwb"

#define BOOKENTRY_NUMRECORDS_OFFSET 7
#define BOOKENTRY_OFFSET 11
#define BOOKENTRY_EXACT 1
#define BOOKENTRY_MIRRORED 2

typedef struct {
	char signature[4];
	uint8_t columns, rows, variant;
	unsigned numRecords;
} BookFile;

enum BookRecord {
	LOSS_RECORD, DRAW_RECORD, WIN_RECORD, NO_RECORD = 0xff
};

BookFile bookFile;
unsigned currentEntries;
size_t hashBytes;

bool BookFile_create(char*);
bool BookFile_readFromDrive(char*);
bool BookFile_write(char*);
bool BookFile_append(char*, ConnectFour*, Position, Result*);
void BookFile_normal_append(FILE*, ConnectFour*, Result*);
void BookFile_popout_append(FILE*, ConnectFour*, Result*);;

bool BookFile_retrieve(char*, ConnectFour*, Position, Result*, Result*);
bool BookFile_normal_retrieve(FILE*, ConnectFour*, Position, Result*, int*);
bool BookFile_popout_retrieve(FILE*, ConnectFour*, Position, Result*, int*);
int BookFile_hashLookup(FILE*, ConnectFour*, Position, bool);

bool BookFile_generateBook(char*, ConnectFour*, unsigned short);
void BookFile_normal_generateEntries(FILE*, ConnectFour*, Result*, unsigned);
void BookFile_popout_generateEntries(FILE*, ConnectFour*, Result*, unsigned);
bool BookFile_storeToTranspositionTable(char*, ConnectFour*, TranspositionTable*);

int8_t BookFile_normal_convertToEntry(Result*);
Result BookFile_normal_convertToResult(int8_t);
int8_t BookFile_popout_convertToEntry(Result*);
Result BookFile_popout_convertToResult(int8_t);

#endif