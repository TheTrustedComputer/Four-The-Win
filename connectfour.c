/*
	Copyright (C) 2019 TheTrustedComputer
*/

#include "connectfour.h"

Position intpow(Position x, Position y) {
	Position z = 1;
	while (y) {
		if (y & 1) {
			z *= x;
		}
		x *= x;
		y >>= 1;
	}
	return z;
}

int intlog2(Position x) {
	if (!x) {
		return -1;
	}
	int y = 0;
	while (x > 1) {
		x >>= 1;
		++y;
	}
	return y;
}

unsigned popcount(Position x) {
	unsigned count = 0u;
	while (x) {
		x &= x - 1;
		++count;
	}
	return count;
}

void ConnectFour_setSizeAndVariant(unsigned newColumns, unsigned newRows, unsigned newVariant) {
#ifndef USE_MACROS
	if (newColumns * (newRows + 1u) > 64u) {
		newColumns = 7u;
		newRows = 6u;
	}
	GAME_VARIANT = newVariant;
	COLUMNS = newColumns;
	ROWS = newRows;
	COLUMNS_M1 = COLUMNS - 1u;
	COLUMNS_X2 = COLUMNS << 1u;
	COLUMNS_D2 = COLUMNS >> 1u;
	ROWS_M1 = ROWS - 1u;
	ROWS_P1 = ROWS + 1u;
	ROWS_P2 = ROWS + 2u;
	AREA = COLUMNS * ROWS;
	switch (GAME_VARIANT) {
	case POPOUT_VARIANT:
	case POPTEN_VARIANT:
		HISTORYSIZE = 21u;
		MOVESIZE = HISTORYSIZE + AREA + (AREA >> 1u);
		break;
	case POWERUP_VARIANT:
		MOVESIZE = AREA + (AREA << 1u);
		break;
	default:
		MOVESIZE = AREA;
	}
	MOVESIZE_M1 = MOVESIZE - 1u;
#else
	GAME_VARIANT = newVariant;
#endif
}

void ConnectFour_setupBitmaps(void) {
	unsigned i;
	BOT = ConnectFour_bottom();
	ALL = BOT * (((Position)1ull << ROWS) - (Position)1ull);
	TOP = BOT << ROWS;
	COLS = (((Position)1ull << ROWS_P1) - 1ull);
	ALLCOLS = COLS >> 1ull;
	EVENROWS = 0ull;
	ODDROWS = BOT;
	for (i = 0u; i < ROWS; ++i) {
		(i & 1u) ? (EVENROWS |= BOT << i) : (ODDROWS ^= BOT << i);
	}
}

void ConnectFour_initialize(ConnectFour *cf) {
	if (!(cf->board = malloc(sizeof(Position) * 2))) {
		fprintf(stderr, "Could not set up memory for the Connect Four board.");
		exit(EXIT_FAILURE);
	}
	if (!(cf->height = malloc(sizeof(uint8_t) * COLUMNS))) {
		fprintf(stderr, "Could not utilize memory for the board height information.");
		exit(EXIT_FAILURE);
	}
	switch (GAME_VARIANT) {
	case POPOUT_VARIANT:
		if (!(cf->history = malloc(sizeof(Position) * HISTORYSIZE))) {
			fprintf(stderr, "Could not handle memory for the position history.");
			exit(EXIT_FAILURE);
		}
	case FIVEINAROW_VARIANT:
	case NORMAL_VARIANT:
		if (!(cf->moves = malloc(sizeof(uint8_t) * MOVESIZE))) {
			fprintf(stderr, "Could not use memory for storing played moves.");
			exit(EXIT_FAILURE);
		}
		break;
	case POWERUP_VARIANT:
		if (!(cf->pc = malloc(sizeof(PowerCheckers)))) {
			fprintf(stderr, "Could not make room in memory for the Power Checkers bitboard.");
			exit(EXIT_FAILURE);
		}
		if (!(cf->pum = malloc(sizeof(PowerUpMove) * MOVESIZE))) {
			fprintf(stderr, "Could not register memory for Power Up moves.");
			exit(EXIT_FAILURE);
		}
		break;
	case POPTEN_VARIANT:
	{
		if (!(cf->ptm = malloc(sizeof(PopTenMove) * (MOVESIZE + 1ull)))) {
			fprintf(stderr, "Could not add on to memory for Pop Ten move structure.");
			exit(EXIT_FAILURE);
		}
		else {
			unsigned i;
			cf->ptm->size = 0;
			for (i = 0; i <= MOVESIZE; ++i) {
				if (!(cf->ptm[i].pops = malloc(sizeof(uint8_t) * 11ull))) {
					fprintf(stderr, "Could not drop by in memory for Pop Ten pop moves.");
					exit(EXIT_FAILURE);
				}
				else {
					cf->ptm[i].size = 0;
				}
			}
		}
	}
	}
	ConnectFour_reset(cf, false);
}

void ConnectFour_reset(ConnectFour *cf, const bool keepBitmaps) {
	unsigned i;
	cf->board[0] = cf->board[1] = 0;
	cf->plyNumber = 0;
	for (i = 0; i < COLUMNS; ++i) {
		cf->height[i] = ROWS_P1 * i;
	}
	switch (GAME_VARIANT) {
	case POPOUT_VARIANT:
		ConnectFour_clearHistory(cf);
	case NORMAL_VARIANT:
		cf->moves[0] = '\xff';
		break;
	case POWERUP_VARIANT:
		cf->pc->anvil = cf->pc->bomb = cf->pc->wall = cf->pc->x2 = 0;
		cf->playedPowerCheckers = 0;
		userPowerUpDiskType = DROPTYPE_NORMAL;
		userPowerCheckerReadyToMoveFlag = 0;
		break;
	case POPTEN_VARIANT:
		ConnectFour_popten_reset(cf, keepBitmaps);
	}
}

void ConnectFour_popten_reset(ConnectFour *cf, const bool bitmap) {
	unsigned i;
	popTenFlags = POPTEN_DROP;
	cf->ptm->size = 1;
	cf->ptm->pops[0] = popTenFlags;
	if (!bitmap) {
		unsigned j, k, l, column, remainingColumns, *setupColumns = malloc(sizeof(unsigned) * COLUMNS);
		init_genrand64((unsigned long long)time(NULL));
		cf->collectedDisks = 0;
		for (i = 0; i < COLUMNS; ++i) {
			cf->height[i] = ROWS_P1 * i;
		}
		for (i = 0; i < ROWS; ++i) {
			for (j = 0; j < COLUMNS; ++j) {
				setupColumns[j] = j;
			}
			remainingColumns = COLUMNS;
			for (j = 0; j < COLUMNS; ++j) {
				column = setupColumns[genrand64_int64() % (unsigned long long)remainingColumns];
				cf->board[cf->plyNumber++ & 1u] |= (Position)1ull << cf->height[column]++;
				for (k = 0;; ++k) {
					if (setupColumns[k] == column) {
						for (l = k; l < remainingColumns - 1; ++l) {
							setupColumns[l] = setupColumns[l + 1];
						}
						--remainingColumns;
						break;
					}
				}
			}
		}
		cf->plyNumber = 0;
		popTenBitmap[0] = cf->board[0];
		popTenBitmap[1] = cf->board[1];
		free(setupColumns);
	}
	else {
		cf->collectedDisks = 0;
		cf->plyNumber = 0;
		cf->board[0] = popTenBitmap[0];
		cf->board[1] = popTenBitmap[1];
		for (i = 0; i < COLUMNS; ++i) {
			cf->height[i] = ROWS_P1 * i + ROWS;
		}
	}
}

void ConnectFour_destroy(ConnectFour *cf) {
	switch (GAME_VARIANT) {
	case POPOUT_VARIANT:
		free(cf->history);
		cf->history = NULL;
	case FIVEINAROW_VARIANT:
	case NORMAL_VARIANT:
		free(cf->moves);
		cf->moves = NULL;
		break;
	case POWERUP_VARIANT:
		free(cf->pc);
		cf->pc = NULL;
		free(cf->pum);
		cf->pum = NULL;
		break;
	case POPTEN_VARIANT:
	{
		for (unsigned i = 0; i < 12; ++i) {
			free(cf->ptm[i].pops);
			cf->ptm[i].pops = NULL;
		}
		free(cf->ptm);
		cf->ptm = NULL;
	}
	}
}

