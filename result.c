/*
	Copyright (C) 2019- TheTrustedComputer
*/

#include "result.h"

void Result_print(Result *current, Result *best) {
#if defined(_WIN32) || defined(_WIN64) // For compilation under Windows

#endif
	switch (current->wdl) {
	default:
	case UNKNOWN_CHAR:
	case '\0':
		printf(NONE_TEXT);
		break;
	case DRAW_CHAR:
		if (best && best->wdl == current->wdl) {
			printf("\e[1;33m%c\e[0m ", DRAW_CHAR);
		}
		else {
			printf("\e[0;33m%c\e[0m ", DRAW_CHAR);
		}
		break;
	case WIN_CHAR:
		if (current->dtc) {
			if (best && best->dtc == current->dtc && best->wdl == current->wdl) {
				printf("\e[1;32m%c%d\e[0m ", current->wdl, current->dtc);
			}
			else {
				printf("\e[0;32m%c%d\e[0m ", current->wdl, current->dtc);
			}
		}
		else {
			if (best && best->dtc == current->dtc && best->wdl == current->wdl) {
				printf("\e[1;32m%s\e[0m ", WIN_TEXT);
			}
			else {
				printf("\e[0;32m%s\e[0m ", WIN_TEXT);
			}
		}
		break;
	case LOSS_CHAR:
		if (current->dtc) {
			if (best && best->dtc == current->dtc && best->wdl == current->wdl) {
				printf("\e[1;31m%c%d\e[0m ", current->wdl, current->dtc);
			}
			else {
				printf("\e[0;31m%c%d\e[0m ", current->wdl, current->dtc);
			}
		}
		else {
			if (best && best->dtc == current->dtc && best->wdl == current->wdl) {
				printf("\e[1;31m%s\e[0m ", LOSS_TEXT);
			}
			else {
				printf("\e[0;31m%s\e[0m ", LOSS_TEXT);
			}
		}
	}
}

void Result_increment(Result *result) {
	switch (result->wdl) {
	case WIN_CHAR:
		result->wdl = LOSS_CHAR;
		++result->dtc;
		break;
	case LOSS_CHAR:
		result->wdl = WIN_CHAR;
		++result->dtc;
	}
}

Result Result_getBestResult(Result *results, unsigned resultSize) {
	unsigned i, score;
	Result best = { LOSS_CHAR, 0 };
	for (i = score = 0; i < resultSize; ++i) {
		switch (score) {
		case 0:
			switch (results[i].wdl) {
			case LOSS_CHAR:
				if (best.dtc < results[i].dtc) {
					best.dtc = results[i].dtc;
				}
				break;
			case DRAW_CHAR:
				best = results[i];
				score = 1;
				break;
			case WIN_CHAR:
				best = results[i];
				score = 2;
			}
			break;
		case 1:
			switch (results[i].wdl) {
			case WIN_CHAR:
				best = results[i];
				score = 2;
			}
			break;
		case 2:
			if (results[i].wdl == WIN_CHAR && best.dtc > results[i].dtc) {
				best.dtc = results[i].dtc;
			}
		}
	}
	return best;
}

unsigned Result_getBestMove(Result *results, unsigned resultSize) {
	unsigned i, d, *bestMoves = GAME_VARIANT == POPOUT_VARIANT || GAME_VARIANT == POPTEN_VARIANT ? malloc(sizeof(int) * COLUMNS_X2) : malloc(sizeof(int) * COLUMNS), best;
	init_genrand64(time(NULL));
	Result bestResult = Result_getBestResult(results, resultSize);
	for (i = d = 0; i < resultSize; ++i) {
		if (bestResult.wdl == results[i].wdl && bestResult.dtc == results[i].dtc) {
			bestMoves[d++] = i;
		}
	}
	best = d ? bestMoves[genrand64_int64() % d] : INT32_MAX;
	free(bestMoves);
	return best;
}

void Result_normal_reverse(Result *results) {
	for (unsigned i = 0; i < COLUMNS_D2; ++i) {
		Result reverseTemp = results[i];
		results[i] = results[COLUMNS - 1 - i];
		results[COLUMNS - 1 - i] = reverseTemp;
	}
}

void Result_popout_reverse(Result *results) {
	for (unsigned i = COLUMNS; i < COLUMNS + COLUMNS_D2; ++i) {
		Result reverseTemp = results[i];
		results[i] = results[COLUMNS_X2 - 1 - (i - COLUMNS)];
		results[COLUMNS_X2 - 1 - (i - COLUMNS)] = reverseTemp;
	}
}
