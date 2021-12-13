#include "alphabeta.h"

void AlphaBeta_getColumnMoveOrder(void) {
	for (int i = 0; i < (int)COLUMNS; ++i) {
		moveOrder[i] = COLUMNS / 2 + (1 - 2 * (i & 1)) * (i + 1) / 2;
	}
}

bool AlphaBeta_normal_checkWin(ConnectFour *cf) {
	unsigned i;
	for (i = 0; i < COLUMNS; ++i) {
		if (ConnectFour_connection(cf->board[cf->plyNumber & 1u] | (Position)1ull << cf->height[i] & ALL)) {
			return true;
		}
	}
	return false;
}

bool AlphaBeta_normal_checkSeveralWaysToWin(ConnectFour *cf) {
	Position forced = (ALL & (cf->board[0] | cf->board[1]) + BOT) & ConnectFour_calculateAllThreats(cf->board[!(cf->plyNumber & 1u)]);
	return forced && forced & (forced - 1ull);
}

bool AlphaBeta_popout_checkWin(ConnectFour *cf) {
	unsigned i;
	for (i = 0; i < COLUMNS; ++i) {
		if (ConnectFour_connection(cf->board[cf->plyNumber & 1] | (Position)1ull << cf->height[i] & ALL)) {
			return true;
		}
		if (ConnectFour_popout_popWinCheck(cf, i)) {
			if (ConnectFour_connectionNoVertical(cf->board[cf->plyNumber & 1])) {
				ConnectFour_popout_unpopWinCheck(cf, i);
				return true;
			}
			ConnectFour_popout_unpopWinCheck(cf, i);
		}
	}
	return false;
}

bool AlphaBeta_powerup_checkWin(ConnectFour *cf) {
	unsigned i, j, turn = cf->plyNumber & 1;
	Position oldBoard[2] = { cf->board[0], cf->board[1] };
	for (i = 0; i < COLUMNS; ++i) {
		if (ConnectFour_connection(cf->board[turn] | (Position)1ull << cf->height[i] & ALL)) {
			return true;
		}
		if (ConnectFour_powerup_dropAnvil(cf, i)) {
			if (ConnectFour_connectionNoVertical(cf->board[turn])) {
				ConnectFour_powerup_undropAnvil(cf);
				return true;
			}
			ConnectFour_powerup_undropAnvil(cf);
		}
		for (j = 0; j < COLUMNS; ++j) {
			if (ConnectFour_powerup_dropBomb(cf, i, j)) {
				if (ConnectFour_connectionNoVertical(cf->board[turn])) {
					ConnectFour_powerup_undropBomb(cf);
					return true;
				}
				ConnectFour_powerup_undropBomb(cf);
			}
			if (ConnectFour_powerup_dropX2(cf, i, j)) {
				if (ConnectFour_connection(cf->board[turn])) {
					ConnectFour_powerup_undropX2(cf);
					return true;
				}
				ConnectFour_powerup_undropX2(cf);
			}
		}
	}
	return false;
}

bool AlphaBeta_popten_checkWin(ConnectFour *cf) {
	for (unsigned i = 0; i < COLUMNS; ++i) {
		if (ConnectFour_popten_pop(cf, i)) {
			if (ConnectFour_hasTenDisks(cf)) {
				ConnectFour_popten_unpop(cf);
				return true;
			}
			ConnectFour_popten_unpop(cf);
		}
	}
	return false;
}

int AlphaBeta_negamax_normal(ConnectFour *cf, int depth, int alpha, int beta) {
	int parentScore, childScore;
	unsigned i;
	++nodes;
	if ((tableScore = TranspositionTable_normal_loadValue(&table, (hashKey = ConnectFour_getHashKey(cf))))) {
		return tableScore;
	}
	if (AlphaBeta_normal_checkWin(cf)) {
		return 1;
	}
	if (!depth) {
		return 0;
	}
	parentScore = alpha;
	for (i = 0; i < COLUMNS; ++i) {
		if (ConnectFour_normal_drop(cf, moveOrder[i])) {
			if ((childScore = -AlphaBeta_negamax_normal(cf, depth - 1, -beta, -alpha)) > parentScore) {
				parentScore = childScore;
			}
			ConnectFour_normal_undrop(cf);
			if (alpha < parentScore) {
				TranspositionTable_storeWithoutDepth(&table, (hashKey = ConnectFour_getHashKey(cf)), (alpha = parentScore));
			}
			if (alpha >= beta) {
				return alpha;
			}
		}
	}
	TranspositionTable_storeWithoutDepth(&table, (hashKey = ConnectFour_getHashKey(cf)), alpha);
	return alpha;
}