void ConnectFour_printBoard(const ConnectFour *cf) {
	unsigned i, j;
#ifdef _WIN32
	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
#endif
	if (cf->plyNumber & 1u) {
#ifdef _WIN32
		SetConsoleTextAttribute(handle, 0xc);
#else
		printf("\e[1;91m");
#endif
	}
	else {
#ifdef _WIN32
		SetConsoleTextAttribute(handle, 0xe);
#else
		printf("\e[1;93m");
#endif
	}
	for (i = 0; i < COLUMNS; ++i) {
		printf(" %u", i + 1u);
	}
#ifdef __unix__
	printf("\e[0m");
#endif
	puts("");
	for (i = ROWS_M1; i != (unsigned)-1; --i) {
		for (j = 0; j < COLUMNS; ++j) {
			Position bit = intpow(2, (Position)ROWS_P1 * j) * intpow(2, i);
			if (GAME_VARIANT == POWERUP_VARIANT) {
				if (bit & cf->board[0] & cf->pc->anvil) {
#ifdef _WIN32
					SetConsoleTextAttribute(handle, 0xe6);
					printf("AN");
#elif defined(__unix__)
					printf("\e[7;1;33;43mAN\e[0m");
#endif
					continue;
				}
				else if (bit & cf->board[1] & cf->pc->anvil) {
#ifdef _WIN32
					SetConsoleTextAttribute(handle, 0xc4);
					printf("AN");
#elif defined(__unix__)
					printf("\e[7;1;31;41mAN\e[0m");
#endif
					continue;
				}
				else if (bit & cf->board[0] & cf->pc->bomb) {
#ifdef _WIN32
					SetConsoleTextAttribute(handle, 0xe6);
					printf("BO");
#elif defined(__unix__)
					printf("\e[7;1;33;43mBO\e[0m");
#endif
					continue;
				}
				else if (bit & cf->board[1] & cf->pc->bomb) {
#ifdef _WIN32
					SetConsoleTextAttribute(handle, 0xc4);
					printf("BO");
#elif defined(__unix__)
					printf("\e[7;1;31;41mBO\e[0m");
#endif
					continue;
				}
				else if (bit & cf->board[0] & cf->pc->wall) {
#ifdef _WIN32
					SetConsoleTextAttribute(handle, 0xe6);
					printf("WA");
#elif defined(__unix__)
					printf("\e[7;1;33;43mWA\e[0m");
#endif
					continue;
				}
				else if (bit & cf->board[1] & cf->pc->wall) {
#ifdef _WIN32
					SetConsoleTextAttribute(handle, 0xc4);
					printf("WA");
#elif defined(__unix__)
					printf("\e[7;1;31;41mWA\e[0m");
#endif
					continue;
				}
				else if (bit & cf->board[0] & cf->pc->x2) {
#ifdef _WIN32
					SetConsoleTextAttribute(handle, 0xe6);
					printf("X2");
#elif defined(__unix__)
					printf("\e[7;1;33;43mX2\e[0m");
#endif
					continue;
				}
				else if (bit & cf->board[1] & cf->pc->x2) {
#ifdef _WIN32
					SetConsoleTextAttribute(handle, 0xc4);
					printf("X2");
#elif defined(__unix__)
					printf("\e[7;1;31;41mX2\e[0m");
#endif
					continue;
				}
			}
			if (bit & cf->board[0]) {
#ifdef _WIN32
				SetConsoleTextAttribute(handle, 0xe6);
				printf("()");
#elif defined(__unix__)
				printf("\e[7;1;33;43m()\e[0m");
#endif
			}
			else if (bit & cf->board[1]) {
#ifdef _WIN32
				SetConsoleTextAttribute(handle, 0xc4);
				printf("()");
#elif defined(__unix__)
				printf("\e[7;1;31;41m()\e[0m");
#endif
			}
			else {
#ifdef _WIN32
				SetConsoleTextAttribute(handle, 0x18);
				printf("()");
#elif defined(__unix__)
				printf("\e[37;44m()\e[0m");
#endif
			}
		}
		if (i) {
#ifdef _WIN32
			SetConsoleTextAttribute(handle, 0x7);
#elif defined(__unix__)
			printf("\e[0m");
#endif
			puts("");
		}
	}
#ifdef _WIN32
	SetConsoleTextAttribute(handle, 0x7);
#endif
	switch (GAME_VARIANT) {
	case POPTEN_VARIANT:
		printf("\n%c-%d %c-%d", PLAYER1_NAME[0], cf->collectedDisks & 0x0f, PLAYER2_NAME[0], (cf->collectedDisks & 0xf0) >> 4);
	}
	puts("");
}

void ConnectFour_printMoves(const ConnectFour *cf) {
	for (unsigned i = 0; i < cf->plyNumber; ++i) {
		printf("%c", (cf->moves[i] < COLUMNS) ? cf->moves[i] + '1' : cf->moves[i] - COLUMNS + 'A');
	}
}

Position ConnectFour_connection(const Position POS) {
	Position ver, hor, di1, di2;
	ver = POS & (POS >> 1);
	hor = POS & (POS >> ROWS_P1);
	di1 = POS & (POS >> ROWS);
	di2 = POS & (POS >> ROWS_P2);
	return (ver & (ver >> 2ull)) | (hor & (hor >> (ROWS_P1 << 1u))) | (di1 & (di1 >> (ROWS << 1u))) | (di2 & (di2 >> (ROWS_P2 << 1u)));
}

Position ConnectFour_connectionNoVertical(const Position POS) {
	Position hor, di1, di2;
	hor = POS & (POS >> ROWS_P1);
	di1 = POS & (POS >> ROWS);
	di2 = POS & (POS >> ROWS_P2);
	return (hor & (hor >> (ROWS_P1 << 1u))) | (di1 & (di1 >> (ROWS << 1u))) | (di2 & (di2 >> (ROWS_P2 << 1u)));
}

Position ConnectFour_popten_connection(const Position POS) {
	Position ver, hor, hor1, di1, di2;
	ver = POS & (POS >> 1ull);
	hor = POS & (POS >> ROWS_P1);
	di1 = POS & (POS >> ROWS);
	di2 = POS & (POS >> ROWS_P2);
	hor1 = (hor & (hor >> (ROWS_P1 << 1u)));
	return (ver & (ver >> 2ull)) | (hor1 | (hor1 << ROWS_P1) | (hor1 << (ROWS_P1 << 1u)) | (hor1 << (ROWS_P1 + (ROWS_P1 << 1u)))) | (di1 & (di1 >> (ROWS << 1u))) << (ROWS + (ROWS << 1)) | (di2 & (di2 >> (ROWS_P2 << 1u)));
}

Position ConnectFour_fiveinarow_connection(const Position POS) {
	// Position ver, hor, di1, di2;
	return 0;
}

Position ConnectFour_calculateAllThreats(const Position POS) {
	Position totalThreats, currentThreats;
	// Vertical
	totalThreats = (POS << 1ull) & (POS << 2ull) & (POS << 3ull);
	// Horizontal going right
	currentThreats = (POS << ROWS_P1) & (POS << (ROWS_P1 << 1u));
	totalThreats |= currentThreats & (POS >> ROWS_P1);
	totalThreats |= currentThreats & (POS << ((ROWS_P1 << 1u) + ROWS_P1));
	// Left
	currentThreats = (POS >> ROWS_P1) & (POS >> (ROWS_P1 << 1u));
	totalThreats |= currentThreats & (POS << ROWS_P1);
	totalThreats |= currentThreats & (POS >> ((ROWS_P1 << 1u) + ROWS_P1));
	// Positive slope diagonal going downleft
	currentThreats = (POS << ROWS_P2) & (POS << (ROWS_P2 << 1u));
	totalThreats |= currentThreats & (POS >> ROWS_P2);
	totalThreats |= currentThreats & (POS << ((ROWS_P2 << 1u) + ROWS_P2));
	// Upright
	currentThreats = (POS >> ROWS_P2) & (POS >> (ROWS_P2 << 1u));
	totalThreats |= currentThreats & (POS << ROWS_P2);
	totalThreats |= currentThreats & (POS >> ((ROWS_P2 << 1u) + ROWS_P2));
	// Negative slope diagonal going downright
	currentThreats = (POS << ROWS) & (POS << (ROWS << 1u));
	totalThreats |= currentThreats & (POS >> ROWS);
	totalThreats |= currentThreats & (POS << ((ROWS << 1u) + ROWS));
	// Upleft
	currentThreats = (POS >> ROWS) & (POS >> (ROWS << 1u));
	totalThreats |= currentThreats & (POS << ROWS);
	return (totalThreats | (currentThreats & (POS >> ((ROWS << 1u) + ROWS)))) & ALL;
}

