#include "transpositiontable.h"

bool TranspositionTable_isPrime(int n) {
	int i;

	// Any even number is not prime (composite)
	if (!(n & 1)) {
		return false;
	}

	// Check every odd number and test if prime
	for (i = 3; i * i <= n; i += 2) {
		if (!(n % i)) { // (n mod p)=0 is not prime
			return false;
		}
	}
	return true;
}

unsigned TranspositionTable_isPrevNumPrime(int n) {
	// If n is even, then subtract two; otherwise subtract one
	n = n & 1 ? n - 2 : n - 1;

	// Test for primality until there is a prime
	while (!TranspositionTable_isPrime(n)) {
		n -= 2;
	}
	return n;
}

void TranspositionTable_reset(TranspositionTable *tt) {
	GAME_VARIANT == POWERUP_VARIANT ? memset(tt->entries, 0, sizeof(PowerUp_HashtableEntry) * tt->size) : memset(tt->entries, 0, sizeof(HashtableEntry) * tt->size);
}

bool TranspositionTable_initialize(TranspositionTable *tt, int initSize) {
	// A transposition table with less than three entries does not make practical sense
	if (initSize > 3) {
		// A prime number of entries minimizes hash collisions
		tt->size = TranspositionTable_isPrevNumPrime(initSize);

		// Try to request memory from the operating system until no more is available
		if (GAME_VARIANT == POWERUP_VARIANT) {
			if (!(tt->powerUpEntries = malloc(sizeof(PowerUp_HashtableEntry) * tt->size))) {
				tt->size = 0;
				return false;
			}
			else {
				TranspositionTable_reset(tt);
			}
		}
		else {
			if (!(tt->entries = malloc(sizeof(HashtableEntry) * tt->size))) {
				tt->size = 0;
				return false;
			}
			else {
				TranspositionTable_reset(tt);
			}
		}
		return true;
	}
	return false;
}

bool TranspositionTable_resize(TranspositionTable *tt, int newSize) {
	if (newSize > 3) {
		// Save the old addresses in case the resizing operation fails
		HashtableEntry *tt_oldEntries = tt->entries;
		PowerUp_HashtableEntry *putt_oldEntries = tt->powerUpEntries;
		int tt_oldSize = tt->size;

		tt->size = prevprime(newSize);
		if (GAME_VARIANT == POWERUP_VARIANT) {
			if (!(tt->powerUpEntries = realloc(tt->powerUpEntries, sizeof(PowerUp_HashtableEntry) * tt->size))) {
				tt->powerUpEntries = putt_oldEntries;
				tt->size = tt_oldSize;
				return false;
			}
			else {
				TranspositionTable_reset(tt);
			}
		}
		else {
			if (!(tt->entries = realloc(tt->entries, sizeof(HashtableEntry) * tt->size))) {
				tt->entries = tt_oldEntries;
				tt->size = tt_oldSize;
				return false;
			}
			else {
				TranspositionTable_reset(tt);
			}
		}
		return true;
	}
	return false;
}

void TranspositionTable_destroy(TranspositionTable *tt) {
	free(tt->entries);
	tt->entries = NULL;
}

void TranspositionTable_store(TranspositionTable *tt, Position key, int value, int depth) {
	int i = key % tt->size;
	tt->entries[i].key = key;
	tt->entries[i].value = value;
	tt->entries[i].depth = depth;
}

void TranspositionTable_storeBounds(TranspositionTable *tt, Position key, int value, int depth, int bounds) {
	int i = key % tt->size;
	tt->entries[i].key = key;
	tt->entries[i].value = value;
	tt->entries[i].depth = depth;
	tt->entries[i].bounds = bounds;
}

void TranspositionTable_powerup_store(TranspositionTable *tt, Position key, Position anvilKey, Position bombKey, Position wallKey, Position x2Key, int value, int depth) {
	int i = key % tt->size;
	tt->powerUpEntries[i].normalKey = key;
	tt->powerUpEntries[i].anvilKey = anvilKey;
	tt->powerUpEntries[i].bombKey = bombKey;
	tt->powerUpEntries[i].wallKey = wallKey;
	tt->powerUpEntries[i].x2Key = x2Key;
	tt->powerUpEntries[i].value = value;
	tt->powerUpEntries[i].depth = depth;
}

void TranspositionTable_storeWithoutDepth(TranspositionTable *tt, Position key, int value) {
	int i = key % tt->size;
	tt->entries[i].key = key;
	tt->entries[i].value = value;
}