int AlphaBeta_negamax_normal_withMoves(ConnectFour *cf, int currDepth, int maxDepth, int alpha, int beta) {
	int parentScore, childScore;
	unsigned i;
	++nodes;
	if ((tableScore = TranspositionTable_normal_loadValue(&table, (hashKey = ConnectFour_getHashKey(cf))))) {
		switch (TranspositionTable_loadBounds(&table, hashKey)) {
		case TT_LOWERBOUND:
			if (alpha < tableScore) {
				alpha = tableScore;
			}
			if (alpha >= beta) {
				return alpha;
			}
			break;
		case TT_UPPERBOUND:
			if (beta > tableScore) {
				beta = tableScore;
			}
			if (alpha >= beta) {
				return beta;
			}
		}
	}
	for (i = 0; i < COLUMNS; ++i) {
		if (ConnectFour_connection(cf->board[cf->plyNumber & 1u] | (Position)1ull << cf->height[i] & ALL)) {
			pv[currDepth] = i + '1';
			return 1;
		}
	}
	if (currDepth >= maxDepth) {
		return 0;
	}
	parentScore = alpha;
	for (i = 0; i < COLUMNS; ++i) {
		if (ConnectFour_normal_drop(cf, moveOrder[i])) {
			if ((childScore = -AlphaBeta_negamax_normal_withMoves(cf, currDepth + 1, maxDepth, -beta, -alpha)) > parentScore) {
				pv[currDepth] = moveOrder[i] + '1';
				parentScore = childScore;
			}
			ConnectFour_normal_undrop(cf);
			if (alpha < parentScore) {
				TranspositionTable_storeBounds(&table, (hashKey = ConnectFour_getHashKey(cf)), (alpha = parentScore), maxDepth, TT_LOWERBOUND);
			}
			if (alpha >= beta) {
				return alpha;
			}
		}
	}
	TranspositionTable_storeBounds(&table, (hashKey = ConnectFour_getHashKey(cf)), alpha, maxDepth, TT_UPPERBOUND);
	return alpha;
}

int AlphaBeta_negamax_popout(ConnectFour *cf, int depth, int alpha, int beta) {
	int parentScore, childScore;
	unsigned i;
	++nodes;
	if (abs((tableScore = TranspositionTable_popout_loadValue(&table, (hashKey = ConnectFour_getHashKey(cf))))) >= PLAYER_WIN) {
		if (TranspositionTable_depthLessOrEqual(&table, hashKey, depth)) {
			return tableScore;
		}
	}
	if (AlphaBeta_popout_checkWin(cf)) {
		return PLAYER_WIN;
	}
	/*if (ConnectFour_repetition(cf)) {
		return -DRAW_OR_WIN;
		if (repetitionFlag) {
			uint8_t repetitionMove;
			int repetitionDepth = MOVESIZE - cf->plyNumber, repetitionScore, currentScore, opponentScore, * dropScores = malloc(sizeof(int) * COLUMNS), * popScores = malloc(sizeof(int) * COLUMNS), j;
			bool isPopMove = false;
			repetitionFlag = 0;
			ConnectFour_printBoard(cf);
			if ((repetitionMove = cf->moves[cf->plyNumber - 4u]) >= COLUMNS) {
				repetitionMove -= COLUMNS;
				isPopMove = true;
			}
			isPopMove ? (popScores[repetitionMove] = DRAW) : (dropScores[repetitionMove] = DRAW);
			for (i = 0; i < COLUMNS; ++i) {
				if (!(i == repetitionMove && isPopMove)) {
					popScores[i] = INT_MIN;
					if (ConnectFour_popout_pop(cf, i)) {
						ConnectFour_printBoard(cf);
						for (j = 0, currentScore = -PLAYER_WIN, opponentScore = DRAW_OR_WIN; j < repetitionDepth; ++j) {
							ConnectFour_clearHistory(cf);
							if ((repetitionScore = AlphaBeta_negamax_popout(cf, j, -PLAYER_WIN, PLAYER_WIN)) > currentScore) {
								currentScore = repetitionScore;
								popScores[i] = currentScore;
								if (abs(currentScore) >= PLAYER_WIN || abs(currentScore) == DRAW_OR_WIN) {
									break;
								}
							}
						}
						ConnectFour_popout_unpop(cf);
					}
				}
				if (!(i == repetitionMove && !isPopMove)) {
					dropScores[i] = INT_MIN;
					if (ConnectFour_popout_drop(cf, i)) {
						ConnectFour_printBoard(cf);
						for (j = 0, currentScore = -PLAYER_WIN, opponentScore = DRAW_OR_WIN; j < repetitionDepth; ++j) {
							ConnectFour_clearHistory(cf);
							if ((repetitionScore = AlphaBeta_negamax_popout(cf, j, -PLAYER_WIN, PLAYER_WIN)) > currentScore) {
								currentScore = repetitionScore;
								dropScores[i] = currentScore;
								if (abs(currentScore) >= PLAYER_WIN || abs(currentScore) == DRAW_OR_WIN) {
									break;
								}
							}
						}
						ConnectFour_popout_undrop(cf);
					}
				}
			}
			ConnectFour_printBoard(cf);
			repetitionScore = INT_MIN;
			for (i = 0; i < COLUMNS; ++i) {
				if (repetitionScore < dropScores[i]) {
					repetitionScore = dropScores[i];
				}
				if (repetitionScore < popScores[i]) {
					repetitionScore = popScores[i];
				}
			}
			repetitionFlag = 1;
			free(dropScores);
			free(popScores);
			return repetitionScore;
		}
		else {
			RepetitionTable_add(&rTable, hashKey);
			return -DRAW_OR_WIN;
		}
	}*/
	if (!depth) { // || cf->plyNumber >= MOVESIZE_M1) {
		return -PLAYER_PROGRESS;
	}
	parentScore = alpha;
	for (i = 0; i < COLUMNS; ++i) {
		if (ConnectFour_popout_drop(cf, moveOrder[i])) {
			if ((childScore = -AlphaBeta_negamax_popout(cf, depth - 1, -beta, -alpha)) > parentScore) {
				parentScore = childScore;
			}
			ConnectFour_popout_undrop(cf);
			if (alpha < parentScore) {
				TranspositionTable_store(&table, (hashKey = ConnectFour_getHashKey(cf)), (alpha = parentScore), depth);
			}
			if (alpha >= beta) {
				return alpha;
			}
		}
	}
	for (i = 0; i < COLUMNS; ++i) {
		if (ConnectFour_popout_pop(cf, moveOrder[i])) {
			if ((childScore = -AlphaBeta_negamax_popout(cf, depth - 1, -beta, -alpha)) > parentScore) {
				parentScore = childScore;
			}
			ConnectFour_popout_unpop(cf);
			if (alpha < parentScore) {
				TranspositionTable_store(&table, (hashKey = ConnectFour_getHashKey(cf)), (alpha = parentScore), depth);
			}
			if (alpha >= beta) {
				return alpha;
			}
		}
	}
	TranspositionTable_store(&table, (hashKey = ConnectFour_getHashKey(cf)), alpha, depth);
	return alpha;
}