LargePosition ConnectFour_LargePosition_connection(const LargePosition lb) {
	LargePosition l_ver, l_hor, l_di1, l_di2;
	l_ver = LargePosition_bitwise_and(lb, LargePosition_rightShift(lb, (LargePosition) { 1ull, 0ull }));
	l_di1 = LargePosition_bitwise_and(lb, LargePosition_rightShift(lb, (LargePosition) { ROWS, 0ull }));
	l_hor = LargePosition_bitwise_and(lb, LargePosition_rightShift(lb, (LargePosition) { ROWS_P1, 0ull }));
	l_di2 = LargePosition_bitwise_and(lb, LargePosition_rightShift(lb, (LargePosition) { ROWS_P2, 0ull }));
	l_ver = LargePosition_bitwise_and(l_ver, LargePosition_rightShift(l_ver, (LargePosition) { 2ull, 0ull }));
	l_di1 = LargePosition_bitwise_and(l_di1, LargePosition_rightShift(l_di1, (LargePosition) { (uint_fast64_t)ROWS << 1ull, 0ull }));
	l_hor = LargePosition_bitwise_and(l_hor, LargePosition_rightShift(l_hor, (LargePosition) { (uint_fast64_t)ROWS_P1 << 1ull, 0ull }));
	l_di2 = LargePosition_bitwise_and(l_di2, LargePosition_rightShift(l_di2, (LargePosition) { (uint_fast64_t)ROWS_P2 << 1ull, 0ull }));
	return LargePosition_bitwise_or(LargePosition_bitwise_or(l_ver, l_di1), LargePosition_bitwise_or(l_hor, l_di2));
}

bool ConnectFour_hasTenDisks(const ConnectFour *cf) {
	return (popTenFlags == POPTEN_POP_CONNECTION) ? ((cf->plyNumber & 1u) ? ((cf->collectedDisks & 0xf0) >= 0xa0) : ((cf->collectedDisks & 0xf) >= 0xa)) : false;
}

uint8_t ConnectFour_repeatIndex(const uint8_t index) {
	return index - 4 < 0 ? index + HISTORYSIZE - 4 : index - 4;
}

bool ConnectFour_repetition(const ConnectFour *cf) {
	uint8_t first, second, third, fourth, fifth;
	Position hash = ConnectFour_getHashKey(cf), mirrorHash = ConnectFour_reverse(hash);
	first = ConnectFour_previousHistory(cf->historyIndex);
	second = ConnectFour_repeatIndex(first);
	third = ConnectFour_repeatIndex(second);
	fourth = ConnectFour_repeatIndex(third);
	fifth = ConnectFour_repeatIndex(fourth);
	return (hash == cf->history[first] && hash == cf->history[second] && hash == cf->history[third] && hash == cf->history[fourth] && hash == cf->history[fifth]) ||
		(hash == cf->history[first] && mirrorHash == cf->history[second] && hash == cf->history[third] && mirrorHash == cf->history[fourth] && hash == cf->history[fifth]) ||
		(mirrorHash == cf->history[first] && hash == cf->history[second] && mirrorHash == cf->history[third] && hash == cf->history[fourth] && mirrorHash == cf->history[fifth]);
}

bool ConnectFour_gameOver(const ConnectFour *cf) {
	switch (GAME_VARIANT) {
	case POPOUT_VARIANT:
		return ConnectFour_connection(cf->board[0]) || ConnectFour_connection(cf->board[1]) || cf->plyNumber >= MOVESIZE;
	case POPTEN_VARIANT:
		return ConnectFour_hasTenDisks(cf);
	default:
		return ConnectFour_connection(cf->board[0]) || ConnectFour_connection(cf->board[1]) || cf->plyNumber >= AREA;
	}
}

bool ConnectFour_normal_drop(ConnectFour *cf, const int COLUMN) {
	Position dropper = (Position)1ull << cf->height[COLUMN];
	if (!(dropper & TOP)) {
		cf->board[cf->plyNumber & 1u] |= dropper;
		cf->moves[cf->plyNumber++] = COLUMN;
		++cf->height[COLUMN];
		return true;
	}
	return false;
}

void ConnectFour_normal_undrop(ConnectFour *cf) {
	cf->board[cf->plyNumber & 1u] ^= (Position)1ull << --cf->height[cf->moves[--cf->plyNumber]];
}

bool ConnectFour_LargePosition_normal_drop(ConnectFour *cf, const int COLUMN) {
	LargePosition dropper, top;
	dropper = LargePosition_leftShift((LargePosition){ 1ull, 0ull }, (LargePosition){cf->height[COLUMN], 0ull});
	top = LargePosition_bitwise_and(dropper, L_TOP);
	if (!LargePosition_bool(top)) {
		cf->l_board[cf->plyNumber & 1u] = LargePosition_bitwise_or(cf->l_board[cf->plyNumber & 1u], dropper);
		cf->moves[cf->plyNumber++] = COLUMN;
		++cf->height[COLUMN];
		return true;
	}
	return false;
}

void ConnectFour_LargePosition_normal_undrop(ConnectFour *cf) {
	cf->l_board[cf->plyNumber & 1u] = LargePosition_bitwise_xor(cf->l_board[cf->plyNumber & 1u], LargePosition_leftShift((LargePosition) { 1ull, 0ull }, (LargePosition) { --cf->height[cf->moves[--cf->plyNumber]], 0ull }));
}

bool ConnectFour_popout_drop(ConnectFour *cf, const int COLUMN) {
	if (ConnectFour_normal_drop(cf, COLUMN)) {
		ConnectFour_addHistory(cf);
		return true;
	}
	return false;
}

void ConnectFour_popout_undrop(ConnectFour *cf) {
	ConnectFour_normal_undrop(cf);
	cf->historyIndex = ConnectFour_previousHistory(cf->historyIndex);
}

bool ConnectFour_popout_pop(ConnectFour *cf, const int COLUMN) {
	Position bottomDisk = (Position)COLUMN * ROWS_P1, falls = ALLCOLS << bottomDisk;
	if (cf->board[cf->plyNumber & 1u] & (Position)1ull << bottomDisk) {
		cf->board[0] = ((cf->board[0] & (ALL ^ falls)) | ((cf->board[0] & falls) >> 1ull)) & ALL;
		cf->board[1] = ((cf->board[1] & (ALL ^ falls)) | ((cf->board[1] & falls) >> 1ull)) & ALL;
		--cf->height[COLUMN];
		cf->moves[cf->plyNumber++] = COLUMN + COLUMNS;
		ConnectFour_addHistory(cf);
		return true;
	}
	return false;
}

bool ConnectFour_popout_popWinCheck(ConnectFour *cf, const int COLUMN) {
	Position bottomDisk = (Position)COLUMN * ROWS_P1, falls = ALLCOLS << bottomDisk;
	if (cf->board[cf->plyNumber & 1u] & (Position)1ull << bottomDisk) {
		cf->board[0] = ((cf->board[0] & (ALL ^ falls)) | ((cf->board[0] & falls) >> 1ull)) & ALL;
		cf->board[1] = ((cf->board[1] & (ALL ^ falls)) | ((cf->board[1] & falls) >> 1ull)) & ALL;
		return true;
	}
	return false;
}

void ConnectFour_popout_unpop(ConnectFour *cf) {
	uint8_t lastPop = cf->moves[--cf->plyNumber] - COLUMNS;
	Position bottomDisk = (Position)lastPop * ROWS_P1, falls = ALLCOLS << bottomDisk;
	cf->board[0] = (cf->board[0] & (ALL ^ falls)) | (cf->board[0] & falls) << 1ull;
	cf->board[1] = (cf->board[1] & (ALL ^ falls)) | (cf->board[1] & falls) << 1ull;
	cf->board[cf->plyNumber & 1ull] |= (Position)1ull << bottomDisk;
	++cf->height[lastPop];
	cf->historyIndex = ConnectFour_previousHistory(cf->historyIndex);
}

