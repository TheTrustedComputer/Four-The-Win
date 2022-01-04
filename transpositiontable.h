#ifndef TABLE_H
#define TABLE_H

// Values for yet unknown values
#define TT_NORMAL_UNKNOWN 0
#define TT_POPOUT_UNKNOWN INT_MIN

// Values for alpha-beta bounds
#define TT_UNKNOWNBOUND 0
#define TT_LOWERBOUND 1
#define TT_UPPERBOUND 2

// This will allocate one gigabyte of memory for the transposition table
#define TT_TABLESIZE 67108864

typedef struct {
	Position key;
	int8_t value, bounds;
	short depth;
} HashtableEntry;

typedef struct {
	Position normalKey, anvilKey, bombKey, wallKey, x2Key;
	int8_t value;
	short depth;
} PowerUp_HashtableEntry;

typedef struct {
	union {
		HashtableEntry *entries;
		PowerUp_HashtableEntry *powerUpEntries;
	};
	long long size;
} TranspositionTable;

typedef struct {
	Position position;
	unsigned long long count;
} RepetitionTable_Entry;

typedef struct {
	RepetitionTable_Entry *repetitionEntries;
	unsigned size;
} RepetitionTable;

static unsigned tableSize;
bool bookEntryFound;

bool TranspositionTable_isPrime(long long);
unsigned TranspositionTable_isPrevNumPrime(long long);

bool TranspositionTable_initialize(TranspositionTable*, long long);
void TranspositionTable_reset(TranspositionTable*);
void TranspositionTable_resetZero(TranspositionTable*);
bool TranspositionTable_resize(TranspositionTable*, long long);
void TranspositionTable_destroy(TranspositionTable*);

void TranspositionTable_store(TranspositionTable*, Position, int8_t, short);
void TranspositionTable_storeBounds(TranspositionTable*, Position, int8_t, short, int8_t);
void TranspositionTable_storeWithoutDepth(TranspositionTable*, Position, int8_t);
void TranspositionTable_powerup_store(TranspositionTable*, Position, Position, Position, Position, Position, int8_t, short);

int TranspositionTable_normal_loadValue(TranspositionTable*, Position);
int TranspositionTable_popout_loadValue(TranspositionTable*, Position);
int TranspositionTable_popout_loadDepth(TranspositionTable*, Position);
int TranspositionTable_powerup_loadValue(TranspositionTable*, Position, Position, Position, Position, Position);
int TranspositionTable_loadBounds(TranspositionTable*, Position);
bool TranspositionTable_depthLessOrEqual(TranspositionTable*, Position, short);

int TranspositionTable_isUnique(TranspositionTable*, Position);

#endif