int AlphaBeta_negamax_popout_withMoves(ConnectFour *cf, int currDepth, int maxDepth, int alpha, int beta) {
	int parentScore, childScore;
	unsigned i;
	++nodes;
	if ((tableScore = TranspositionTable_popout_loadValue(&table, (hashKey = ConnectFour_getHashKey(cf)))) != TT_POPOUT_UNKNOWN) {
		if (TranspositionTable_depthGreaterOrEqual(&table, hashKey, maxDepth)) {
			switch (TranspositionTable_loadBounds(&table, hashKey)) {
			case TT_LOWERBOUND:
				if (alpha < tableScore) {
					alpha = tableScore;
				}
				if (alpha >= beta) {
					return alpha;
				}
				break;
			case TT_UPPERBOUND:
				if (beta > tableScore) {
					beta = tableScore;
				}
				if (alpha >= beta) {
					return beta;
				}
			}
			
		}
	}
	for (i = 0; i < COLUMNS; ++i) {
		if (ConnectFour_connection(cf->board[cf->plyNumber & 1u] | (Position)1ull << cf->height[i] & ALL)) {
			pv[currDepth] = i + '1';
			return PLAYER_WIN;
		}
		if (ConnectFour_popout_popWinCheck(cf, i)) {
			if (ConnectFour_connectionNoVertical(cf->board[cf->plyNumber & 1u])) {
				ConnectFour_popout_unpopWinCheck(cf, i);
				pv[currDepth] = i + 'A';
				return PLAYER_WIN;
			}
			ConnectFour_popout_unpopWinCheck(cf, i);
		}
	}
	if (ConnectFour_repetition(cf)) {
		if (repetitionFlag) {
			repetitionFlag = 0;
			printf("%s", REPETITION_STRING);
		}
		return -DRAW_OR_WIN;
	}
	if (currDepth >= maxDepth) {
		return -PLAYER_PROGRESS;
	}
	parentScore = alpha;
	for (i = 0; i < COLUMNS; ++i) {
		if (ConnectFour_popout_drop(cf, moveOrder[i])) {
			if ((childScore = -AlphaBeta_negamax_popout_withMoves(cf, currDepth + 1, maxDepth, -beta, -alpha)) > parentScore) {
				pv[currDepth] = moveOrder[i] + '1';
				parentScore = childScore;
			}
			ConnectFour_popout_undrop(cf);
			if (alpha < parentScore) {
				TranspositionTable_storeBounds(&table, (hashKey = ConnectFour_getHashKey(cf)), (alpha = parentScore), maxDepth, TT_LOWERBOUND);
			}
			if (alpha >= beta) {
				return alpha;
			}
		}
	}
	for (i = 0; i < COLUMNS; ++i) {
		if (ConnectFour_popout_pop(cf, moveOrder[i])) {
			if ((childScore = -AlphaBeta_negamax_popout_withMoves(cf, currDepth + 1, maxDepth, -beta, -alpha)) > parentScore) {
				pv[currDepth] = moveOrder[i] + 'A';
				parentScore = childScore;
			}
			ConnectFour_popout_unpop(cf);
			if (alpha < parentScore) {
				TranspositionTable_storeBounds(&table, (hashKey = ConnectFour_getHashKey(cf)), (alpha = parentScore), maxDepth, TT_LOWERBOUND);
			}
			if (alpha >= beta) {
				return alpha;
			}
		}
	}
	TranspositionTable_storeBounds(&table, (hashKey = ConnectFour_getHashKey(cf)), alpha, maxDepth, TT_UPPERBOUND);
	return alpha;
}

