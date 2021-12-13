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
	int8_t value, depth, bounds;
} HashtableEntry;

typedef struct {
	Position normalKey, anvilKey, bombKey, wallKey, x2Key;
	int8_t value, depth;
} PowerUp_HashtableEntry;

typedef struct {
	union {
		HashtableEntry *entries;
		PowerUp_HashtableEntry *powerUpEntries;
	};
	int size;
} TranspositionTable;

/*typedef struct {
	Position position;
	unsigned long long count;
} RepetitionTable_Entry;

typedef struct {
	RepetitionTable_Entry *repetitionEntries;
	unsigned size;
} RepetitionTable;*/

static unsigned tableSize;

bool TranspositionTable_isPrime(int);
unsigned TranspositionTable_isPrevNumPrime(int);

bool TranspositionTable_initialize(TranspositionTable*, int);
void TranspositionTable_reset(TranspositionTable*);
void TranspositionTable_resetZero(TranspositionTable*);
bool TranspositionTable_resize(TranspositionTable*, int);
void TranspositionTable_destroy(TranspositionTable*);

void TranspositionTable_store(TranspositionTable*, Position, int, int);
void TranspositionTable_storeBounds(TranspositionTable*, Position, int, int, int);
void TranspositionTable_storeWithoutDepth(TranspositionTable*, Position, int);
void TranspositionTable_powerup_store(TranspositionTable*, Position, Position, Position, Position, Position, int, int);

int TranspositionTable_normal_loadValue(TranspositionTable*, Position);
int TranspositionTable_popout_loadValue(TranspositionTable*, Position);
int TranspositionTable_popout_loadDepth(TranspositionTable*, Position);
int TranspositionTable_powerup_loadValue(TranspositionTable*, Position, Position, Position, Position, Position);
int TranspositionTable_loadBounds(TranspositionTable*, Position);
bool TranspositionTable_depthLessOrEqual(TranspositionTable*, Position, short);

int TranspositionTable_isUnique(TranspositionTable*, Position);

#endif
