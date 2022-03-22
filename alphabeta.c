#include "alphabeta.h"

void AlphaBeta_getColumnMoveOrder(void) {
	for (int c = 0; c < (int)COLUMNS; ++c) {
		moveOrder[c] = COLUMNS / 2 + (1 - 2 * (c & 1)) * (c + 1) / 2;
	}
}

bool AlphaBeta_normal_checkWin(ConnectFour *cf) {
	for (unsigned c = 0; c < COLUMNS; ++c) {
		if (ConnectFour_connection(cf->board[cf->plyNumber & 1u] | (Position)1ull << cf->height[c] & ALL)) {
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
	for (unsigned c = 0; c < COLUMNS; ++c) {
		if (ConnectFour_connection(cf->board[cf->plyNumber & 1] | (Position)1ull << cf->height[c] & ALL)) {
			return true;
		}
		if (ConnectFour_popout_popWinCheck(cf, c)) {
			if (ConnectFour_connectionNoVertical(cf->board[cf->plyNumber & 1])) {
				ConnectFour_popout_unpopWinCheck(cf, c);
				return true;
			}
			ConnectFour_popout_unpopWinCheck(cf, c);
		}
	}
	return false;
}

bool AlphaBeta_popten_checkDraw(ConnectFour *cf) {
	return (popcount(cf->board[0]) < 4) && (popcount(cf->board[1]) < 4) && (((cf->collectedDisks & 0xf) < 10) || (((cf->collectedDisks & 0xf0) >> 4) < 10));
}

bool AlphaBeta_powerup_checkWin(ConnectFour *cf) {
	unsigned c, turn = cf->plyNumber & 1u;
	/*Position oldBoard[2] = { cf->board[0], cf->board[1] }, oldPowerCheckers[4] = { cf->pc->anvil, cf->pc->bomb, cf->pc->wall, cf->pc->x2 };
	unsigned short oldPPC = cf->playedPowerCheckers, oldStatus = cf->pum[cf->plyNumber].status;*/
	for (c = 0; c < COLUMNS; ++c) {
		if ((cf->pum[cf->plyNumber].status == POWERUP_DROP_NORMAL_OR_ANVIL || cf->pum[cf->plyNumber].status == POWERUP_DROP_X2) && ConnectFour_connection(cf->board[turn] | (Position)1ull << cf->height[c] & ALL)) {
			return true;
		}
		if (ConnectFour_powerup_dropAnvil(cf, c)) {
			if (ConnectFour_connection(cf->board[turn])) {
				ConnectFour_powerup_undropAnvil(cf);
				return true;
			}
			ConnectFour_powerup_undropAnvil(cf);
			/*assert(cf->board[0] == oldBoard[0] && cf->board[1] == oldBoard[1] &&
					oldStatus == cf->pum[cf->plyNumber].status &&
					oldPPC == cf->playedPowerCheckers &&
					oldPowerCheckers[0] == cf->pc->anvil &&
					oldPowerCheckers[1] == cf->pc->bomb &&
					oldPowerCheckers[2] == cf->pc->wall &&
					oldPowerCheckers[3] == cf->pc->x2);*/
		}
		if (ConnectFour_powerup_dropBomb(cf, c)) {
			if (ConnectFour_connection(cf->board[turn])) {
				ConnectFour_powerup_undropBomb(cf);
				return true;
			}
			ConnectFour_powerup_undropBomb(cf);
		}
		if (ConnectFour_powerup_pop(cf, c)) {
			if (ConnectFour_connectionNoVertical(cf->board[turn])) {
				ConnectFour_powerup_unpop(cf);
				return true;
			}
			ConnectFour_powerup_unpop(cf);
			/*assert(cf->board[0] == oldBoard[0] && cf->board[1] == oldBoard[1] &&
					oldStatus == cf->pum[cf->plyNumber].status &&
					oldPPC == cf->playedPowerCheckers &&
					oldPowerCheckers[0] == cf->pc->anvil &&
					oldPowerCheckers[1] == cf->pc->bomb &&
					oldPowerCheckers[2] == cf->pc->wall &&
					oldPowerCheckers[3] == cf->pc->x2);*/
		}
		if (ConnectFour_powerup_dropX2(cf, c)) {
			if (ConnectFour_connection(cf->board[turn])) {
				ConnectFour_powerup_undropX2(cf);
				return true;
			}
			ConnectFour_powerup_undropX2(cf);
			/*assert(cf->board[0] == oldBoard[0] && cf->board[1] == oldBoard[1] &&
					oldStatus == cf->pum[cf->plyNumber].status &&
					oldPPC == cf->playedPowerCheckers &&
					oldPowerCheckers[0] == cf->pc->anvil &&
					oldPowerCheckers[1] == cf->pc->bomb &&
					oldPowerCheckers[2] == cf->pc->wall &&
					oldPowerCheckers[3] == cf->pc->x2);*/
		}
	}
	/*assert(cf->board[0] == oldBoard[0] && cf->board[1] == oldBoard[1] &&
					oldStatus == cf->pum[cf->plyNumber].status &&
					oldPPC == cf->playedPowerCheckers &&
					oldPowerCheckers[0] == cf->pc->anvil &&
					oldPowerCheckers[1] == cf->pc->bomb &&
					oldPowerCheckers[2] == cf->pc->wall &&
					oldPowerCheckers[3] == cf->pc->x2);*/
	return false;
}

/*for (d = 0; d < COLUMNS; ++d) {
	if (ConnectFour_powerup_dropBomb(cf, c)) {
		if (ConnectFour_connection(cf->board[turn])) {
		ConnectFour_printBoard(cf);
		if (ConnectFour_powerup_pop(cf, d)) {
			if (ConnectFour_connection(cf->board[turn])) {
				ConnectFour_powerup_undropBomb(cf);
				ConnectFour_powerup_undropBomb(cf);
				assert(cf->board[0] == oldBoard[0] && cf->board[1] == oldBoard[1] && oldPPC == cf->playedPowerCheckers);
				return true;
			}
			ConnectFour_powerup_undropBomb(cf);
		}
		ConnectFour_powerup_undropBomb(cf);
		}
	}
	if (ConnectFour_powerup_dropX2(cf, c, d)) {
		if (ConnectFour_connection(cf->board[turn])) {
			ConnectFour_powerup_undropX2(cf);
			assert(cf->board[0] == oldBoard[0] && cf->board[1] == oldBoard[1] && oldPPC == cf->playedPowerCheckers);
			return true;
		}
		ConnectFour_powerup_undropX2(cf);
	}
}*/

bool AlphaBeta_popten_checkWin(ConnectFour *cf) {
	for (unsigned c = 0; c < COLUMNS; ++c) {
		if (ConnectFour_popten_pop(cf, c)) {
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
	int rootScore, leafScore;
	unsigned c;
	++nodes;
	if ((tableScore = TranspositionTable_normal_loadValue(&table, (hashKey = ConnectFour_getHashKey(cf))))) {
		return tableScore;
	}
	if (AlphaBeta_normal_checkWin(cf)) {
		return PLAYER_WIN;
	}
	if (!depth) {
		return DRAW;
	}
	rootScore = alpha;
	for (c = 0; c < COLUMNS; ++c) {
		if (ConnectFour_normal_drop(cf, moveOrder[c])) {
			if ((leafScore = -AlphaBeta_negamax_normal(cf, depth - 1, -beta, -alpha)) > rootScore) {
				rootScore = leafScore;
			}
			ConnectFour_normal_undrop(cf);
			if (alpha < rootScore) {
				TranspositionTable_storeWithoutDepth(&table, (hashKey = ConnectFour_getHashKey(cf)), (alpha = rootScore));
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
	int rootScore, branchScore;
	unsigned c;
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
	for (c = 0; c < COLUMNS; ++c) {
		if (ConnectFour_connection(cf->board[cf->plyNumber & 1u] | (Position)1ull << cf->height[c] & ALL)) {
			pv[currDepth] = c + '1';
			return PLAYER_WIN;
		}
	}
	if (currDepth >= maxDepth) {
		return DRAW;
	}
	rootScore = alpha;
	for (c = 0; c < COLUMNS; ++c) {
		if (ConnectFour_normal_drop(cf, moveOrder[c])) {
			if ((branchScore = -AlphaBeta_negamax_normal_withMoves(cf, currDepth + 1, maxDepth, -beta, -alpha)) > rootScore) {
				pv[currDepth] = moveOrder[c] + '1';
				rootScore = branchScore;
			}
			ConnectFour_normal_undrop(cf);
			if (alpha < rootScore) {
				TranspositionTable_storeBounds(&table, (hashKey = ConnectFour_getHashKey(cf)), (alpha = rootScore), maxDepth, TT_LOWERBOUND);
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
	int rootScore, branchScore;
	unsigned c;
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
			int repetitionDepth = MOVESIZE - cf->plyNumber, repetitionScore, currentScore, opponentScore, * dropScores = malloc(sizeof(int) * COLUMNS), * popScores = malloc(sizeof(int) * COLUMNS), d;
			bool isPopMove = false;
			repetitionFlag = 0;
			ConnectFour_printBoard(cf);
			if ((repetitionMove = cf->moves[cf->plyNumber - 4u]) >= COLUMNS) {
				repetitionMove -= COLUMNS;
				isPopMove = true;
			}
			isPopMove ? (popScores[repetitionMove] = DRAW) : (dropScores[repetitionMove] = DRAW);
			for (c = 0; c < COLUMNS; ++c) {
				if (!(c == repetitionMove && isPopMove)) {
					popScores[c] = INT_MIN;
					if (ConnectFour_popout_pop(cf, c)) {
						ConnectFour_printBoard(cf);
						for (d = 0, currentScore = -PLAYER_WIN, opponentScore = DRAW_OR_WIN; d < repetitionDepth; ++d) {
							ConnectFour_clearHistory(cf);
							if ((repetitionScore = AlphaBeta_negamax_popout(cf, d, -PLAYER_WIN, PLAYER_WIN)) > currentScore) {
								currentScore = repetitionScore;
								popScores[c] = currentScore;
								if (abs(currentScore) >= PLAYER_WIN || abs(currentScore) == DRAW_OR_WIN) {
									break;
								}
							}
						}
						ConnectFour_popout_unpop(cf);
					}
				}
				if (!(c == repetitionMove && !isPopMove)) {
					dropScores[c] = INT_MIN;
					if (ConnectFour_popout_drop(cf, c)) {
						ConnectFour_printBoard(cf);
						for (d = 0, currentScore = -PLAYER_WIN, opponentScore = DRAW_OR_WIN; d < repetitionDepth; ++d) {
							ConnectFour_clearHistory(cf);
							if ((repetitionScore = AlphaBeta_negamax_popout(cf, d, -PLAYER_WIN, PLAYER_WIN)) > currentScore) {
								currentScore = repetitionScore;
								dropScores[c] = currentScore;
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
			for (c = 0; c < COLUMNS; ++c) {
				if (repetitionScore < dropScores[c]) {
					repetitionScore = dropScores[c];
				}
				if (repetitionScore < popScores[c]) {
					repetitionScore = popScores[c];
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
		return -IN_PROGRESS;
	}
	rootScore = alpha;
	for (c = 0; c < COLUMNS; ++c) {
		if (ConnectFour_popout_drop(cf, moveOrder[c])) {
			if ((branchScore = -AlphaBeta_negamax_popout(cf, depth - 1, -beta, -alpha)) > rootScore) {
				rootScore = branchScore;
			}
			ConnectFour_popout_undrop(cf);
			if (alpha < rootScore) {
				TranspositionTable_store(&table, (hashKey = ConnectFour_getHashKey(cf)), (alpha = rootScore), depth);
			}
			if (alpha >= beta) {
				return alpha;
			}
		}
	}
	for (c = 0; c < COLUMNS; ++c) {
		if (ConnectFour_popout_pop(cf, moveOrder[c])) {
			if ((branchScore = -AlphaBeta_negamax_popout(cf, depth - 1, -beta, -alpha)) > rootScore) {
				rootScore = branchScore;
			}
			ConnectFour_popout_unpop(cf);
			if (alpha < rootScore) {
				TranspositionTable_store(&table, (hashKey = ConnectFour_getHashKey(cf)), (alpha = rootScore), depth);
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
	int rootScore, branchScore;
	unsigned c;
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
	for (c = 0; c < COLUMNS; ++c) {
		if (ConnectFour_connection(cf->board[cf->plyNumber & 1u] | (Position)1ull << cf->height[c] & ALL)) {
			pv[currDepth] = c + '1';
			return PLAYER_WIN;
		}
		if (ConnectFour_popout_popWinCheck(cf, c)) {
			if (ConnectFour_connectionNoVertical(cf->board[cf->plyNumber & 1u])) {
				ConnectFour_popout_unpopWinCheck(cf, c);
				pv[currDepth] = c + 'A';
				return PLAYER_WIN;
			}
			ConnectFour_popout_unpopWinCheck(cf, c);
		}
	}
	if (ConnectFour_repetition(cf)) {
		if (repetitionFlag) {
			repetitionFlag = 0;
			printf("%s (%d)\n", REPETITION_STRING, maxDepth - 1);
		}
		return -DRAW_WIN;
	}
	if (currDepth >= maxDepth) {
		return -IN_PROGRESS;
	}
	rootScore = alpha;
	for (c = 0; c < COLUMNS; ++c) {
		if (ConnectFour_popout_drop(cf, moveOrder[c])) {
			if ((branchScore = -AlphaBeta_negamax_popout_withMoves(cf, currDepth + 1, maxDepth, -beta, -alpha)) > rootScore) {
				pv[currDepth] = moveOrder[c] + '1';
				rootScore = branchScore;
			}
			ConnectFour_popout_undrop(cf);
			if (alpha < rootScore) {
				TranspositionTable_storeBounds(&table, (hashKey = ConnectFour_getHashKey(cf)), (alpha = rootScore), maxDepth, TT_LOWERBOUND);
			}
			if (alpha >= beta) {
				return alpha;
			}
		}
	}
	for (c = 0; c < COLUMNS; ++c) {
		if (ConnectFour_popout_pop(cf, moveOrder[c])) {
			if ((branchScore = -AlphaBeta_negamax_popout_withMoves(cf, currDepth + 1, maxDepth, -beta, -alpha)) > rootScore) {
				pv[currDepth] = moveOrder[c] + 'A';
				rootScore = branchScore;
			}
			ConnectFour_popout_unpop(cf);
			if (alpha < rootScore) {
				TranspositionTable_storeBounds(&table, (hashKey = ConnectFour_getHashKey(cf)), (alpha = rootScore), maxDepth, TT_LOWERBOUND);
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
	int rootScore, branchScore;
	unsigned c;
	/*Position oldBoard[2] = { cf->board[0], cf->board[1] }, oldPowerCheckers[4] = { cf->pc->anvil, cf->pc->bomb, cf->pc->wall, cf->pc->x2 };
	unsigned short oldPPC = cf->playedPowerCheckers, oldStatus = cf->pum[cf->plyNumber].status;*/
	++nodes;
	if (abs((tableScore = TranspositionTable_powerup_loadValue(&table, (hashKey = ConnectFour_getHashKey(cf)), hashKey ^ cf->pc->anvil, hashKey ^ cf->pc->bomb, hashKey ^ cf->pc->wall, hashKey ^ cf->pc->x2))) != IN_PROGRESS) {
		if (TranspositionTable_powerup_depthEquality(&table, hashKey, hashKey ^ cf->pc->anvil, hashKey ^ cf->pc->bomb, hashKey ^ cf->pc->wall, hashKey ^ cf->pc->x2, depth)) {
			return tableScore;
		}
	}
	if (AlphaBeta_powerup_checkWin(cf)) {
		return PLAYER_WIN;
	}
	if ((cf->board[0] ^ cf->board[1]) == ALL) {
		return DRAW;
	}
	if (!depth) {
		return -IN_PROGRESS;
	}
	rootScore = alpha;
	for (c = 0; c < COLUMNS; ++c) {
		if (ConnectFour_powerup_dropX2(cf, moveOrder[c])) {
			if ((branchScore = AlphaBeta_negamax_powerup(cf, depth - 1, alpha, beta)) > rootScore) {
				rootScore = branchScore;
			}
			ConnectFour_powerup_undropX2(cf);
			/*assert(cf->board[0] == oldBoard[0] && cf->board[1] == oldBoard[1] &&
					oldStatus == cf->pum[cf->plyNumber].status &&
					oldPPC == cf->playedPowerCheckers &&
					oldPowerCheckers[0] == cf->pc->anvil &&
					oldPowerCheckers[1] == cf->pc->bomb &&
					oldPowerCheckers[2] == cf->pc->wall &&
					oldPowerCheckers[3] == cf->pc->x2);*/
			if (alpha < rootScore) {
				TranspositionTable_powerup_store(&table, (hashKey = ConnectFour_getHashKey(cf)), hashKey ^ cf->pc->anvil, hashKey ^ cf->pc->bomb, hashKey ^ cf->pc->wall, hashKey ^ cf->pc->x2, (alpha = rootScore), depth);
			}
			if (alpha >= beta) {
				return alpha;
			}
		}
	}
	for (c = 0; c < COLUMNS; ++c) {
		if (ConnectFour_powerup_dropWall(cf, moveOrder[c])) {
			if ((branchScore = AlphaBeta_negamax_powerup(cf, depth - 1, alpha, beta)) > rootScore) {
				rootScore = branchScore;
			}
			ConnectFour_powerup_undropWall(cf);
			/*assert(cf->board[0] == oldBoard[0] && cf->board[1] == oldBoard[1] &&
					oldStatus == cf->pum[cf->plyNumber].status &&
					oldPPC == cf->playedPowerCheckers &&
					oldPowerCheckers[0] == cf->pc->anvil &&
					oldPowerCheckers[1] == cf->pc->bomb &&
					oldPowerCheckers[2] == cf->pc->wall &&
					oldPowerCheckers[3] == cf->pc->x2);*/
			if (alpha < rootScore) {
				TranspositionTable_powerup_store(&table, (hashKey = ConnectFour_getHashKey(cf)), hashKey ^ cf->pc->anvil, hashKey ^ cf->pc->bomb, hashKey ^ cf->pc->wall, hashKey ^ cf->pc->x2, (alpha = rootScore), depth);
			}
			if (alpha >= beta) {
				return alpha;
			}
		}
	}

	for (c = 0; c < COLUMNS; ++c) {
		if (ConnectFour_powerup_dropBomb(cf, moveOrder[c])) {
			if ((branchScore = AlphaBeta_negamax_powerup(cf, depth - 1, alpha, beta)) > rootScore) {
				rootScore = branchScore;
			}
			ConnectFour_powerup_undropBomb(cf);
			/*assert(cf->board[0] == oldBoard[0] && cf->board[1] == oldBoard[1] &&
					oldStatus == cf->pum[cf->plyNumber].status &&
					oldPPC == cf->playedPowerCheckers &&
					oldPowerCheckers[0] == cf->pc->anvil &&
					oldPowerCheckers[1] == cf->pc->bomb &&
					oldPowerCheckers[2] == cf->pc->wall &&
					oldPowerCheckers[3] == cf->pc->x2);*/
			if (alpha < rootScore) {
				TranspositionTable_powerup_store(&table, (hashKey = ConnectFour_getHashKey(cf)), hashKey ^ cf->pc->anvil, hashKey ^ cf->pc->bomb, hashKey ^ cf->pc->wall, hashKey ^ cf->pc->x2, (alpha = rootScore), depth);
			}
			if (alpha >= beta) {
				return alpha;
			}
		}

	}
	for (c = 0; c < COLUMNS; ++c) {
		if (ConnectFour_powerup_pop(cf, moveOrder[c])) {
			if ((branchScore = -AlphaBeta_negamax_powerup(cf, depth - 1, -beta, -alpha)) > rootScore) {
				rootScore = branchScore;
			}
			ConnectFour_powerup_unpop(cf);
			/*assert(cf->board[0] == oldBoard[0] && cf->board[1] == oldBoard[1] &&
					oldStatus == cf->pum[cf->plyNumber].status &&
					oldPPC == cf->playedPowerCheckers &&
					oldPowerCheckers[0] == cf->pc->anvil &&
					oldPowerCheckers[1] == cf->pc->bomb &&
					oldPowerCheckers[2] == cf->pc->wall &&
					oldPowerCheckers[3] == cf->pc->x2);*/
			if (alpha < rootScore) {
				TranspositionTable_powerup_store(&table, (hashKey = ConnectFour_getHashKey(cf)), hashKey ^ cf->pc->anvil, hashKey ^ cf->pc->bomb, hashKey ^ cf->pc->wall, hashKey ^ cf->pc->x2, (alpha = rootScore), depth);
			}
			if (alpha >= beta) {
				return alpha;
			}
		}
	}

	for (c = 0; c < COLUMNS; ++c) {
		if (ConnectFour_powerup_dropAnvil(cf, moveOrder[c])) {
			if ((branchScore = -AlphaBeta_negamax_powerup(cf, depth - 1, -beta, -alpha)) > rootScore) {
				rootScore = branchScore;
			}
			ConnectFour_powerup_undropAnvil(cf);
			/*assert(cf->board[0] == oldBoard[0] && cf->board[1] == oldBoard[1] &&
					oldStatus == cf->pum[cf->plyNumber].status &&
					oldPPC == cf->playedPowerCheckers &&
					oldPowerCheckers[0] == cf->pc->anvil &&
					oldPowerCheckers[1] == cf->pc->bomb &&
					oldPowerCheckers[2] == cf->pc->wall &&
					oldPowerCheckers[3] == cf->pc->x2);*/
			if (alpha < rootScore) {
				TranspositionTable_powerup_store(&table, (hashKey = ConnectFour_getHashKey(cf)), hashKey ^ cf->pc->anvil, hashKey ^ cf->pc->bomb, hashKey ^ cf->pc->wall, hashKey ^ cf->pc->x2, (alpha = rootScore), depth);
			}
			if (alpha >= beta) {
				return alpha;
			}
		}
	}
	for (c = 0; c < COLUMNS; ++c) {
		if (ConnectFour_powerup_drop(cf, moveOrder[c])) {
			if ((branchScore = -AlphaBeta_negamax_powerup(cf, depth - 1, -beta, -alpha)) > rootScore) {
				rootScore = branchScore;
			}
			ConnectFour_powerup_undrop(cf);
			/*assert(cf->board[0] == oldBoard[0] && cf->board[1] == oldBoard[1] &&
					oldStatus == cf->pum[cf->plyNumber].status &&
					oldPPC == cf->playedPowerCheckers &&
					oldPowerCheckers[0] == cf->pc->anvil &&
					oldPowerCheckers[1] == cf->pc->bomb &&
					oldPowerCheckers[2] == cf->pc->wall &&
					oldPowerCheckers[3] == cf->pc->x2);*/
			if (alpha < rootScore) {
				TranspositionTable_powerup_store(&table, (hashKey = ConnectFour_getHashKey(cf)), hashKey ^ cf->pc->anvil, hashKey ^ cf->pc->bomb, hashKey ^ cf->pc->wall, hashKey ^ cf->pc->x2, (alpha = rootScore), depth);
			}
			if (alpha >= beta) {
				return alpha;
			}
		}
	}
	TranspositionTable_powerup_store(&table, (hashKey = ConnectFour_getHashKey(cf)), hashKey ^ cf->pc->anvil, hashKey ^ cf->pc->bomb, hashKey ^ cf->pc->wall, hashKey ^ cf->pc->x2, (alpha = rootScore), depth);
	return alpha;
}

int AlphaBeta_negamax_popten(ConnectFour *cf, int depth, int alpha, int beta) {
	int rootScore, branchScore;
	unsigned m, l, sortedMove;
	MoveSorter moveSorter;
	++nodes;
	if (abs((tableScore = TranspositionTable_popout_loadValue(&table, (hashKey = ConnectFour_getHashKey(cf))))) >= PLAYER_WIN) {
		if (TranspositionTable_depthLessOrEqual(&table, hashKey, depth)) {
			return tableScore;
		}
	}
	if (AlphaBeta_popten_checkWin(cf)) {
		return PLAYER_WIN;
	}
	if (AlphaBeta_popten_checkDraw(cf)) {
		return DRAW;
	}
	if (!depth) {
		return -IN_PROGRESS;
	}
	rootScore = alpha;
	moveSorter.moveEntries = malloc(sizeof(struct MoveSorter_Entries) * (l = ConnectFour_numberLegalMoves(cf)));
	moveSorter.size = 0;
	if (ConnectFour_popten_pass(cf)) {
		MoveSorter_addToEntry(&moveSorter, COLUMNS_X2_P1, 0);
		ConnectFour_popten_unpass(cf);
	}
	else {
		for (m = 0; m < COLUMNS; ++m) {
			if (ConnectFour_popten_pop(cf, m)) {
				switch(popTenFlags) {
				case POPTEN_POP_NO_CONNECTION:
					MoveSorter_addToEntry(&moveSorter, m + COLUMNS, 2);
					break;
				case POPTEN_POP_CONNECTION:
					MoveSorter_addToEntry(&moveSorter, m + COLUMNS, 3);
				}
				ConnectFour_popten_unpop(cf);
			}
			if (ConnectFour_popten_drop(cf, m)) {
				MoveSorter_addToEntry(&moveSorter, m, 1);
				ConnectFour_popten_undrop(cf);
			}
		}
	}
	for (m = 0; m < l; ++m) {
		sortedMove = moveSorter.moveEntries[m].move;
		if (sortedMove < COLUMNS) {
			ConnectFour_popten_drop(cf, sortedMove);
		}
		else if (sortedMove < COLUMNS_X2) {
			ConnectFour_popten_pop(cf, sortedMove - COLUMNS);
		}
		else {
			ConnectFour_popten_pass(cf);
		}

		if (popTenFlags == POPTEN_DROP || popTenFlags == POPTEN_PASS) {
			// Whenever players make a pop that is not part of a four-in-a-row connection, the turn switches to their opponent; switch alpha-beta bounds accordingly
			branchScore = -AlphaBeta_negamax_popten(cf, depth - 1, -beta, -alpha);
		}
		else {
			// A pop belonging to a four-in-a-row connection means the player gains a turn; keep the alpha-beta bounds but increase in depth
			branchScore = AlphaBeta_negamax_popten(cf, depth - 1, alpha, beta);
		}

		if (branchScore > rootScore) {
			rootScore = branchScore;
		}


		if (sortedMove < COLUMNS) {
			ConnectFour_popten_undrop(cf);
		}
		else if (sortedMove < COLUMNS_X2) {
			ConnectFour_popten_unpop(cf);
		}
		else {
			ConnectFour_popten_unpass(cf);
		}

		if (alpha < rootScore) {
			TranspositionTable_store(&table, (hashKey = ConnectFour_getHashKey(cf)), (alpha = rootScore), depth);
		}
		if (alpha >= beta) {
			free(moveSorter.moveEntries);
			return alpha;
		}
	}
	free(moveSorter.moveEntries);
	TranspositionTable_store(&table, (hashKey = ConnectFour_getHashKey(cf)), alpha, depth);
	return alpha;
}

int AlphaBeta_negamax_popten_withMoves(ConnectFour *cf, int currDepth, int maxDepth, int alpha, int beta) {
	int rootScore, branchScore;
	unsigned m, l, sortedMove;
	MoveSorter moveSorter;
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
	if (AlphaBeta_popten_checkWin(cf)) {
		return PLAYER_WIN;
	}
	if (AlphaBeta_popten_checkDraw(cf)) {
		return DRAW;
	}
	if (currDepth >= maxDepth) {
		return -IN_PROGRESS;
	}
	rootScore = alpha;
	moveSorter.moveEntries = malloc(sizeof(struct MoveSorter_Entries) * (l = ConnectFour_numberLegalMoves(cf)));
	moveSorter.size = 0;
	if (ConnectFour_popten_pass(cf)) {
		MoveSorter_addToEntry(&moveSorter, COLUMNS_X2_P1, 0);
		ConnectFour_popten_unpass(cf);
	}
	else {
		for (m = 0; m < COLUMNS; ++m) {
			if (ConnectFour_popten_pop(cf, m)) {
				switch(popTenFlags) {
				case POPTEN_POP_NO_CONNECTION:
					MoveSorter_addToEntry(&moveSorter, m + COLUMNS, 2);
					break;
				case POPTEN_POP_CONNECTION:
					MoveSorter_addToEntry(&moveSorter, m + COLUMNS, 3);
				}
				ConnectFour_popten_unpop(cf);
			}
			if (ConnectFour_popten_drop(cf, m)) {
				MoveSorter_addToEntry(&moveSorter, m, 1);
				ConnectFour_popten_undrop(cf);
			}
		}
	}
	for (m = 0; m < l; ++m) {
		sortedMove = moveSorter.moveEntries[m].move;

		if (sortedMove < COLUMNS) {
			ConnectFour_popten_drop(cf, sortedMove);
		}
		else if (sortedMove < COLUMNS_X2) {
			ConnectFour_popten_pop(cf, sortedMove - COLUMNS);
		}
		else {
			ConnectFour_popten_pass(cf);
		}

		if (popTenFlags == POPTEN_DROP || popTenFlags == POPTEN_PASS) {
			branchScore = -AlphaBeta_negamax_popten_withMoves(cf, currDepth + 1, maxDepth, -beta, -alpha);
		}
		else {
			branchScore = AlphaBeta_negamax_popten_withMoves(cf, currDepth + 1, maxDepth, alpha, beta);
		}

		if (branchScore > rootScore) {
			rootScore = branchScore;
			if (sortedMove < COLUMNS) {
				pv[currDepth] = sortedMove + '1';
			}
			else if (sortedMove < COLUMNS_X2) {
				pv[currDepth] = sortedMove + 'A' - COLUMNS;
			}
			else {
				pv[currDepth] = 'P';
			}
		}

		if (sortedMove < COLUMNS) {
			ConnectFour_popten_undrop(cf);
		}
		else if (sortedMove < COLUMNS_X2) {
			ConnectFour_popten_unpop(cf);
		}
		else {
			ConnectFour_popten_unpass(cf);
		}

		if (alpha < rootScore) {
			TranspositionTable_storeBounds(&table, (hashKey = ConnectFour_getHashKey(cf)), (alpha = rootScore), maxDepth, TT_LOWERBOUND);
		}
		if (alpha >= beta) {
			free(moveSorter.moveEntries);
			return alpha;
		}
	}

	free(moveSorter.moveEntries);
	TranspositionTable_storeBounds(&table, (hashKey = ConnectFour_getHashKey(cf)), alpha, maxDepth, TT_UPPERBOUND);
	return alpha;
}

void AlphaBeta_normal_getMoveScores(ConnectFour *cf, Result *result, Result *best, bool out) {
	if (!ConnectFour_gameOver(cf)) {
		Result *results = malloc(sizeof(Result) * COLUMNS);
		bool isSymmetric = ConnectFour_symmetrical(cf->board[0]) && ConnectFour_symmetrical(cf->board[1]), madeMove = false;
		unsigned c, cols = isSymmetric ? COLUMNS & 1 ? (COLUMNS >> 1) + 1 : COLUMNS >> 1 : COLUMNS;

		for (c = 0; c < COLUMNS; ++c) {
			results[c].wdl = (char)UNKNOWN_CHAR;
		}

		if (GAME_VARIANT == POWERUP_VARIANT) {
			printf("normal: ");
		}

		for (c = 0; c < cols; ++c) {
#ifdef __unix__ // The standard output is line buffered on Linux. It has no effect on Windows since the standard output is not line buffered.
			fflush(stdout);
#endif
			switch (GAME_VARIANT) {
			case POPOUT_VARIANT:
				madeMove = ConnectFour_popout_drop(cf, c);
				break;
			case POWERUP_VARIANT:
				madeMove = ConnectFour_powerup_drop(cf, c);
				break;
			default:
				madeMove = ConnectFour_normal_drop(cf, c);
			}

			if (madeMove) {
				if (ConnectFour_connection(cf->board[!(cf->plyNumber & 1)])) {
					results[c] = (Result){ WIN_CHAR, 0 };
					if (isSymmetric) {
						results[COLUMNS - 1 - c].wdl = results[c].wdl;
						results[COLUMNS - 1 - c].dtc = results[c].dtc;
					}
				}
				else {
					TranspositionTable_dynamicReset(&table, tableSize);
					if (GAME_VARIANT == POPOUT_VARIANT) {
						ConnectFour_clearHistory(cf);
					}
					switch (GAME_VARIANT) {
					case POPOUT_VARIANT:
						results[c] = AlphaBeta_popout_solve(cf, false);
						break;
					case POWERUP_VARIANT:
						results[c] = AlphaBeta_powerup_solve(cf, false);
						break;
					default:
						results[c] = AlphaBeta_normal_solve(cf, false);
					}
					Result_increment(&results[c]);
					if (isSymmetric) {
						results[COLUMNS - 1 - c].wdl = results[c].wdl;
						results[COLUMNS - 1 - c].dtc = results[c].dtc;
					}
				}
				switch (GAME_VARIANT) {
				case POPOUT_VARIANT:
					 ConnectFour_popout_undrop(cf);
					break;
				case POWERUP_VARIANT:
					ConnectFour_powerup_undrop(cf);
					break;
				default:
					ConnectFour_normal_undrop(cf);
				}
			}
			if (out) {
				Result_print(&results[c], best);
			}
		}
		if (isSymmetric && out) {
			for (; c < COLUMNS; ++c) {
				Result_print(&results[c], best);
			}
		}
		if (result) {
			for (c = 0; c < COLUMNS; ++c) {
				result[c] = results[c];
			}
		}
		free(results);
	}
}

void AlphaBeta_popout_getMoveScores(ConnectFour *cf, Result *result, Result *best, bool out) {
	AlphaBeta_normal_getMoveScores(cf, result, best, out);
	puts("");
	if (!ConnectFour_gameOver(cf)) {
		Result *results = malloc(sizeof(Result) * COLUMNS);
		bool isSymmetric = ConnectFour_symmetrical(cf->board[0]) && ConnectFour_symmetrical(cf->board[1]);
		unsigned c, cols = isSymmetric ? COLUMNS & 1 ? (COLUMNS >> 1) + 1 : COLUMNS >> 1 : COLUMNS;
		for (c = 0; c < COLUMNS; ++c) {
			results[c].wdl = (char)UNKNOWN_CHAR;
		}
		for (c = 0; c < cols; ++c) {
#ifdef __unix__
			fflush(stdout);
#endif
			if (ConnectFour_popout_pop(cf, c)) {
				if (ConnectFour_connectionNoVertical(cf->board[!(cf->plyNumber & 1)])) {
					results[c] = (Result){ WIN_CHAR, 0 };
					if (isSymmetric) {
						results[COLUMNS - 1 - c].wdl = results[c].wdl;
						results[COLUMNS - 1 - c].dtc = results[c].dtc;
					}
				}
				else if (ConnectFour_connectionNoVertical(cf->board[cf->plyNumber & 1])) {
					results[c] = (Result){ LOSS_CHAR, 0 };
					if (isSymmetric) {
						results[COLUMNS - 1 - c].wdl = results[c].wdl;
						results[COLUMNS - 1 - c].dtc = results[c].dtc;
					}
				}
				else {
					TranspositionTable_dynamicReset(&table, tableSize);
					ConnectFour_clearHistory(cf);
					results[c] = AlphaBeta_popout_solve(cf, false);
					Result_increment(&results[c]);
					if (isSymmetric) {
						results[COLUMNS - 1 - c].wdl = results[c].wdl;
						results[COLUMNS - 1 - c].dtc = results[c].dtc;
					}
				}
				ConnectFour_popout_unpop(cf);
			}
			if (out) {
				Result_print(&results[c], best);
			}
		}
		if (isSymmetric && out) {
			for (; c < COLUMNS; ++c) {
				Result_print(&results[c], best);
			}
		}
		if (result) {
			for (c = COLUMNS; c < COLUMNS_X2; ++c) {
				result[c] = results[c - COLUMNS];
			}
		}
		free(results);
	}
}

void AlphaBeta_powerup_getMoveScores(ConnectFour *cf, Result *result, Result *best, bool out) {
	AlphaBeta_normal_getMoveScores(cf, result, best, out);
	puts("");
	if (!ConnectFour_gameOver(cf)) {
		Result *anvilResults, *bombResults, *wallResults, *x2Results;
		bool isSymmetric = ConnectFour_symmetrical(cf->board[0]) && ConnectFour_symmetrical(cf->board[1]);
		unsigned c, cols = isSymmetric ? COLUMNS & 1 ? (COLUMNS >> 1) + 1 : COLUMNS >> 1 : COLUMNS;
		anvilResults = malloc(sizeof(Result) * COLUMNS);
		bombResults = malloc(sizeof(Result) * COLUMNS);
		wallResults = malloc(sizeof(Result) * COLUMNS);
		x2Results = malloc(sizeof(Result) * COLUMNS);

		printf("anvil: ");

		for (c = 0; c < cols; ++c) {
#ifdef __unix__
			fflush(stdout);
#endif
			if (ConnectFour_powerup_dropAnvil(cf, c)) {
				if (ConnectFour_connectionNoVertical(cf->board[!(cf->plyNumber & 1)])) {
					anvilResults[c] = (Result){ WIN_CHAR, 0 };
					if (isSymmetric) {
						anvilResults[COLUMNS - 1 - c].wdl = anvilResults[c].wdl;
						anvilResults[COLUMNS - 1 - c].dtc = anvilResults[c].dtc;
					}
				}
				else {
					TranspositionTable_dynamicReset(&table, tableSize);
					anvilResults[c] = AlphaBeta_powerup_solve(cf, false);
					Result_increment(&anvilResults[c]);
					if (isSymmetric) {
						anvilResults[COLUMNS - 1 - c].wdl = anvilResults[c].wdl;
						anvilResults[COLUMNS - 1 - c].dtc = anvilResults[c].dtc;
					}
				}
				ConnectFour_powerup_undropAnvil(cf);
			}
			if (out) {
				Result_print(&anvilResults[c], best);
			}
		}
		if (isSymmetric && out) {
			for (; c < COLUMNS; ++c) {
				Result_print(&anvilResults[c], best);
			}
		}

		printf("\nbomb: ");

		for (c = 0; c < COLUMNS; ++c) {
#ifdef __unix__
			fflush(stdout);
#endif
			if (ConnectFour_powerup_dropBomb(cf, c)) {
				if (ConnectFour_connection(cf->board[cf->plyNumber & 1])) {
					bombResults[c] = (Result){ WIN_CHAR, 0 };
					if (isSymmetric) {
						bombResults[COLUMNS - 1 - c].wdl = bombResults[c].wdl;
						bombResults[COLUMNS - 1 - c].dtc = bombResults[c].dtc;
					}
				}
				else {
					TranspositionTable_dynamicReset(&table, tableSize);
					bombResults[c] = AlphaBeta_powerup_solve(cf, false);
					++bombResults[c].dtc;
					if (isSymmetric) {
						bombResults[COLUMNS - 1 - c].wdl = bombResults[c].wdl;
						bombResults[COLUMNS - 1 - c].dtc = bombResults[c].dtc;
					}
				}
				ConnectFour_powerup_undropBomb(cf);
			}
			else if (ConnectFour_powerup_pop(cf, c)) {
				if (ConnectFour_connectionNoVertical(cf->board[!(cf->plyNumber & 1)])) {
					bombResults[c] = (Result){ WIN_CHAR, 0 };
					if (isSymmetric) {
						bombResults[COLUMNS - 1 - c].wdl = bombResults[c].wdl;
						bombResults[COLUMNS - 1 - c].dtc = bombResults[c].dtc;
					}
				}
				else if (ConnectFour_connectionNoVertical(cf->board[cf->plyNumber & 1])) {
					bombResults[c] = (Result){ LOSS_CHAR, 0 };
					if (isSymmetric) {
						bombResults[COLUMNS - 1 - c].wdl = bombResults[c].wdl;
						bombResults[COLUMNS - 1 - c].dtc = bombResults[c].dtc;
					}
				}
				else {
					TranspositionTable_dynamicReset(&table, tableSize);
					bombResults[c] = AlphaBeta_powerup_solve(cf, false);
					Result_increment(&bombResults[c]);
					if (isSymmetric) {
						bombResults[COLUMNS - 1 - c].wdl = bombResults[c].wdl;
						bombResults[COLUMNS - 1 - c].dtc = bombResults[c].dtc;
					}
				}
				ConnectFour_powerup_unpop(cf);
			}
			if (out) {
				Result_print(&bombResults[c], best);
			}
		}
		if (isSymmetric && out) {
			for (; c < COLUMNS; ++c) {
				Result_print(&bombResults[c], best);
			}
		}

		printf("\nwall: ");

		for (c = 0; c < cols; ++c) {
#ifdef __unix__
			fflush(stdout);
#endif
			if (ConnectFour_powerup_dropWall(cf, c)) {
				TranspositionTable_dynamicReset(&table, tableSize);
				wallResults[c] = AlphaBeta_powerup_solve(cf, false);
				if (cf->pum[cf->plyNumber].status == POWERUP_DROP_WALL) {
					++wallResults[c].dtc;
				}
				else {
					Result_increment(&bombResults[c]);
				}
				if (isSymmetric) {
					wallResults[COLUMNS - 1 - c].wdl = wallResults[c].wdl;
					wallResults[COLUMNS - 1 - c].dtc = wallResults[c].dtc;
				}
				ConnectFour_powerup_undropWall(cf);
			}
			if (out) {
				Result_print(&wallResults[c], best);
			}
		}
		if (isSymmetric && out) {
			for (; c < COLUMNS; ++c) {
				Result_print(&wallResults[c], best);
			}
		}

		printf("\nx2: ");

		for (c = 0; c < cols; ++c) {
#ifdef __unix__
			fflush(stdout);
#endif
			if (ConnectFour_powerup_dropX2(cf, c)) {
				if (ConnectFour_connection(cf->board[cf->plyNumber & 1])) {
					x2Results[c] = (Result){ WIN_CHAR, 0 };
					if (isSymmetric) {
						x2Results[COLUMNS - 1 - c].wdl = x2Results[c].wdl;
						x2Results[COLUMNS - 1 - c].dtc = x2Results[c].dtc;
					}
				}
				else {
					TranspositionTable_dynamicReset(&table, tableSize);
					x2Results[c] = AlphaBeta_powerup_solve(cf, false);
					if (cf->pum[cf->plyNumber].status == POWERUP_DROP_X2) {
						++x2Results[c].dtc;
					}
					else {
						Result_increment(&bombResults[c]);
					}
					if (isSymmetric) {
						x2Results[COLUMNS - 1 - c].wdl = x2Results[c].wdl;
						x2Results[COLUMNS - 1 - c].dtc = x2Results[c].dtc;
					}
				}
				ConnectFour_powerup_undropX2(cf);
			}
			if (out) {
				Result_print(&x2Results[c], best);
			}
		}
		if (isSymmetric && out) {
			for (; c < COLUMNS; ++c) {
				Result_print(&x2Results[c], best);
			}
		}

		free(anvilResults);
		free(bombResults);
		free(wallResults);
		free(x2Results);
	}
}


void AlphaBeta_popten_getMoveScores(ConnectFour *cf, Result *result, Result *best, bool out) {
	if (!ConnectFour_gameOver(cf)) {
		Result *results = malloc(sizeof(Result) * ConnectFour_getMoveSize());
		bool isSymmetric = ConnectFour_symmetrical(cf->board[0]) && ConnectFour_symmetrical(cf->board[1]);
		unsigned c, cols = isSymmetric ? COLUMNS & 1 ? (COLUMNS >> 1) + 1 : COLUMNS >> 1 : COLUMNS;
		for (c = 0; c < COLUMNS_X2; ++c) {
			results[c].wdl = (char)UNKNOWN_CHAR;
		}
		if (ConnectFour_popten_pass(cf)) {
			results[(c = 0)] = AlphaBeta_popten_solve(cf, false);
			Result_increment(&results[c]);
			ConnectFour_popten_unpass(cf);
			if (out) {
				printf("Pass");
			}
			puts("");
			goto resultAssignment;
		}
		for (c = 0; c < cols; ++c) {
#ifdef __unix__
			fflush(stdout);
#endif
			if (ConnectFour_popten_drop(cf, c)) {
				TranspositionTable_dynamicReset(&table, tableSize);
				results[c] = AlphaBeta_popten_solve(cf, false);
				Result_increment(&results[c]);
				if (isSymmetric) {
					results[COLUMNS_M1 - c].wdl = results[c].wdl;
					results[COLUMNS_M1 - c].dtc = results[c].dtc;
				}
				ConnectFour_popten_undrop(cf);
			}
			if (out) {
				Result_print(&results[c], best);
			}
		}
		if (isSymmetric && out) {
			for (; c < COLUMNS; ++c) {
				Result_print(&results[c], best);
			}
		}
		puts("");
		for (c = 0; c < cols; ++c) {
#ifdef __unix__
			fflush(stdout);
#endif
			if (ConnectFour_popten_pop(cf, c)) {
				if (ConnectFour_hasTenDisks(cf)) {
					results[c + COLUMNS] = (Result){ WIN_CHAR, 0 };
					if (isSymmetric) {
						results[COLUMNS_X2 - 1 - c].wdl = results[c + COLUMNS].wdl;
						results[COLUMNS_X2 - 1 - c].dtc = results[c + COLUMNS].dtc;
					}
				}
				else {
					TranspositionTable_dynamicReset(&table, tableSize);
					results[c + COLUMNS] = AlphaBeta_popten_solve(cf, false);
					++results[c + COLUMNS].dtc;
					if (isSymmetric) {
						results[COLUMNS_X2 - 1 - c].wdl = results[c + COLUMNS].wdl;
						results[COLUMNS_X2 - 1 - c].dtc = results[c + COLUMNS].dtc;
					}
				}
				ConnectFour_popten_unpop(cf);
			}
			if (out) {
				Result_print(&results[c + COLUMNS], best);
			}
		}
		if (isSymmetric && out) {
			for (; c < COLUMNS; ++c) {
				Result_print(&results[c + COLUMNS], best);
			}
		}
		resultAssignment:
		if (result) {
			for (c = 0; c < COLUMNS_X2; ++c) {
				result[c] = results[c];
			}
		}
		free(results);
	}
}

int AlphaBeta_normal_getBestMove(ConnectFour *cf, Result *result, Result *best, bool out) {
	AlphaBeta_normal_getMoveScores(cf, result, best, out);
	if (!ConnectFour_gameOver(cf)) {
		return Result_getBestMove(result, ConnectFour_getMoveSize());
	}
	return -1;
}

int AlphaBeta_popout_getBestMove(ConnectFour *cf, Result *result, Result *best, bool out) {
	AlphaBeta_popout_getMoveScores(cf, result, best, out);
	if (!ConnectFour_gameOver(cf)) {
		return Result_getBestMove(result, ConnectFour_getMoveSize());
	}
	return -1;
}

int AlphaBeta_powerup_getBestMove(ConnectFour *cf, Result *result, Result *best, bool out) {
	AlphaBeta_powerup_getMoveScores(cf, result, best, out);
	if (!ConnectFour_gameOver(cf)) {
		return Result_getBestMove(result, ConnectFour_getMoveSize());
	}
	return -1;
}

int AlphaBeta_popten_getBestMove(ConnectFour *cf, Result *result, Result *best, bool out) {
	AlphaBeta_popten_getMoveScores(cf, result, best, out);
	if (!ConnectFour_gameOver(cf)) {
		return Result_getBestMove(result, ConnectFour_getMoveSize());
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
	for (; i && sorter->moveEntries[i - 1].score < score; --i) {
		sorter->moveEntries[i] = sorter->moveEntries[i - 1];
	}
	sorter->moveEntries[i].move = move;
	sorter->moveEntries[i].score = score;
}

int MoveSorter_obtainNextMove(MoveSorter *sorter) {
	return sorter->size ? sorter->moveEntries[--sorter->size].move : -1;
}

Result AlphaBeta_normal_solve(ConnectFour *cf, const bool VERBOSE) {
	int depth = MOVESIZE - cf->plyNumber, solution = 0, d;
	for (d = 0; d < depth; ++d) {
		if (VERBOSE) {
			printf(SEARCHING_STRING, d);
#ifdef __unix__
			fflush(stdout);
#endif
		}
		if ((solution = AlphaBeta_negamax_normal(cf, d, -PLAYER_WIN, PLAYER_WIN))) {
			return (Result) { solution > DRAW ? WIN_CHAR : LOSS_CHAR, d };
		}
	}
	return DRAW_RESULT;
}

Result AlphaBeta_popout_solve(ConnectFour *cf, const bool VERBOSE) {
	int depth = HISTORYSIZE + AREA + (AREA >> 1), solution = DRAW, d;
	repetitionFlag = 1;
	for (d = 0; d < depth; ++d) {
		if (VERBOSE) {
			printf(SEARCHING_STRING, d);
#ifdef __unix__
			fflush(stdout);
#endif
		}
		if (abs((solution = AlphaBeta_negamax_popout(cf, d, -PLAYER_WIN, PLAYER_WIN))) != IN_PROGRESS) {
			if (((solution == PLAYER_WIN) && !(d & 1)) || ((solution == -PLAYER_WIN) && (d & 1))) {
				return (Result) {solution > DRAW ? WIN_CHAR : LOSS_CHAR, d };
			}
			else {
				return DRAW_RESULT;
			}
		}
	}
	return DRAW_RESULT;
}

Result AlphaBeta_powerup_solve(ConnectFour *cf, const bool VERBOSE) {
	int depth = MOVESIZE - cf->plyNumber, solution = DRAW, d;
	for (d = 0; d < depth; ++d) {
		if (VERBOSE) {
			printf(SEARCHING_STRING, d);
#ifdef __unix__
			fflush(stdout);
#endif
		}
		if (abs((solution = AlphaBeta_negamax_powerup(cf, d, -PLAYER_WIN, PLAYER_WIN))) != IN_PROGRESS) {
			if (abs(solution) >= PLAYER_WIN) {
				return (Result) { solution > DRAW ? WIN_CHAR : LOSS_CHAR, d };
			}
			else {
				return DRAW_RESULT;
			}
		}
	}
	return UNKNOWN_RESULT;
}

Result AlphaBeta_popten_solve(ConnectFour *cf, const bool VERBOSE) {
	int depth = MOVESIZE - cf->plyNumber, solution = DRAW, d;
	for (d = 0; d < depth; ++d) {
		if (VERBOSE) {
			printf(SEARCHING_STRING, d);
#ifdef __unix__
			fflush(stdout);
#endif
		}
		if (abs((solution = AlphaBeta_negamax_popten(cf, d, -PLAYER_WIN, PLAYER_WIN))) != IN_PROGRESS) {
			if (abs(solution) == PLAYER_WIN) {
				return (Result) {solution > DRAW ? WIN_CHAR : LOSS_CHAR, d };
			}
			else {
				return DRAW_RESULT;
			}
		}
	}
	return UNKNOWN_RESULT;
}