void ConnectFour_popout_unpopWinCheck(ConnectFour *cf, const int COLUMN) {
	Position bottomDisk = (Position)COLUMN * ROWS_P1, falls = ALLCOLS << bottomDisk;
	cf->board[0] = (cf->board[0] & (ALL ^ falls)) | (cf->board[0] & falls) << 1ull;
	cf->board[1] = (cf->board[1] & (ALL ^ falls)) | (cf->board[1] & falls) << 1ull;
	cf->board[cf->plyNumber & 1ull] |= (Position)1ull << bottomDisk;
}

void ConnectFour_popout_undoMoveRepetition(ConnectFour *cf) {
	for (unsigned i = 0u; i < HISTORYSIZE - 5u; ++i) {
		cf->moves[cf->plyNumber - 1u] < COLUMNS ? ConnectFour_popout_undrop(cf) : ConnectFour_popout_unpop(cf);
	}
}

bool ConnectFour_powerup_drop(ConnectFour *cf, const int COLUMN) {
	Position dropper = (Position)1ull << cf->height[COLUMN];
	if (!(dropper & TOP)) {
		cf->board[cf->plyNumber & 1ull] |= dropper;
		cf->pum[cf->plyNumber].diskType = DROPTYPE_NORMAL;
		cf->pum[cf->plyNumber].powerColumn = -1;
		cf->pum[cf->plyNumber++].normalColumn = COLUMN;
		++cf->height[COLUMN];
		return true;
	}
	return false;
}

void ConnectFour_powerup_undrop(ConnectFour *cf) {
	cf->board[cf->plyNumber & 1u] ^= (Position)1ull << --cf->height[cf->pum[--cf->plyNumber].normalColumn];
}

bool ConnectFour_powerup_dropAnvil(ConnectFour *cf, const int COLUMN) {
	unsigned turn = cf->plyNumber & 1u;
	unsigned short anvilCount = turn ? (cf->playedPowerCheckers & PLAYER2_ANVILMASK) >> PLAYER2_ANVILSHIFT : (cf->playedPowerCheckers & PLAYER1_ANVILMASK) >> PLAYER1_ANVILSHIFT;
	if (anvilCount < 2) {
		Position dropper = (Position)1ull << cf->height[COLUMN], bottomDisk = (Position)COLUMN * ROWS_P1, falls = ALLCOLS << bottomDisk;
		if (!(dropper & TOP)) {
			if ((cf->board[0] | cf->board[1]) & falls & BOT) {
				anvilColumnNormals[turn][anvilCount][0] = cf->board[0] & falls;
				anvilColumnNormals[turn][anvilCount][1] = cf->board[1] & falls;
				anvilColumnAnvils[turn][anvilCount] = cf->pc->anvil & falls;
				anvilColumnBombs[turn][anvilCount] = cf->pc->bomb & falls;
				anvilColumnWalls[turn][anvilCount] = cf->pc->wall & falls;
				anvilColumnX2s[turn][anvilCount] = cf->pc->x2 & falls;
				cf->board[0] &= ALL ^ falls;
				cf->board[1] &= ALL ^ falls;
				cf->pc->anvil &= ALL ^ falls;
				cf->pc->bomb &= ALL ^ falls;
				cf->pc->wall &= ALL ^ falls;
				cf->pc->x2 &= ALL ^ falls;
				cf->pc->anvil |= (Position)1ull << bottomDisk;
				cf->board[turn] |= (Position)1ull << bottomDisk;
				anvilHeight[turn][anvilCount] = cf->height[COLUMN];
				cf->height[COLUMN] = (unsigned)bottomDisk + 1u;
				ConnectFour_powerup_incrementPlayedPowerCheckers(cf, DROPTYPE_ANVIL);
				cf->pum[cf->plyNumber].diskType = DROPTYPE_ANVIL;
				cf->pum[cf->plyNumber].normalColumn = -1;
				cf->pum[cf->plyNumber++].powerColumn = COLUMN;
				return true;
			}
		}
	}
	return false;
}

void ConnectFour_powerup_undropAnvil(ConnectFour *cf) {
	unsigned turn = --cf->plyNumber & 1u;
	unsigned short anvilCount = turn ? (cf->playedPowerCheckers & PLAYER2_ANVILMASK) >> PLAYER2_ANVILSHIFT : (cf->playedPowerCheckers & PLAYER1_ANVILMASK) >> PLAYER1_ANVILSHIFT;
	cf->board[cf->plyNumber & 1] ^= (Position)1ull << --cf->height[cf->pum[cf->plyNumber].powerColumn];
	cf->pc->anvil ^= (Position)1ull << cf->height[cf->pum[cf->plyNumber].powerColumn];
	cf->height[cf->pum[cf->plyNumber].powerColumn] = anvilHeight[turn][--anvilCount];
	cf->board[0] |= anvilColumnNormals[turn][anvilCount][0];
	cf->board[1] |= anvilColumnNormals[turn][anvilCount][1];
	cf->pc->anvil |= anvilColumnAnvils[turn][anvilCount];
	cf->pc->bomb |= anvilColumnBombs[turn][anvilCount];
	cf->pc->wall |= anvilColumnWalls[turn][anvilCount];
	cf->pc->x2 |= anvilColumnX2s[turn][anvilCount];
	ConnectFour_powerup_decrementPlayedPowerCheckers(cf);
}

bool ConnectFour_powerup_dropBomb(ConnectFour *cf, const int BOMB_COLUMN, const int POP_COLUMN) {
	unsigned turn = cf->plyNumber & 1u;
	unsigned short bombCount = turn ? (cf->playedPowerCheckers & PLAYER2_BOMBMASK) >> PLAYER2_BOMBSHIFT : (cf->playedPowerCheckers & PLAYER1_BOMBMASK) >> PLAYER1_BOMBSHIFT;
	if (bombCount < 2) {
		Position dropper = (Position)1ull << cf->height[BOMB_COLUMN];
		if (!(dropper & TOP)) {
			cf->pc->bomb |= dropper;
			cf->board[turn] |= dropper;
			++cf->height[BOMB_COLUMN];
			if (ConnectFour_powerup_pop(cf, POP_COLUMN, bombCount)) {
				ConnectFour_powerup_incrementPlayedPowerCheckers(cf, DROPTYPE_BOMB);
				cf->pum[cf->plyNumber].diskType = DROPTYPE_BOMB;
				cf->pum[cf->plyNumber].powerColumn = BOMB_COLUMN;
				cf->pum[cf->plyNumber++].normalColumn = POP_COLUMN;
				return true;
			}
			else {
				cf->pc->bomb ^= dropper;
				cf->board[turn] ^= dropper;
				--cf->height[BOMB_COLUMN];
			}
		}
	}
	return false;
}

void ConnectFour_powerup_undropBomb(ConnectFour *cf) {
	ConnectFour_powerup_unpop(cf);
	cf->board[cf->plyNumber & 1u] ^= (Position)1ull << --cf->height[cf->pum[cf->plyNumber].powerColumn];
	cf->pc->bomb ^= (Position)1ull << cf->height[cf->pum[cf->plyNumber].powerColumn];
	ConnectFour_powerup_decrementPlayedPowerCheckers(cf);
}

bool ConnectFour_powerup_dropWall(ConnectFour *cf, const int WALL_COLUMN, const int DROP_COLUMN) {
	unsigned turn = cf->plyNumber & 1u;
	unsigned short wallCount = turn ? (cf->playedPowerCheckers & PLAYER2_WALLMASK) >> PLAYER2_WALLSHIFT : (cf->playedPowerCheckers & PLAYER1_WALLMASK) >> PLAYER1_WALLSHIFT;
	if (wallCount < 2) {
		Position dropper = (Position)1ull << cf->height[WALL_COLUMN], nextDropper;
		if (!((dropper & TOP) || ConnectFour_connection(cf->board[turn] | dropper))) {
			cf->pc->wall |= dropper;
			cf->board[turn] |= dropper;
			++cf->height[WALL_COLUMN];
			nextDropper = (Position)1ull << cf->height[DROP_COLUMN];
			if (!((nextDropper & TOP) || ConnectFour_connection(cf->board[turn] | nextDropper))) {
				ConnectFour_powerup_incrementPlayedPowerCheckers(cf, DROPTYPE_WALL);
				cf->board[turn] |= nextDropper;
				cf->pum[cf->plyNumber].diskType = DROPTYPE_WALL;
				cf->pum[cf->plyNumber].powerColumn = WALL_COLUMN;
				cf->pum[cf->plyNumber++].normalColumn = DROP_COLUMN;
				++cf->height[DROP_COLUMN];
				return true;
			}
			else {
				cf->pc->wall ^= dropper;
				cf->board[turn] ^= dropper;
				--cf->height[WALL_COLUMN];
			}
		}
	}
	return false;
}