int AlphaBeta_negamax_powerup(ConnectFour *cf, int depth, int alpha, int beta) {
	int parentScore, childScore;
	unsigned i, j;
	++nodes;
	if ((tableScore = TranspositionTable_powerup_loadValue(&table, (hashKey = ConnectFour_getHashKey(cf)), cf->pc->anvil, cf->pc->bomb, cf->pc->wall, cf->pc->x2))) {
		if (TranspositionTable_powerup_depthEquality(&table, hashKey, cf->pc->anvil, cf->pc->bomb, cf->pc->wall, cf->pc->x2, depth)) {
			return tableScore;
		}
	}
	if (AlphaBeta_powerup_checkWin(cf)) {
		return 1;
	}
	if (!depth || (cf->board[0] | cf->board[1]) == ALL) {
		return 0;
	}
	parentScore = alpha;
	for (i = 0; i < COLUMNS; ++i) {
		for (j = 0; j < COLUMNS; ++j) {
			if (ConnectFour_powerup_dropX2(cf, moveOrder[i], moveOrder[j])) {
				if ((childScore = -AlphaBeta_negamax_powerup(cf, depth - 1, -beta, -alpha)) > parentScore) {
					parentScore = childScore;
				}
				ConnectFour_powerup_undropX2(cf);
				if (alpha < parentScore) {
					TranspositionTable_powerup_store(&table, (hashKey = ConnectFour_getHashKey(cf)), cf->pc->anvil, cf->pc->bomb, cf->pc->wall, cf->pc->x2, (alpha = parentScore), depth);
				}
				if (alpha >= beta) {
					return alpha;
				}
			}
		}
	}
	for (i = 0; i < COLUMNS; ++i) {
		for (j = 0; j < COLUMNS; ++j) {
			if (ConnectFour_powerup_dropWall(cf, moveOrder[i], moveOrder[j])) {
				if ((childScore = -AlphaBeta_negamax_powerup(cf, depth - 1, -beta, -alpha)) > parentScore) {
					parentScore = childScore;
				}
				ConnectFour_powerup_undropWall(cf);
				if (alpha < parentScore) {
					TranspositionTable_powerup_store(&table, (hashKey = ConnectFour_getHashKey(cf)), cf->pc->anvil, cf->pc->bomb, cf->pc->wall, cf->pc->x2, (alpha = parentScore), depth);
				}
				if (alpha >= beta) {
					return alpha;
				}
			}
		}
	}
	for (i = 0; i < COLUMNS; ++i) {
		for (j = 0; j < COLUMNS; ++j) {
			if (ConnectFour_powerup_dropBomb(cf, moveOrder[i], moveOrder[j])) {
				if ((childScore = -AlphaBeta_negamax_powerup(cf, depth - 1, -beta, -alpha)) > parentScore) {
					parentScore = childScore;
				}
				ConnectFour_powerup_undropBomb(cf);
				if (alpha < parentScore) {
					TranspositionTable_powerup_store(&table, (hashKey = ConnectFour_getHashKey(cf)), cf->pc->anvil, cf->pc->bomb, cf->pc->wall, cf->pc->x2, (alpha = parentScore), depth);
				}
				if (alpha >= beta) {
					return alpha;
				}
			}
		}
	}
	for (i = 0; i < COLUMNS; ++i) {
		if (ConnectFour_powerup_dropAnvil(cf, moveOrder[i])) {
			if ((childScore = -AlphaBeta_negamax_powerup(cf, depth - 1, -beta, -alpha)) > parentScore) {
				parentScore = childScore;
			}
			ConnectFour_powerup_undropAnvil(cf);
			if (alpha < parentScore) {
				TranspositionTable_powerup_store(&table, (hashKey = ConnectFour_getHashKey(cf)), cf->pc->anvil, cf->pc->bomb, cf->pc->wall, cf->pc->x2, (alpha = parentScore), depth);
			}
			if (alpha >= beta) {
				return alpha;
			}
		}
	}
	for (i = 0; i < COLUMNS; ++i) {
		if (ConnectFour_powerup_drop(cf, moveOrder[i])) {
			if ((childScore = -AlphaBeta_negamax_powerup(cf, depth - 1, -beta, -alpha)) > parentScore) {
				parentScore = childScore;
			}
			ConnectFour_powerup_undrop(cf);
			if (alpha < parentScore) {
				TranspositionTable_powerup_store(&table, (hashKey = ConnectFour_getHashKey(cf)), cf->pc->anvil, cf->pc->bomb, cf->pc->wall, cf->pc->x2, (alpha = parentScore), depth);
			}
			if (alpha >= beta) {
				return alpha;
			}
		}
	}
	TranspositionTable_powerup_store(&table, (hashKey = ConnectFour_getHashKey(cf)), cf->pc->anvil, cf->pc->bomb, cf->pc->wall, cf->pc->x2, alpha, depth);
	return alpha;
}