int TranspositionTable_normal_loadValue(TranspositionTable *tt, Position key) {
	int i = key % tt->size;
	return (tt->entries[i].key) == key ? tt->entries[i].value : TT_NORMAL_UNKNOWN;
}

int TranspositionTable_popout_loadValue(TranspositionTable *tt, Position key) {
	int i = key % tt->size;
	return (tt->entries[i].key == key) ? tt->entries[i].value : TT_POPOUT_UNKNOWN;
}

int TranspositionTable_popout_loadDepth(TranspositionTable *tt, Position key) {
	int i = key % tt->size;
	return tt->entries[i].key == key ? tt->entries[i].depth : -1;
}

int TranspositionTable_loadBounds(TranspositionTable* tt, Position key) {
	int i = key % tt->size;
	return tt->entries[i].key == key ? tt->entries[i].bounds : TT_UNKNOWNBOUND;
}

int TranspositionTable_powerup_loadValue(TranspositionTable *tt, Position key, Position anvilKey, Position bombKey, Position wallKey, Position x2Key) {
	int i = key % tt->size;
	if (tt->powerUpEntries[i].normalKey == key && tt->powerUpEntries[i].anvilKey == anvilKey && tt->powerUpEntries[i].bombKey == bombKey && tt->powerUpEntries[i].wallKey == wallKey && tt->powerUpEntries[i].x2Key == x2Key) {
		return tt->powerUpEntries[i].value;
	}
	return TT_NORMAL_UNKNOWN;
}

bool TranspositionTable_depthLessOrEqual(TranspositionTable *tt, Position key, short depth) {
	return tt->entries[key % tt->size].depth <= depth;
}

bool TranspositionTable_depthGreaterOrEqual(TranspositionTable *tt, Position key, short depth) {
	return tt->entries[key % tt->size].depth >= depth;
}

bool TranspositionTable_powerup_depthEquality(TranspositionTable *tt, Position key, Position anvilKey, Position bombKey, Position wallKey, Position x2Key, short depth) {
	int i = key % tt->size;
	if (tt->powerUpEntries[i].normalKey == key && tt->powerUpEntries[i].anvilKey == anvilKey && tt->powerUpEntries[i].bombKey == bombKey && tt->powerUpEntries[i].wallKey == wallKey && tt->powerUpEntries[i].x2Key == x2Key) {
		return tt->powerUpEntries[i].depth == depth;
	}
	return false;
}

int TranspositionTable_isUnique(TranspositionTable *tt, Position hash) {
	int i = hash % tt->size, j;
	if (tt->entries[i].key) { // Hash entry
		if (tt->entries[i].key == hash) { // Exact match
			return 1;
		}
		else { // Hash collision and linear probing
			for (j = 1; tt->entries[i + j].key && i + j != i; ++j) { // End condition when we have looped 
				if (i + j == tt->size) { // Reached the end of the table--wrap around to zero
					j = -i;
					continue;
				}
				if (tt->entries[i + j].key == hash) { // Collision with a match
					return 1;
				}
			}
			if (i + j != tt->entries) { // There is free space for additional entries
				tt->entries[i + j].key = hash;
				return 0;
			}
			else { // Table has no more room for more entries
				return -1;				
			}
		}
	}
	else { // No hash entry
		tt->entries[i].key = hash;
		return 0;
	}
}

/*void RepetitionTable_initialize(RepetitionTable *rt) {
	rt->repetitionEntries = calloc(MOVESIZE, sizeof(RepetitionTable_Entry));
	rt->size = 0;
}

void RepetitionTable_reset(RepetitionTable *rt) {
	memset(rt->repetitionEntries, 0, sizeof(RepetitionTable_Entry) * MOVESIZE);
	rt->size=0;
}

void RepetitionTable_add(RepetitionTable *rt, Position position) {
	if (rt->size < MOVESIZE) {
		for (unsigned i = rt->size - 1u; i != (unsigned)-1; --i) {
			if (rt->repetitionEntries[i].position == position) {
				++rt->repetitionEntries[i].count;
				return;
			}
		}
		rt->repetitionEntries[rt->size].position = position;
		++rt->repetitionEntries[rt->size++].count;
	}
	else {
		rt->repetitionEntries[(rt->size = 0)].position = position;
		rt->repetitionEntries[rt->size++].count = 1;
	}
}

void RepetitionTable_remove(RepetitionTable* rt) {
	rt->repetitionEntries[--rt->size].position = 0;
	rt->repetitionEntries[rt->size].count = 0;
}*/