void ConnectFour_powerup_undropWall(ConnectFour *cf) {
	ConnectFour_powerup_undrop(cf);
	cf->board[cf->plyNumber & 1u] ^= (Position)1ull << --cf->height[cf->pum[cf->plyNumber].powerColumn];
	cf->pc->wall ^= (Position)1ull << cf->height[cf->pum[cf->plyNumber].powerColumn];
	ConnectFour_powerup_decrementPlayedPowerCheckers(cf);
}

bool ConnectFour_powerup_dropX2(ConnectFour *cf, const int X2_COLUMN, const int DROP_COLUMN) {
	unsigned turn = cf->plyNumber & 1u;
	unsigned short x2Count = turn ? (cf->playedPowerCheckers & PLAYER2_X2MASK) >> PLAYER2_X2SHIFT : (cf->playedPowerCheckers & PLAYER1_X2MASK) >> PLAYER1_X2SHIFT;
	if (x2Count < 2) {
		Position dropper = (Position)1ull << cf->height[X2_COLUMN], nextDropper;
		if (!(dropper & TOP)) {
			cf->pc->x2 |= dropper;
			cf->board[turn] |= dropper;
			++cf->height[X2_COLUMN];
			nextDropper = (Position)1ull << cf->height[DROP_COLUMN];
			if (!(nextDropper & TOP)) {
				ConnectFour_powerup_incrementPlayedPowerCheckers(cf, DROPTYPE_X2);
				cf->board[turn] |= nextDropper;
				cf->pum[cf->plyNumber].diskType = DROPTYPE_X2;
				cf->pum[cf->plyNumber].powerColumn = X2_COLUMN;
				cf->pum[cf->plyNumber++].normalColumn = DROP_COLUMN;
				++cf->height[DROP_COLUMN];
				return true;
			}
			else {
				cf->pc->x2 ^= dropper;
				cf->board[turn] ^= dropper;
				--cf->height[X2_COLUMN];
			}
		}
	}
	return false;
}

void ConnectFour_powerup_undropX2(ConnectFour *cf) {
	ConnectFour_powerup_undrop(cf);
	cf->board[cf->plyNumber & 1] ^= (Position)1ull << --cf->height[cf->pum[cf->plyNumber].powerColumn];
	cf->pc->x2 ^= (Position)1ull << cf->height[cf->pum[cf->plyNumber].powerColumn];
	ConnectFour_powerup_decrementPlayedPowerCheckers(cf);
}

bool ConnectFour_powerup_pop(ConnectFour *cf, const int COLUMN, const unsigned short BOMB_COUNT) {
	Position bottomDisk = (Position)COLUMN * ROWS_P1, falls = COLS << bottomDisk, popped = (Position)1ull << bottomDisk;
	unsigned opponentTurn = !(cf->plyNumber & 1u);
	if (cf->board[opponentTurn] & cf->pc->anvil & popped) {
		bombPoppedType[BOMB_COUNT][opponentTurn] = DROPTYPE_ANVIL;
		goto popper;
	}
	if (cf->board[opponentTurn] & cf->pc->bomb & popped) {
		bombPoppedType[BOMB_COUNT][opponentTurn] = DROPTYPE_BOMB;
		goto popper;
	}
	if (cf->board[opponentTurn] & cf->pc->wall & popped) {
		bombPoppedType[BOMB_COUNT][opponentTurn] = DROPTYPE_WALL;
		goto popper;
	}
	if (cf->board[opponentTurn] & cf->pc->x2 & popped) {
		bombPoppedType[BOMB_COUNT][opponentTurn] = DROPTYPE_X2;
		goto popper;
	}
	if (cf->board[opponentTurn] & popped) {
		bombPoppedType[BOMB_COUNT][opponentTurn] = DROPTYPE_NORMAL;
		goto popper;
	}
	goto invalid;
popper:
	cf->board[0] = ((cf->board[0] & (ALL ^ falls)) | ((cf->board[0] & falls) >> 1ull)) & ALL;
	cf->board[1] = ((cf->board[1] & (ALL ^ falls)) | ((cf->board[1] & falls) >> 1ull)) & ALL;
	cf->pc->anvil = ((cf->pc->anvil & (ALL ^ falls)) | ((cf->pc->anvil & falls) >> 1ull)) & ALL;
	cf->pc->bomb = ((cf->pc->bomb & (ALL ^ falls)) | ((cf->pc->bomb & falls) >> 1ull)) & ALL;
	cf->pc->wall = ((cf->pc->wall & (ALL ^ falls)) | ((cf->pc->wall & falls) >> 1ull)) & ALL;
	cf->pc->x2 = ((cf->pc->x2 & (ALL ^ falls)) | ((cf->pc->x2 & falls) >> 1ull)) & ALL;
	--cf->height[COLUMN];
	return true;
invalid:
	return false;
}

void ConnectFour_powerup_unpop(ConnectFour *cf) {
	unsigned turn = --cf->plyNumber & 1u, opponentTurn = !turn;
	unsigned short bombCount = turn ? (cf->playedPowerCheckers & PLAYER2_BOMBMASK) >> PLAYER2_BOMBSHIFT : (cf->playedPowerCheckers & PLAYER1_BOMBMASK) >> PLAYER1_BOMBSHIFT;
	uint8_t popper = cf->pum[cf->plyNumber].normalColumn;
	Position bottomDisk = (Position)popper * ROWS_P1, falls = COLS << bottomDisk;
	cf->board[0] = (cf->board[0] & (ALL ^ falls)) | (cf->board[0] & falls) << 1ull;
	cf->board[1] = (cf->board[1] & (ALL ^ falls)) | (cf->board[1] & falls) << 1ull;
	cf->pc->anvil = (cf->pc->anvil & (ALL ^ falls)) | (cf->pc->anvil & falls) << 1ull;
	cf->pc->bomb = (cf->pc->bomb & (ALL ^ falls)) | (cf->pc->bomb & falls) << 1ull;
	cf->pc->wall = (cf->pc->wall & (ALL ^ falls)) | (cf->pc->wall & falls) << 1ull;
	cf->pc->x2 = (cf->pc->x2 & (ALL ^ falls)) | (cf->pc->x2 & falls) << 1ull;
	++cf->height[popper];
	cf->board[opponentTurn] |= (Position)1ull << bottomDisk;
	switch (bombPoppedType[--bombCount][opponentTurn]) {
	case DROPTYPE_ANVIL:
		cf->pc->anvil|= (Position)1ull << bottomDisk;
		break;
	case DROPTYPE_BOMB:
		cf->pc->bomb |= (Position)1ull << bottomDisk;
		break;
	case DROPTYPE_WALL:
		cf->pc->wall |= (Position)1ull << bottomDisk;
		break;
	case DROPTYPE_X2:
		cf->pc->x2 |= (Position)1ull << bottomDisk;
	}
}

void ConnectFour_powerup_incrementPlayedPowerCheckers(ConnectFour *cf, const int TYPE) {
	unsigned short anvils, bombs, walls, x2s, others;
	unsigned turn = cf->plyNumber & 1u;
	switch (TYPE) {
	case DROPTYPE_ANVIL:
		anvils = turn ? (cf->playedPowerCheckers & PLAYER2_ANVILMASK) >> PLAYER2_ANVILSHIFT : (cf->playedPowerCheckers & PLAYER1_ANVILMASK) >> PLAYER1_ANVILSHIFT;
		others = turn ? cf->playedPowerCheckers & ~PLAYER2_ANVILMASK : cf->playedPowerCheckers & ~PLAYER1_ANVILMASK;
		cf->playedPowerCheckers = others | (++anvils << (turn ? PLAYER2_ANVILSHIFT : PLAYER1_ANVILSHIFT));
		break;
	case DROPTYPE_BOMB:
		bombs = turn ? (cf->playedPowerCheckers & PLAYER2_BOMBMASK) >> PLAYER2_BOMBSHIFT : (cf->playedPowerCheckers & PLAYER1_BOMBMASK) >> PLAYER1_BOMBSHIFT;
		others = turn ? cf->playedPowerCheckers & ~PLAYER2_BOMBMASK : cf->playedPowerCheckers & ~PLAYER1_BOMBMASK;
		cf->playedPowerCheckers = others | (++bombs << (turn ? PLAYER2_BOMBSHIFT : PLAYER1_BOMBSHIFT));
		break;
	case DROPTYPE_WALL:
		walls = turn ? (cf->playedPowerCheckers & PLAYER2_WALLMASK) >> PLAYER2_WALLSHIFT : (cf->playedPowerCheckers & PLAYER1_WALLMASK) >> PLAYER1_WALLSHIFT;
		others = turn ? cf->playedPowerCheckers & ~PLAYER2_WALLMASK : cf->playedPowerCheckers & ~PLAYER1_WALLMASK;
		cf->playedPowerCheckers = others | (++walls << (turn ? PLAYER2_WALLSHIFT : PLAYER1_WALLSHIFT));
		break;
	case DROPTYPE_X2:
		x2s = turn ? (cf->playedPowerCheckers & PLAYER2_X2MASK) >> PLAYER2_X2SHIFT : (cf->playedPowerCheckers & PLAYER1_X2MASK) >> PLAYER1_X2SHIFT;
		others = turn ? cf->playedPowerCheckers & ~PLAYER2_X2MASK : cf->playedPowerCheckers & ~PLAYER1_X2MASK;
		cf->playedPowerCheckers = others | (++x2s << (turn ? PLAYER2_X2SHIFT : PLAYER1_X2SHIFT));
	}
}