int AlphaBeta_negamax_popten(ConnectFour *cf, int depth, int alpha, int beta) {
	int parentScore, childScore;
	unsigned i;
	++nodes;
	if ((tableScore = TranspositionTable_normal_loadValue(&table, (hashKey = ConnectFour_getHashKey(cf))))) {
		if (TranspositionTable_depthLessOrEqual(&table, hashKey, depth)) {
			return tableScore;
		}
	}
	if (AlphaBeta_popten_checkWin(cf)) {
		return PLAYER_WIN;
	}
	if (!depth) {
		return DRAW;
	}
	parentScore = alpha;
	for (i = 0; i < COLUMNS; ++i) {
		if (ConnectFour_popten_pop(cf, i)) {
			if ((childScore = AlphaBeta_negamax_popten(cf, depth - 1, alpha, beta)) > parentScore) {
				parentScore = childScore;
			}
			ConnectFour_popten_unpop(cf);
			if (alpha < parentScore) {
				TranspositionTable_store(&table, (hashKey = ConnectFour_getHashKey(cf)), (alpha = parentScore), depth);
			}
			if (alpha >= beta) {
				return alpha;
			}
		}
	}
	for (i = 0; i < COLUMNS; ++i) {
		if (ConnectFour_popten_drop(cf, i)) {
			if ((childScore = -AlphaBeta_negamax_popten(cf, depth - 1, -beta, -alpha)) > parentScore) {
				parentScore = childScore;
			}
			ConnectFour_popten_undrop(cf);
			if (alpha < parentScore) {
				TranspositionTable_store(&table, (hashKey = ConnectFour_getHashKey(cf)), (alpha = parentScore), depth);
			}
			if (alpha >= beta) {
				return alpha;
			}
		}
	}
	if (ConnectFour_popten_pass(cf)) {
		if ((childScore = -AlphaBeta_negamax_popten(cf, depth - 1, -beta, -alpha)) > parentScore) {
			parentScore = childScore;
		}
		ConnectFour_popten_unpass(cf);
		if (alpha < parentScore) {
			TranspositionTable_store(&table, (hashKey = ConnectFour_getHashKey(cf)), (alpha = parentScore), depth);
		}
		if (alpha >= beta) {
			return alpha;
		}
	}
	TranspositionTable_store(&table, (hashKey = ConnectFour_getHashKey(cf)), alpha, depth);
	return alpha;
}

int AlphaBeta_negamax_popten_withMoves(ConnectFour *cf, int currDepth, int maxDepth, int alpha, int beta) {
	int parentScore, childScore;
	unsigned i;
	++nodes;
	if ((tableScore = TranspositionTable_normal_loadValue(&table, (hashKey = ConnectFour_getHashKey(cf))))) {
		switch (TranspositionTable_loadBounds(&table, hashKey)) {
		case TT_LOWERBOUND:
			if (alpha < tableScore) {
				alpha = tableScore;
			}
			if (alpha >= beta) {
				return alpha;
			}
			break;
		case TT_UPPERBOUND:
			if (beta > tableScore) {
				beta = tableScore;
			}
			if (alpha >= beta) {
				return beta;
			}
		}
	}
	if (AlphaBeta_popten_checkWin(cf)) {
		return PLAYER_WIN;
	}
	if (ConnectFour_repetition(cf)) {
		if (repetitionFlag) {
			repetitionFlag = 0;
			printf("%s", REPETITION_STRING);
		}
		return -DRAW_OR_WIN;
	}
	if (currDepth >= maxDepth) {
		return -PLAYER_PROGRESS;
	}
	parentScore = alpha;
	for (i = 0; i < COLUMNS; ++i) {
		if (ConnectFour_popten_pop(cf, i)) {
			if ((childScore = AlphaBeta_negamax_popten_withMoves(cf, currDepth + 1, maxDepth, alpha, beta)) > parentScore) {
				pv[currDepth] = i + 'A';
				parentScore = childScore;
			}
			ConnectFour_popten_unpop(cf);
			if (alpha < parentScore) {
				TranspositionTable_storeBounds(&table, (hashKey = ConnectFour_getHashKey(cf)), (alpha = parentScore), maxDepth, TT_LOWERBOUND);
			}
			if (alpha >= beta) {
				return alpha;
			}
		}
	}
	for (i = 0; i < COLUMNS; ++i) {
		if (ConnectFour_popten_drop(cf, i)) {
			if ((childScore = -AlphaBeta_negamax_popten_withMoves(cf, currDepth + 1, maxDepth, -beta, -alpha)) > parentScore) {
				pv[currDepth] = i + '1';
				parentScore = childScore;
			}
			ConnectFour_popten_undrop(cf);
			if (alpha < parentScore) {
				TranspositionTable_storeBounds(&table, (hashKey = ConnectFour_getHashKey(cf)), (alpha = parentScore), maxDepth, TT_LOWERBOUND);
			}
			if (alpha >= beta) {
				return alpha;
			}
		}
	}
	if (ConnectFour_popten_pass(cf)) {
		if ((childScore = -AlphaBeta_negamax_popten_withMoves(cf, currDepth + 1, maxDepth, -beta, -alpha)) > parentScore) {
			pv[currDepth] = 'P';
			parentScore = childScore;
		}
		ConnectFour_popten_unpass(cf);
		if (alpha < parentScore) {
			TranspositionTable_storeBounds(&table, (hashKey = ConnectFour_getHashKey(cf)), (alpha = parentScore), maxDepth, TT_LOWERBOUND);
		}
		if (alpha >= beta) {
			return alpha;
		}
	}
	TranspositionTable_storeBounds(&table, (hashKey = ConnectFour_getHashKey(cf)), alpha, maxDepth, TT_UPPERBOUND);
	return alpha;
}