void ConnectFour_powerup_decrementPlayedPowerCheckers(ConnectFour *cf) {
	unsigned short anvils, bombs, walls, x2s, others;
	unsigned turn = cf->plyNumber & 1u;
	switch (cf->pum[cf->plyNumber].diskType) {
	case DROPTYPE_ANVIL:
		anvils = turn ? (cf->playedPowerCheckers & PLAYER2_ANVILMASK) >> PLAYER2_ANVILSHIFT : (cf->playedPowerCheckers & PLAYER1_ANVILMASK) >> PLAYER1_ANVILSHIFT;
		others = turn ? cf->playedPowerCheckers & ~PLAYER2_ANVILMASK : cf->playedPowerCheckers & ~PLAYER1_ANVILMASK;
		cf->playedPowerCheckers = others | (--anvils << (turn ? PLAYER2_ANVILSHIFT : PLAYER1_ANVILSHIFT));
		break;
	case DROPTYPE_BOMB:
		bombs = turn ? (cf->playedPowerCheckers & PLAYER2_BOMBMASK) >> PLAYER2_BOMBSHIFT : (cf->playedPowerCheckers & PLAYER1_BOMBMASK) >> PLAYER1_BOMBSHIFT;
		others = turn ? cf->playedPowerCheckers & ~PLAYER2_BOMBMASK : cf->playedPowerCheckers & ~PLAYER1_BOMBMASK;
		cf->playedPowerCheckers = others | (--bombs << (turn ? PLAYER2_BOMBSHIFT : PLAYER1_BOMBSHIFT));
		break;
	case DROPTYPE_WALL:
		walls = turn ? (cf->playedPowerCheckers & PLAYER2_WALLMASK) >> PLAYER2_WALLSHIFT : (cf->playedPowerCheckers & PLAYER1_WALLMASK) >> PLAYER1_WALLSHIFT;
		others = turn ? cf->playedPowerCheckers & ~PLAYER2_WALLMASK : cf->playedPowerCheckers & ~PLAYER1_WALLMASK;
		cf->playedPowerCheckers = others | (--walls << (turn ? PLAYER2_WALLSHIFT : PLAYER1_WALLSHIFT));
		break;
	case DROPTYPE_X2:
		x2s = turn ? (cf->playedPowerCheckers & PLAYER2_X2MASK) >> PLAYER2_X2SHIFT : (cf->playedPowerCheckers & PLAYER1_X2MASK) >> PLAYER1_X2SHIFT;
		others = turn ? cf->playedPowerCheckers & ~PLAYER2_X2MASK : cf->playedPowerCheckers & ~PLAYER1_X2MASK;
		cf->playedPowerCheckers = others | (--x2s << (turn ? PLAYER2_X2SHIFT : PLAYER1_X2SHIFT));
	}
}

bool ConnectFour_popten_drop(ConnectFour *cf, const int COLUMN) {
	Position dropper = (Position)1ull << cf->height[COLUMN];
	if (!(dropper & TOP) && cf->ptm[cf->plyNumber].size && popTenFlags != POPTEN_DROP) {
		if ((cf->board[cf->plyNumber & 1u] & BOT) || popTenFlags != POPTEN_DROP) {
			uint8_t lastPop = cf->ptm[cf->plyNumber].pops[cf->ptm[cf->plyNumber].size - 1u];
			if (lastPop != POPTEN_PASS) {
				lastPop = (lastPop - COLUMNS) & 0b111111;
				if (ConnectFour_numberLegalDrops(cf) == 1u || lastPop != COLUMN) {
					ConnectFour_popten_removeDisk(cf);
					popTenFlags = POPTEN_DROP;
					cf->board[cf->plyNumber & 1u] |= dropper;
					++cf->height[COLUMN];
					cf->ptm[cf->plyNumber].pops[cf->ptm[cf->plyNumber].size++] = COLUMN;
					if ((cf->plyNumber + 1u) < MOVESIZE) {
						cf->ptm[++cf->plyNumber].pops[cf->ptm[cf->plyNumber].size++] = popTenFlags;
						return true;
					}
				}
			}
		}
	}
	return false;
}

void ConnectFour_popten_undrop(ConnectFour *cf) {
	if (cf->ptm[--cf->plyNumber].size) {
		unsigned vectorSize = cf->ptm[cf->plyNumber].size - 1ull;
		uint8_t lastDrop = cf->ptm[cf->plyNumber].pops[vectorSize] & 0b111111;
		cf->board[cf->plyNumber & 1u] ^= (Position)1ull << --cf->height[lastDrop];
		popTenFlags = cf->ptm[cf->plyNumber].pops[vectorSize - 1ull] & 0b11000000;
		--cf->ptm[cf->plyNumber].size;
		--cf->ptm[cf->plyNumber + 1u].size;
		ConnectFour_popten_addDisk(cf);
	}
}

bool ConnectFour_popten_pop(ConnectFour *cf, const int COLUMN) {
	Position bottomDisk = (Position)COLUMN * ROWS_P1, botDiskBit = (Position)1ull << bottomDisk, falls = ALLCOLS << bottomDisk;
	if ((cf->board[cf->plyNumber & 1u] & botDiskBit) && popTenFlags != POPTEN_POP_NO_CONNECTION) {
		popTenFlags = POPTEN_POP_NO_CONNECTION;
		if (ConnectFour_popten_connection(cf->board[cf->plyNumber & 1u]) & botDiskBit) {
			popTenFlags = POPTEN_POP_CONNECTION;
		}
		cf->board[0] = ((cf->board[0] & (ALL ^ falls)) | ((cf->board[0] & falls) >> 1ull)) & ALL;
		cf->board[1] = ((cf->board[1] & (ALL ^ falls)) | ((cf->board[1] & falls) >> 1ull)) & ALL;
		--cf->height[COLUMN];
		cf->ptm[cf->plyNumber].pops[cf->ptm[cf->plyNumber].size++] = (COLUMN + COLUMNS) | popTenFlags;
		ConnectFour_popten_addDisk(cf);
		return true;
	}
	return false;
}

void ConnectFour_popten_unpop(ConnectFour *cf) {
	if (cf->ptm[cf->plyNumber].size) {
		unsigned vectorSize = cf->ptm[cf->plyNumber].size - 1ull;
		uint8_t lastPop = (cf->ptm[cf->plyNumber].pops[vectorSize] - COLUMNS) & 0b111111;
		Position bottomDisk = (Position)lastPop * ROWS_P1, falls = ALLCOLS << bottomDisk;
		cf->board[0] = (cf->board[0] & (ALL ^ falls)) | (cf->board[0] & falls) << 1ull;
		cf->board[1] = (cf->board[1] & (ALL ^ falls)) | (cf->board[1] & falls) << 1ull;
		cf->board[cf->plyNumber & 1u] |= (Position)1ull << bottomDisk;
		++cf->height[lastPop];
		popTenFlags = cf->ptm[cf->plyNumber].pops[vectorSize - 1ull] & 0b11000000;
		--cf->ptm[cf->plyNumber].size;
		ConnectFour_popten_removeDisk(cf);
	}
}