void AlphaBeta_normal_getMoveScores(ConnectFour *cf, Result *r, bool out) {
	if (!ConnectFour_gameOver(cf)) {
		Result *results = malloc(sizeof(Result) * COLUMNS);
		bool isSymmetric = cf->board[0] == ConnectFour_reverse(cf->board[0]) && cf->board[1] == ConnectFour_reverse(cf->board[1]);
		unsigned i, cols = isSymmetric ? COLUMNS & 1 ? (COLUMNS >> 1) + 1 : COLUMNS >> 1 : COLUMNS;
		for (i = 0; i < COLUMNS; ++i) {
			results[i].wdl = (char)UNKNOWN_CHAR;
		}
		for (i = 0; i < cols; ++i) {
#ifdef __unix__ // The standard output is line buffered on Linux. It has no effect on Windows since the standard output is not line buffered.
				fflush(stdout);
#endif
			if ((GAME_VARIANT == POPOUT_VARIANT) ? ConnectFour_popout_drop(cf, i) : ConnectFour_normal_drop(cf, i)) {
				if (ConnectFour_connection(cf->board[!(cf->plyNumber & 1)])) {
					results[i] = (Result){ WIN_CHAR, 0 };
					if (isSymmetric) {
						results[COLUMNS - 1 - i].wdl = results[i].wdl;
						results[COLUMNS - 1 - i].dtc = results[i].dtc;
					}
				}
				else {				
#ifdef __GNUC__
					TranspositionTable_destroy(&table);
					TranspositionTable_initialize(&table, tableSize);
#else
					TranspositionTable_reset(&table);
#endif			
					if (GAME_VARIANT == POPOUT_VARIANT) {
						ConnectFour_clearHistory(cf);
					}
					results[i] = (GAME_VARIANT == POPOUT_VARIANT) ? AlphaBeta_popout_solve(cf) : AlphaBeta_normal_solve(cf);
					Result_increment(&results[i]);
					if (isSymmetric) {
						results[COLUMNS - 1 - i].wdl = results[i].wdl;
						results[COLUMNS - 1 - i].dtc = results[i].dtc;
					}
				}
				if (GAME_VARIANT == POPOUT_VARIANT) {
					ConnectFour_popout_undrop(cf);
				}
				else {
					ConnectFour_normal_undrop(cf);
				}
			}
			if (out) {
				Result_print(&results[i]);
			}
		}
		if (isSymmetric && out) {
			for (; i < COLUMNS; ++i) {
				Result_print(&results[i]);
			}
		}
		if (r) {
			for (i = 0; i < COLUMNS; ++i) {
				r[i] = results[i];
			}
		}
		free(results);
	}
}

void AlphaBeta_popout_getMoveScores(ConnectFour *cf, Result *r, bool out) {
	AlphaBeta_normal_getMoveScores(cf, r, out);
	puts("");
	if (!ConnectFour_gameOver(cf)) {
		Result *results = malloc(sizeof(Result) * COLUMNS);
		bool isSymmetric = cf->board[0] == ConnectFour_reverse(cf->board[0]) && cf->board[1] == ConnectFour_reverse(cf->board[1]);
		unsigned i, cols = isSymmetric ? COLUMNS & 1 ? (COLUMNS >> 1) + 1 : COLUMNS >> 1 : COLUMNS;
		for (i = 0; i < COLUMNS; ++i) {
			results[i].wdl = (char)UNKNOWN_CHAR;
		}
		for (i = 0; i < cols; ++i) {
#ifdef __unix__ 
				fflush(stdout);
#endif
			if (ConnectFour_popout_pop(cf, i)) {
				if (ConnectFour_connectionNoVertical(cf->board[!(cf->plyNumber & 1)])) {
					results[i] = (Result){ WIN_CHAR, 0 };
					if (isSymmetric) {
						results[COLUMNS - 1 - i].wdl = results[i].wdl;
						results[COLUMNS - 1 - i].dtc = results[i].dtc;
					}
				}
				else if (ConnectFour_connectionNoVertical(cf->board[cf->plyNumber & 1])) {
					results[i] = (Result){ LOSS_CHAR, 0 };
					if (isSymmetric) {
						results[COLUMNS - 1 - i].wdl = results[i].wdl;
						results[COLUMNS - 1 - i].dtc = results[i].dtc;
					}
				}
				else {
#ifdef __GNUC__
					TranspositionTable_destroy(&table);
					TranspositionTable_initialize(&table, tableSize);
#else
					TranspositionTable_reset(&table);
#endif		
					ConnectFour_clearHistory(cf);
					results[i] = AlphaBeta_popout_solve(cf);
					Result_increment(&results[i]);
					if (isSymmetric) {
						results[COLUMNS - 1 - i].wdl = results[i].wdl;
						results[COLUMNS - 1 - i].dtc = results[i].dtc;
					}
				}
				ConnectFour_popout_unpop(cf);
			}
			if (out) {
				Result_print(&results[i]);
			}
		}
		if (isSymmetric && out) {
			for (; i < COLUMNS; ++i) {
				Result_print(&results[i]);
			}
		}
		if (r) {
			for (i = COLUMNS; i < COLUMNS_X2; ++i) {
				r[i] = results[i - COLUMNS];
			}
		}
		free(results);
	}
}