bool ConnectFour_popten_pass(ConnectFour *cf) {
	if (!(cf->board[cf->plyNumber & 1] & BOT) && popTenFlags == POPTEN_DROP) {
		popTenFlags = POPTEN_PASS;
		cf->ptm[cf->plyNumber].pops[cf->ptm[cf->plyNumber].size++] = 'P';
		if ((cf->plyNumber + 1u) < MOVESIZE) {
			cf->ptm[++cf->plyNumber].pops[cf->ptm[cf->plyNumber].size++] = popTenFlags;
			return true;
		}
	}
	return false;
}

void ConnectFour_popten_unpass(ConnectFour *cf) {
	--cf->ptm[cf->plyNumber].size;
	if (cf->ptm[--cf->plyNumber].size) {
		popTenFlags = cf->ptm[cf->plyNumber].pops[--cf->ptm[cf->plyNumber].size - 1u] & 0b11000000;
	}
}

void ConnectFour_popten_addDisk(ConnectFour *cf) {
	cf->collectedDisks = (cf->plyNumber & 1u) ? cf->collectedDisks + 0x10u : cf->collectedDisks + 1u;
}

void ConnectFour_popten_removeDisk(ConnectFour *cf) {
	cf->collectedDisks = (cf->plyNumber & 1u) ? cf->collectedDisks - 0x10u : cf->collectedDisks - 1u;
}

bool ConnectFour_play(ConnectFour *cf, const char SEQ) {
	if (!ConnectFour_gameOver(cf)) {
		bool moveSuccess;
		unsigned char column = SEQ - '1';
		if (column >= 0 && column < COLUMNS) {
			switch (GAME_VARIANT) {
			case POPOUT_VARIANT:
				return ConnectFour_popout_drop(cf, column);
			case POPTEN_VARIANT:
				return ConnectFour_popten_drop(cf, column);
			case POWERUP_VARIANT:
				switch (userPowerUpDiskType) {
				case DROPTYPE_NORMAL:
					return ConnectFour_powerup_drop(cf, column);
				case DROPTYPE_ANVIL:
					if ((moveSuccess = ConnectFour_powerup_dropAnvil(cf, column))) {
						userPowerUpDiskType = DROPTYPE_NORMAL;
					}
					return moveSuccess;
				case DROPTYPE_BOMB:
					if (userPowerCheckerReadyToMoveFlag) {
						userPowerCheckerReadyToMoveFlag = 0;
						if ((moveSuccess = ConnectFour_powerup_dropBomb(cf, userPowerCheckerColumnBuffer, column))) {
							userPowerUpDiskType = DROPTYPE_NORMAL;
						}
						return moveSuccess;
					}
					else {
						userPowerCheckerColumnBuffer = column;
						userPowerCheckerReadyToMoveFlag = 1;
					}
					return true;
				case DROPTYPE_WALL:
					if (userPowerCheckerReadyToMoveFlag) {
						userPowerCheckerReadyToMoveFlag = 0;
						if ((moveSuccess = ConnectFour_powerup_dropWall(cf, userPowerCheckerColumnBuffer, column))) {
							userPowerUpDiskType = DROPTYPE_NORMAL;
						}
						return moveSuccess;
					}
					else {
						userPowerCheckerColumnBuffer = column;
						userPowerCheckerReadyToMoveFlag = 1;
					}
					return true;
				case DROPTYPE_X2:
					if (userPowerCheckerReadyToMoveFlag) {
						userPowerCheckerReadyToMoveFlag = 0;
						if ((moveSuccess = ConnectFour_powerup_dropX2(cf, userPowerCheckerColumnBuffer, column))) {
							userPowerUpDiskType = DROPTYPE_NORMAL;
						}
						return moveSuccess;
					}
					else {
						userPowerCheckerColumnBuffer = column;
						userPowerCheckerReadyToMoveFlag = 1;
					}
					return true;
				}
			default:
				return ConnectFour_normal_drop(cf, column);
			}
		}
		if (GAME_VARIANT == POWERUP_VARIANT) {
			switch (SEQ) {
			case 'A':
			case 'a':
				userPowerUpDiskType = DROPTYPE_ANVIL;
				return true;
			case 'B':
			case 'b':
				userPowerUpDiskType = DROPTYPE_BOMB;
				return true;
			case 'W':
			case 'w':
				userPowerUpDiskType = DROPTYPE_WALL;
				return true;
			case 'X':
			case 'x':
				userPowerUpDiskType = DROPTYPE_X2;
				return true;
			}
		}
		else {
			column = SEQ - 'A';
			for (int i = 0; i < 2; ++i) {
				if (column >= 0 && column < COLUMNS) {
					switch (GAME_VARIANT) {
					case POPOUT_VARIANT:
						return ConnectFour_popout_pop(cf, column);
					case POPTEN_VARIANT:
						return ConnectFour_popten_pop(cf, column);
					default:
						return false;
					}
				}
				column = SEQ - 'a';
			}
			if ((GAME_VARIANT == POPTEN_VARIANT) && (SEQ == 'P' || SEQ == 'p')) {
				return ConnectFour_popten_pass(cf);
			}
			return false;
		}
	}
	return false;
}

bool ConnectFour_sequence(ConnectFour *cf, const char *SEQ) {
	for (int i = 0; SEQ[i]; ++i) {
		if (!ConnectFour_play(cf, SEQ[i])) {
			return false;
		}
	}
	return true;
}

unsigned ConnectFour_numberLegalMoves(const ConnectFour *cf) {
	return ConnectFour_numberLegalDrops(cf) + ConnectFour_numberLegalPops(cf);
}

unsigned ConnectFour_numberLegalDrops(const ConnectFour *cf) {
	return popcount(ALL & ((cf->board[0] | cf->board[1]) + BOT));
}

unsigned ConnectFour_numberLegalPops(const ConnectFour *cf) {
	return popcount(cf->board[cf->plyNumber & 1u] & BOT);
}

Position ConnectFour_bottom(void) {
	Position bot, column;
	for (bot = column = 0; column < COLUMNS; ++column) {
		bot |= (Position)1ull << column * ROWS_P1;
	}
	return bot;
}

Position ConnectFour_reverse(Position pos) {
	Position revPos, revCols;
	for (revPos = 0, revCols = COLUMNS_M1; pos; pos >>= ROWS_P1, --revCols) {
		revPos |= (pos & COLS) << revCols * ROWS_P1;
	}
 	return revPos;
}

Position ConnectFour_getHashKey(const ConnectFour *cf) {
	return cf->board[cf->plyNumber & 1u] + cf->board[0] + cf->board[1] + BOT;
}

Position ConnectFour_getPowerUpHashKey(const ConnectFour *cf, const Position POWER_CHECKERS) {
	return cf->board[cf->plyNumber & 1u] + POWER_CHECKERS + BOT;
}

unsigned ConnectFour_countBottomDisksFromHashKey(Position hash) {
	unsigned count = 0u;
	for (; hash; hash >>= COLUMNS) {
		if (hash & 1ull && hash & (COLS - 1ull)) {
			++count;
		}
	}
	return count;
}

void ConnectFour_reverseBoard(ConnectFour *cf) {
	cf->board[0] = ConnectFour_reverse(cf->board[0]);
	cf->board[1] = ConnectFour_reverse(cf->board[1]);
	if (GAME_VARIANT == POWERUP_VARIANT) {
		cf->pc->anvil = ConnectFour_reverse(cf->pc->anvil);
		cf->pc->bomb = ConnectFour_reverse(cf->pc->bomb);
		cf->pc->wall = ConnectFour_reverse(cf->pc->wall);
		cf->pc->x2 = ConnectFour_reverse(cf->pc->x2);
	}
	for (unsigned i = 0; i < COLUMNS; ++i) {
		cf->height[i] = ROWS_P1 * i + popcount(COLS << i * ROWS_P1 & (cf->board[0] | cf->board[1]));
	}
}

unsigned ConnectFour_getMoveSize(void) {
	switch (GAME_VARIANT) {
	case POPOUT_VARIANT:
	case POPTEN_VARIANT:
		return COLUMNS_X2;
	default:
		return COLUMNS;
	}
}