void AlphaBeta_popten_getMoveScores(ConnectFour *cf, Result *r, bool out) {
	if (!ConnectFour_gameOver(cf)) {
		Result *results = malloc(sizeof(Result) * ConnectFour_getMoveSize());
		bool isSymmetric = cf->board[0] == ConnectFour_reverse(cf->board[0]) && cf->board[1] == ConnectFour_reverse(cf->board[1]);
		unsigned i, cols = isSymmetric ? COLUMNS & 1 ? (COLUMNS >> 1) + 1 : COLUMNS >> 1 : COLUMNS;
		for (i = 0; i < COLUMNS_X2; ++i) {
			results[i].wdl = (char)UNKNOWN_CHAR;
		}
		if (ConnectFour_popten_pass(cf)) {
			results[(i = 0)] = AlphaBeta_popten_solve(cf);
			Result_increment(&results[i]);
			ConnectFour_popten_unpass(cf);
			if (out) {
				printf("Pass");
			}
			puts("");
			goto resultAssignment;
		}
		for (i = 0; i < cols; ++i) {
#ifdef __unix__ 
			fflush(stdout);
#endif
			if (ConnectFour_popten_drop(cf, i)) {
#ifdef __GNUC__
				TranspositionTable_destroy(&table);
				TranspositionTable_initialize(&table, tableSize);
#else
				TranspositionTable_reset(&table);
#endif		
				results[i] = AlphaBeta_popten_solve(cf);
				Result_increment(&results[i]);
				if (isSymmetric) {
					results[COLUMNS - 1 - i].wdl = results[i].wdl;
					results[COLUMNS - 1 - i].dtc = results[i].dtc;
				}
				ConnectFour_popten_undrop(cf);
			}
			if (out) {
				Result_print(&results[i]);
			}
		}
		if (isSymmetric && out) {
			for (; i < COLUMNS; ++i) {
				Result_print(&results[i]);
			}
		}
		puts("");
		for (i = 0; i < cols; ++i) {
#ifdef __unix__ 
			fflush(stdout);
#endif
			if (ConnectFour_popten_pop(cf, i)) {
				if (ConnectFour_hasTenDisks(cf)) {
					results[i + COLUMNS] = (Result){ WIN_CHAR, 0 };
					if (isSymmetric) {
						results[COLUMNS_X2 - 1 - i].wdl = results[i + COLUMNS].wdl;
						results[COLUMNS_X2 - 1 - i].dtc = results[i + COLUMNS].dtc;
					}
				}
				else {
#ifdef __GNUC__
					TranspositionTable_destroy(&table);
					TranspositionTable_initialize(&table, tableSize);
#else
					TranspositionTable_reset(&table);
#endif		
					results[i + COLUMNS] = AlphaBeta_popten_solve(cf);
					++results[i + COLUMNS].dtc;
					if (isSymmetric) {
						results[COLUMNS_X2 - 1 - i].wdl = results[i + COLUMNS].wdl;
						results[COLUMNS_X2 - 1 - i].dtc = results[i + COLUMNS].dtc;
					}
				}
				ConnectFour_popten_unpop(cf);
			}
			if (out) {
				Result_print(&results[i + COLUMNS]);
			}
		}
		if (isSymmetric && out) {
			for (; i < COLUMNS; ++i) {
				Result_print(&results[i + COLUMNS]);
			}
		}
resultAssignment:
		if (r) {
			for (i = 0; i < COLUMNS_X2; ++i) {
				r[i] = results[i];
			}
		}
		free(results);
	}
}

int AlphaBeta_normal_getBestMove(ConnectFour *cf, Result *r, bool out) {
	AlphaBeta_normal_getMoveScores(cf, r, out);
	if (!ConnectFour_gameOver(cf)) {
		return Result_getBestMove(r, COLUMNS);
	}
	return -1;
}

int AlphaBeta_popout_getBestMove(ConnectFour *cf, Result *r, bool out) {
	AlphaBeta_popout_getMoveScores(cf, r, out);
	if (!ConnectFour_gameOver(cf)) {
		return Result_getBestMove(r, COLUMNS_X2);
	}
	return -1;
}

int AlphaBeta_popten_getBestMove(ConnectFour* cf, Result* r, bool out) {
	AlphaBeta_popten_getMoveScores(cf, r, out);
	if (!ConnectFour_gameOver(cf)) {
		return Result_getBestMove(r, ConnectFour_getMoveSize());
	}
	return -1;
}
/*
int AlphaBeta_normal_MTDF(ConnectFour *cf, int guess, int depth) {
	int lowerBound = -PLAYER_WIN, upperBound = PLAYER_WIN, beta;
	while (lowerBound < upperBound) {
		beta = guess != lowerBound ? guess : guess + 1;
		guess = AlphaBeta_negamax_normal(cf, depth, beta - 1, beta);
		if (guess < beta) {
			upperBound = guess;
		}
		else {
			lowerBound = guess;
		}
	}
	return guess;
}*/