void ConnectFour_displayHelpMessage(void) {
	puts("FourTheWin -help | -bench | -book | -generate [LENGTH] [MOVES] | [VARIANT] |\n-size [COLUMNS] [ROWS] | [MOVES]\n");
	puts("Solves the game of Connect Four and its official variants.\nFour the Win! accepts the following command-line switches below:\n");
	puts("Switches\tDescription");
	puts("_______________________________________________________________________________\n");
	puts("-help\t\tPrints this detailed help message to the console and then exit.\n");
	puts("-bench\t\tPerforms a simple benchmark test on a billion positions. The");
	puts("\t\ttest displays how many positions per second were examined at");
	puts("\t\tthe initial position.\n");
	puts("-book\t\tUses an opening book. It is created if it does not exist. If");
	puts("\t\tnot, Four the Win! will append the solution to the opening book");
	puts("\t\tafter solving. Opening books are unsupported for the Pop Ten");
	puts("\t\tvariant.\n");
	puts("-generate\tGenerates the opening book within the specified [LENGTH] in");
	puts("\t\tposition [MOVES]. Four the Win! exits on completion. This");
	puts("\t\tswitch implies -book.\n");
	puts("-size\t\tSets the size of the board in [COLUMNS]x[ROWS]. Four the Win!");
	puts("\t\twill accept sizes from 4x4 to 16x16. If no size or an invaild");
	puts("\t\tsize arguments were given, then it is set to the standard size.\n");
	puts("Parameters\tDescription");
	puts("_______________________________________________________________________________\n");
	puts("[LENGTH]\tThe move length in which the opening book will generate.\n");
	puts("[MOVES]\t\tPlay a sequence of moves from an empty position up until the");
	puts("\t\tend of the move sequence. Assuming a size less than ten columns");
	puts("\t\twide, drop moves range from leftmost 1 to rightmost 9, and pop");
	puts("\t\tmoves from A to I respectively. Pop moves are case-insensitive.\n");
	puts("[VARIANT]\tSelects a variant. If no variant or an invalid variant is");
	puts("\t\tpassed in Four the Win!, then the normal variant is used.");
	puts("\t\tOtherwise, it must be one of the five switches: -normal");
	puts("\t\t-popout -powerup -popten -fiveinarow\n");
	puts("[COLUMNS]\tThe number of columns, or the width of the board.\n");
	puts("[ROWS]\t\tThe number of rows, or the height of the board.\n");
	puts("Connect Four, Connect 4, and related terms are trademarks of Hasbro, Inc.");
}

uint8_t ConnectFour_previousHistory(const uint8_t current) {
	return !(current) ? HISTORYSIZE - 1u : current - 1u;
}

uint8_t ConnectFour_nextHistory(const uint8_t current) {
	return current + 1u < HISTORYSIZE ? current + 1u : 0u;
}

void ConnectFour_addHistory(ConnectFour *cf) {
	cf->history[cf->historyIndex] = ConnectFour_getHashKey(cf);
	cf->historyIndex = ConnectFour_nextHistory(cf->historyIndex);
}

void ConnectFour_clearHistory(ConnectFour *cf) {
	cf->historyIndex = 0;
	ConnectFour_addHistory(cf);
	for (unsigned i = 1; i < HISTORYSIZE; ++i) {
		cf->history[i] = 0;
	}
}

void ConnectFour_benchmark(ConnectFour *cf, const unsigned COUNT) {
	double benchSeconds, benchPPS;
	printf("Benchmarking with %u positions...\n", COUNT);
	counter = 0u;
	clock_t benchTime = clock();
	switch (GAME_VARIANT) {
	case POPOUT_VARIANT:
		ConnectFour_popout_benchmark(cf, COUNT);
		break;
	case POWERUP_VARIANT:
		ConnectFour_powerup_benchmark(cf, COUNT);
		break;
	case POPTEN_VARIANT:
		ConnectFour_popten_benchmark(cf, COUNT);
		break;
	case FIVEINAROW_VARIANT:
		ConnectFour_fiveinarow_benchmark(cf, COUNT);
		break;
	default:
		ConnectFour_normal_benchmark(cf, COUNT);
	}
	benchTime = clock() - benchTime;
	benchSeconds = (double)benchTime / CLOCKS_PER_SEC;
	benchPPS = (double)COUNT / benchSeconds;
	printf("Benchmark completed in %.3lf %s\nMove generation speed: %.3lf positions per seconds\n", benchSeconds, (benchSeconds == 1.0) ? "second" : "seconds", benchPPS);
}

void ConnectFour_normal_benchmark(ConnectFour *cf, const unsigned COUNT) {
	if (!ConnectFour_connection(cf->board[cf->plyNumber & 1u]) && (counter++ < COUNT)) {
		unsigned i;
		for (i = 0; i < COLUMNS; ++i) {
			if (ConnectFour_normal_drop(cf, i)) {
				ConnectFour_normal_benchmark(cf, COUNT);
				ConnectFour_normal_undrop(cf);
			}
		}
	}
}

void ConnectFour_popout_benchmark(ConnectFour *cf, const unsigned COUNT) {
	if (!ConnectFour_connection(cf->board[cf->plyNumber & 1u]) && (counter++ < COUNT) && (cf->plyNumber < MOVESIZE)) {
		unsigned i;
		for (i = 0; i < COLUMNS; ++i) {
			if (ConnectFour_popout_drop(cf, i)) {
				ConnectFour_popout_benchmark(cf, COUNT);
				ConnectFour_popout_undrop(cf);
			}
		}
		for (i = 0; i < COLUMNS; ++i) {
			if (ConnectFour_popout_pop(cf, i)) {
				ConnectFour_popout_benchmark(cf, COUNT);
				ConnectFour_popout_unpop(cf);
			}
		}
	}
}

void ConnectFour_powerup_benchmark(ConnectFour *cf, const unsigned COUNT) {
	if (!ConnectFour_connection(cf->board[cf->plyNumber & 1u]) && (counter++ < COUNT) && (cf->plyNumber < MOVESIZE)) {
		unsigned i, j;
		for (i = 0; i < COLUMNS; ++i) {
			if (ConnectFour_powerup_drop(cf, i)) {
				ConnectFour_powerup_benchmark(cf, COUNT);
				ConnectFour_powerup_undrop(cf);
			}
		}
		for (i = 0; i < COLUMNS; ++i) {
			if (ConnectFour_powerup_dropAnvil(cf, i)) {
				ConnectFour_powerup_benchmark(cf, COUNT);
				ConnectFour_powerup_undropAnvil(cf);
			}
			for (j = 0; j < COLUMNS; ++j) {
				if (ConnectFour_powerup_dropBomb(cf, i, j)) {
					ConnectFour_powerup_benchmark(cf, COUNT);
					ConnectFour_powerup_undropBomb(cf);
				}
				if (ConnectFour_powerup_dropWall(cf, i, j)) {
					ConnectFour_powerup_benchmark(cf, COUNT);
					ConnectFour_powerup_undropWall(cf);
				}
				if (ConnectFour_powerup_dropX2(cf, i, j)) {
					ConnectFour_powerup_benchmark(cf, COUNT);
					ConnectFour_powerup_undropX2(cf);
				}
			}
		}
	}
}

void ConnectFour_popten_benchmark(ConnectFour *cf, const unsigned COUNT) {
	if (!ConnectFour_hasTenDisks(cf) && (counter++ < COUNT) && (cf->plyNumber < MOVESIZE)) {
		unsigned i;
		for (i = 0; i < COLUMNS; ++i) {
			if (ConnectFour_popten_pop(cf, i)) {
				ConnectFour_popten_benchmark(cf, COUNT);
				ConnectFour_popten_unpop(cf);
			}
		}
		for (i = 0; i < COLUMNS; ++i) {
			if (ConnectFour_popten_drop(cf, i)) {
				ConnectFour_popten_benchmark(cf, COUNT);
				ConnectFour_popten_undrop(cf);
			}
		}
		if (ConnectFour_popten_pass(cf)) {
			ConnectFour_popten_benchmark(cf, COUNT);
			ConnectFour_popten_unpass(cf);
		}
	}
}

void ConnectFour_fiveinarow_benchmark(ConnectFour *cf, const unsigned COUNT) {
	if (!ConnectFour_fiveinarow_connection(cf->board[cf->plyNumber & 1u]) && (counter++ < COUNT)) {
		unsigned i;
		for (i = 0; i < COLUMNS; ++i) {
			if (ConnectFour_normal_drop(cf, i)) {
				ConnectFour_normal_benchmark(cf, COUNT);
				ConnectFour_normal_undrop(cf);
			}
		}
	}
}