void MoveSorter_addToEntry(MoveSorter *sorter, const int move, const int score) {
	int i = sorter->size++;
	for (; i && sorter->moveEntries[i - 1].score > score; --i) {
		sorter->moveEntries[i] = sorter->moveEntries[i - 1];
	}
	sorter->moveEntries[i].move = move;
	sorter->moveEntries[i].score = score;
}

int MoveSorter_obtainNextMove(MoveSorter *sorter) {
	return sorter->size ? sorter->moveEntries[--sorter->size].move : 0;
}

Result AlphaBeta_normal_solve(ConnectFour *cf) {
	int depth = MOVESIZE - cf->plyNumber, solution = 0, d;
	for (d = 0; d < depth; ++d) {
#ifdef VERBOSE
		printf(SEARCHING_STRING, d);
#endif
		if ((solution = AlphaBeta_negamax_normal(cf, d, -1, 1))) {
#ifdef VERBOSE
			printf(FOUND_STRING);
#endif
			return (Result) { solution > DRAW ? WIN_CHAR : LOSS_CHAR, d };
		}
	}
#ifdef VERBOSE
	printf(FOUND_STRING);
#endif
	return DRAW_RESULT;
}

Result AlphaBeta_popout_solve(ConnectFour *cf) {
	int depth = MOVESIZE - cf->plyNumber, solution = DRAW, d;
	repetitionFlag = 1;
	for (d = 0; d < depth; ++d) {
#ifdef VERBOSE
		printf(SEARCHING_STRING, d);
#endif
		if (abs((solution = AlphaBeta_negamax_popout(cf, d, -PLAYER_WIN, PLAYER_WIN))) != PLAYER_PROGRESS) {
#ifdef VERBOSE
			printf(FOUND_STRING);
#endif
			if (((solution == PLAYER_WIN) && !(d & 1)) || ((solution == -PLAYER_WIN) && (d & 1))) {
				return (Result) {solution > DRAW ? WIN_CHAR : LOSS_CHAR, d };
			}
			else {
				return DRAW_RESULT;
			}
		}
	}
#ifdef VERBOSE
	printf(NOT_FOUND_STRING, d, (d == 1) ? "ply" : "plies");
#endif
	return DRAW_RESULT;
}

Result AlphaBeta_powerup_solve(ConnectFour *cf) {
	int depth = MOVESIZE - cf->plyNumber, solution = DRAW, d;
	for (d = 0; d < depth; ++d) {
#ifdef VERBOSE
		printf(SEARCHING_STRING, d);
#endif
		if ((solution = AlphaBeta_negamax_powerup(cf, d, -1, 1))) {
#ifdef VERBOSE
			printf(FOUND_STRING);
#endif
			return (Result) { solution > DRAW ? WIN_CHAR : LOSS_CHAR, d };
		}
	}
#ifdef VERBOSE
	printf(FOUND_STRING);
#endif
	return DRAW_RESULT;
}

Result AlphaBeta_popten_solve(ConnectFour *cf) {
	int depth = MOVESIZE - cf->plyNumber, solution = DRAW, d;
	for (d = 0; d < depth; ++d) {
#ifdef VERBOSE
		printf(SEARCHING_STRING, d);
#endif
		if ((solution = AlphaBeta_negamax_popten(cf, d, -PLAYER_WIN, PLAYER_WIN))) {
#ifdef VERBOSE
			printf(FOUND_STRING);
#endif
			return (Result) { solution > DRAW ? WIN_CHAR : LOSS_CHAR, d };
		}
	}
#ifdef VERBOSE
	printf(FOUND_STRING);
#endif
	return UNKNOWN_RESULT;
}

void Result_print(Result *result) {
	switch (result->wdl) {
	case UNKNOWN_CHAR:
		printf(NONE_TEXT);
		break;
	case DRAW_CHAR:
		printf("%c ", DRAW_CHAR);
		break;
	default:
		if (result->wdl == WIN_CHAR && !result->dtc) {
			printf(WIN_TEXT);
		}
		else if (result->wdl == LOSS_CHAR && !result->dtc) {
			printf(LOSS_TEXT);
		}
		else {
			printf("%c%d ", result->wdl, result->dtc);
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
	unsigned i, j, *bestMoves = GAME_VARIANT == POPOUT_VARIANT || GAME_VARIANT == POPTEN_VARIANT ? malloc(sizeof(int) * COLUMNS_X2) : malloc(sizeof(int) * COLUMNS), best;
	init_genrand64(time(NULL));
	Result bestResult = Result_getBestResult(results, resultSize);
	for (i = j = 0; i < resultSize; ++i) {
		if (bestResult.wdl == results[i].wdl && bestResult.dtc == results[i].dtc) {
			bestMoves[j++] = i;
		}
	}
	best = bestMoves[genrand64_int64() % j];
	free(bestMoves);
	return best;
}
