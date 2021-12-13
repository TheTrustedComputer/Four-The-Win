#include "book.h"

bool BookFile_create(char *fileName) {
#ifdef _MSC_VER
	FILE *newFile;
	errno_t errors = fopen_s(&newFile, fileName, "a");
#else
	FILE *newFile = fopen(fileName, "a");
#endif
	if (newFile) {
#ifdef _MSC_VER
		if (errors) {
			fprintf(stderr, "%s%s", BOOK_UNABLE_OPEN_FILE, BOOK_CREATING_STRING);
			return false;
		}
#endif
		if (fclose(newFile) == EOF) {
			fprintf(stderr, "%s%s", BOOK_UNABLE_CLOSE_FILE, BOOK_CREATING_STRING);
			errno = EIO;
			return false;
		}
		return BookFile_write(fileName);
	}
	else {
		fprintf(stderr, "%s%s", BOOK_UNABLE_OPEN_FILE, BOOK_CREATING_STRING);
		return false;
	}
}

bool BookFile_readFromDrive(char *fileName) {
	if (GAME_VARIANT != POPTEN_VARIANT) {
#ifdef _MSC_VER
		FILE *fetchedFile;
		errno_t errors = fopen_s(&fetchedFile, fileName, "rb");
#else
		FILE *fetchedFile = fopen(fileName, "rb");
#endif
		if (fetchedFile) {
#ifdef _MSC_VER
			if (errors) {
				fprintf(stderr, "%s%s", BOOK_UNABLE_OPEN_FILE, BOOK_READING_STRING);
				return false;
			}
#endif
			if (fread(bookFile.signature, sizeof(char), 4, fetchedFile) != 4) {
				if (strncmp(bookFile.signature, BOOK_SIGNATURE, 4)) {
					fprintf(stderr, "Signature \"%s\" was not found.\n", BOOK_SIGNATURE);
					errno = EBADF;
				}
				fclose(fetchedFile);
				return false;
			}
			if (fread(&bookFile.columns, sizeof(uint8_t), 1, fetchedFile) != 1) {
				if (bookFile.columns != COLUMNS) {
					fprintf(stderr, "%s%s", BOOKFILE_WRONG, "column size.\n");
					errno = EINVAL;
				}
				fclose(fetchedFile);
				return false;
			}
			if (fread(&bookFile.rows, sizeof(uint8_t), 1, fetchedFile) != 1) {
				if (bookFile.rows != ROWS) {
					fprintf(stderr, "%s%s", BOOKFILE_WRONG, "row size.\n");
					errno = EINVAL;
				}
				fclose(fetchedFile);
				return false;
			}
			if (fread(&bookFile.variant, sizeof(uint8_t), 1, fetchedFile) != 1) {
				if (bookFile.variant != GAME_VARIANT) {
					fprintf(stderr, "Variant mismatch between actual %d and book %d.\n", GAME_VARIANT, bookFile.variant);
					errno = EINVAL;
				}
				fclose(fetchedFile);
				return false;
			}
			if (fread(&bookFile.numRecords, sizeof(unsigned), 1, fetchedFile) != 1) {
				fprintf(stderr, "Read an unknown number of records.\n");
				errno = EINVAL;
				fclose(fetchedFile);
				return false;
			}
			if (fclose(fetchedFile) == EOF) {
				fprintf(stderr, "%s%s", BOOK_UNABLE_CLOSE_FILE, BOOK_READING_STRING);
				errno = EIO;
				return false;
			}
			return true;
		}
		else {
			fprintf(stderr, "%s%s", BOOK_UNABLE_OPEN_FILE, BOOK_READING_STRING);
			return false;
		}
	}
	else {
		fprintf(stderr, "Opening books are unsupported in Pop Ten.\n");
		errno = EPERM;
		return false;
	}
}

bool BookFile_write(char *fileName) {
#ifdef _MSC_VER
	FILE *storedFile;
	errno_t errors = fopen_s(&storedFile, fileName, "rb+");
#else
	FILE *storedFile = fopen(fileName, "rb+");
#endif
	if (storedFile) {
#ifdef _MSC_VER
		if (errors) {
			fprintf(stderr, "%s%s", BOOK_UNABLE_OPEN_FILE, BOOK_WRITING_STRING);
			return false;
		}
#endif
		if (fwrite(BOOK_SIGNATURE, sizeof(char), 4, storedFile) != 4) {
			fprintf(stderr, "Could not write the %s signature to the opening book.\n", BOOK_UNABLE_OPEN_FILE);
			errno = EIO;
			fclose(storedFile);
			return false;
		}
		if (fputc(COLUMNS, storedFile) == EOF) {
			fprintf(stderr, "Could not write the column size to the opening book.\n");
			errno = EIO;
			fclose(storedFile);
			return false;
		}
		if (fputc(ROWS, storedFile) == EOF) {
			fprintf(stderr, "Could not write the row size to the opening book.\n");
			errno = EIO;
			fclose(storedFile);
			return false;
		}
		if (fputc(GAME_VARIANT, storedFile) == EOF) {
			fprintf(stderr, "Could not write the game variant to the opening book.\n");
			errno = EIO;
			fclose(storedFile);
			return false;
		}
		if (fwrite(&bookFile.numRecords, sizeof(unsigned), 1, storedFile) != 1) {
			fprintf(stderr, "Could not write the number of records to the opening book.\n");
			errno = EIO;
			fclose(storedFile);
			return false;
		}
		if (fclose(storedFile) == EOF) {
			fprintf(stderr, "%s%s", BOOK_UNABLE_CLOSE_FILE, BOOK_WRITING_STRING);
			errno = EIO;
			return false;
		}
	}
	else {
		fprintf(stderr, "%s%s", BOOK_UNABLE_OPEN_FILE, BOOK_WRITING_STRING);
		return false;
	}
	return true;
}

bool BookFile_append(char *fileName, ConnectFour *cf, Position hash, Result *r) {
#ifdef _MSC_VER
	FILE *appendedFile;
	errno_t errors = fopen_s(&appendedFile, fileName, "ab+");
#else
	FILE *appendedFile = fopen(fileName, "ab+");
#endif
	if (appendedFile) {
#ifdef _MSC_VER
		if (errors) {
			fprintf(stderr, "%s%s", BOOK_UNABLE_OPEN_FILE, BOOK_APPENDING_STRING);
			return false;
		}
#endif
		if (BookFile_hashLookup(appendedFile, cf, hash, true)) {
			fclose(appendedFile);
			return false;
		}
		fwrite(&hash, hashBytes, 1, appendedFile);
		switch (GAME_VARIANT) {
		case NORMAL_VARIANT:
			BookFile_normal_append(appendedFile, cf, r);
			break;
		case POPOUT_VARIANT:
			BookFile_popout_append(appendedFile, cf, r);
		}
		if (fclose(appendedFile) == EOF) {
			fprintf(stderr, "%s%s", BOOK_UNABLE_CLOSE_FILE, BOOK_APPENDING_STRING);
			return false;
		}
	}
	else {
		fprintf(stderr, "%s%s", BOOK_UNABLE_OPEN_FILE, BOOK_APPENDING_STRING);
		return false;
	}
#ifdef _MSC_VER
	errors = fopen_s(&appendedFile, fileName, "rb+");
#else
	appendedFile = fopen(fileName, "rb+");
#endif
	if (appendedFile) {
#ifdef _MSC_VER
		if (errors) {
			fprintf(stderr, "%s%s", BOOK_UNABLE_OPEN_FILE, BOOK_UPDATING_STRING);
			return false;
		}
#endif
		fseek(appendedFile, BOOKENTRY_NUMRECORDS_OFFSET, SEEK_SET);
		if (fwrite(&bookFile.numRecords, sizeof(unsigned), 1, appendedFile) != 1) {
			fprintf(stderr, "A write error occurred when updating the number of records from book.\n");
			errno = EIO;
			fclose(appendedFile);
			return false;
		}
		if (fclose(appendedFile) == EOF) {
			fprintf(stderr, "%s%s", BOOK_UNABLE_CLOSE_FILE, BOOK_UPDATING_STRING);
			return false;
		}
		return true;
	}
	else {
		fprintf(stderr, "%s%s", BOOK_UNABLE_OPEN_FILE, BOOK_UPDATING_STRING);
		return false;
	}
}

void BookFile_normal_append(FILE *file, ConnectFour *cf, Result *r) {
	for (unsigned i = 0u; i < COLUMNS; ++i) {
		if (ConnectFour_normal_drop(cf, i)) {
			int8_t score = BookFile_normal_convertToEntry(&r[i]);
			fwrite(&score, sizeof(int8_t), 1, file);
			ConnectFour_normal_undrop(cf);
		}
	}
}

void BookFile_popout_append(FILE *file, ConnectFour *cf, Result *r) {
	for (unsigned i = 0u; i < COLUMNS_X2; ++i) {
		int8_t score;
		if (i < COLUMNS) {
			if (ConnectFour_popout_drop(cf, i)) {
				score = BookFile_popout_convertToEntry(&r[i]);
				fwrite(&score, sizeof(int8_t), 1, file);
				ConnectFour_popout_undrop(cf);
			}
		}
		else {
			if (ConnectFour_popout_pop(cf, i - COLUMNS)) {
				score = BookFile_popout_convertToEntry(&r[i]);
				fwrite(&score, sizeof(int8_t), 1, file);
				ConnectFour_popout_unpop(cf);
			}
		}
	}
}

bool BookFile_retrieve(char *fileName, ConnectFour *cf, Position hash, Result *r, Result *rs) {
	if (BookFile_readFromDrive(fileName)) {
		int hashResult;
#ifdef _MSC_VER
		FILE *readFile;
		errno_t errors = fopen_s(&readFile, fileName, "rb");
#else
		FILE *readFile = fopen(fileName, "rb");
#endif
		if (readFile) {
#ifdef _MSC_VER
			if (errors) {
				fprintf(stderr, "%s%s", BOOK_UNABLE_OPEN_FILE, BOOK_READING_STRING);
				return false;
			}
#endif
			switch (GAME_VARIANT) {
			case NORMAL_VARIANT:
				if (!BookFile_normal_retrieve(readFile, cf, hash, rs, &hashResult)) {
					return false;
				}
				if (hashResult == BOOKENTRY_MIRRORED) {
					ConnectFour_reverseBoard(cf);
					Result_normal_reverse(rs);
				}
				break;
			case POPOUT_VARIANT:
				if (!BookFile_popout_retrieve(readFile, cf, hash, rs, &hashResult)) {
					return false;
				}
				if (hashResult == BOOKENTRY_MIRRORED) {
					ConnectFour_reverseBoard(cf);
					Result_normal_reverse(rs);
					Result_popout_reverse(rs);
				}
			default:
				return false;
			}
			*r = Result_getBestResult(rs, ConnectFour_getMoveSize());
			if (fclose(readFile) == EOF) {
				fprintf(stderr, "%s%s", BOOK_UNABLE_CLOSE_FILE, BOOK_READING_STRING);
				return false;
			}
			return true;
		}
		else {
			fprintf(stderr, "%s%s", BOOK_UNABLE_OPEN_FILE, BOOK_READING_STRING);
			return false;
		}
	}
	return false;
}

bool BookFile_normal_retrieve(FILE *file, ConnectFour *cf, Position hash, Result *rs, int *hashResult) {
	int8_t bookResult;
	if ((*hashResult = BookFile_hashLookup(file, cf, hash, false))) {
		if (*hashResult == BOOKENTRY_MIRRORED) {
			ConnectFour_reverseBoard(cf);
		}
		for (unsigned i = 0u; i < COLUMNS; ++i) {
			if (ConnectFour_normal_drop(cf, i)) {
				rs[i] = (fread(&bookResult, sizeof(int8_t), 1, file) == 1) ? BookFile_normal_convertToResult(bookResult) : UNKNOWN_RESULT;
				ConnectFour_normal_undrop(cf);
			}
			else {
				rs[i] = UNKNOWN_RESULT;
			}
		}
	}
	else {
		fclose(file);
		return false;
	}
	return true;
}

bool BookFile_popout_retrieve(FILE *file, ConnectFour *cf, Position hash, Result *rs, int *hashResult) {
	int8_t bookResult;
	if ((*hashResult = BookFile_hashLookup(file, cf, hash, false))) {
		if (*hashResult == BOOKENTRY_MIRRORED) {
			ConnectFour_reverseBoard(cf);
		}
		for (unsigned i = 0u; i < COLUMNS_X2; ++i) {
			if (i < COLUMNS) {
				if (ConnectFour_popout_drop(cf, i)) {
					rs[i] = (fread(&bookResult, sizeof(int8_t), 1, file) == 1) ? BookFile_popout_convertToResult(bookResult) : UNKNOWN_RESULT;
					ConnectFour_popout_undrop(cf);
				}
				else {
					rs[i] = UNKNOWN_RESULT;
				}
			}
			else {
				if (ConnectFour_popout_pop(cf, i - COLUMNS)) {
					rs[i] = (fread(&bookResult, sizeof(int8_t), 1, file) == 1) ? BookFile_popout_convertToResult(bookResult) : UNKNOWN_RESULT;
					ConnectFour_popout_unpop(cf);
				}
				else {
					rs[i] = UNKNOWN_RESULT;
				}
			}
		}
	}
	else {
		fclose(file);
		return false;
	}
	return true;
}

int BookFile_hashLookup(FILE *file, ConnectFour *cf, Position targetHash, bool appendToBook) {
	Position bookHash = 0ull;
	unsigned i, valueBlocks = 1u;
	fseek(file, BOOKENTRY_OFFSET, SEEK_SET);
	for (i = 0u; i < bookFile.numRecords; ++i) {
		if (fread(&bookHash, hashBytes, 1, file) == 1) {
			switch (GAME_VARIANT) {
			case NORMAL_VARIANT:
				valueBlocks = sizeof(int8_t) * popcount(~bookHash & TOP);
				break;
			case POPOUT_VARIANT:
				valueBlocks = sizeof(int8_t) * popcount(~bookHash & TOP) + ConnectFour_countBottomDisksFromHashKey(bookHash);
			}
			if (targetHash == bookHash) {
				return BOOKENTRY_EXACT;
			}
			if (ConnectFour_reverse(targetHash) == bookHash) {
				return BOOKENTRY_MIRRORED;
			}
			if (fseek(file, valueBlocks, SEEK_CUR)) {
				puts(BOOK_EOF_REACHED_STRING);
				break;
			}
		}
		else {
			break;
		}
	}
	if (appendToBook) {
		++bookFile.numRecords;
	}
	return 0;
}

bool BookFile_generateBook(char *fileName, ConnectFour *cf, unsigned short plies) {
	unsigned i;
	uint8_t bookEntry;
	Position hash = 0ull;
	Result *results = malloc(sizeof(Result) * ConnectFour_getMoveSize());
	Result *bookResult = malloc(sizeof(Result) * ConnectFour_getMoveSize());
#ifdef _MSC_VER
	FILE *generatorFile;
	errno_t errors = fopen_s(&generatorFile, fileName, "ab+");
#else
	FILE *generatorFile = fopen(fileName, "ab+");
#endif
	currentEntries = 0;
	if (generatorFile) {
#ifdef _MSC_VER
		if (errors) {
			fprintf(stderr, "%s%s", BOOK_UNABLE_GENERATE, BOOK_READING_STRING);
			free(results);
			free(bookResult);
			return false;
		}
#endif
		ConnectFour_printMoves(cf);
		if (!BookFile_hashLookup(generatorFile, cf, (hash = ConnectFour_getHashKey(cf)), false)) {
			switch (GAME_VARIANT) {
			case NORMAL_VARIANT:
				AlphaBeta_normal_getMoveScores(cf, results, false);
				break;
			case POPOUT_VARIANT:
				AlphaBeta_popout_getMoveScores(cf, results, false);
			}
			fwrite(&hash, hashBytes, 1, generatorFile);
			for (i = 0; i < COLUMNS; ++i) {
				switch (GAME_VARIANT) {
				case NORMAL_VARIANT:
					bookEntry = BookFile_normal_convertToEntry(&results[i]);
					fwrite(&bookEntry, sizeof(uint8_t), 1, generatorFile);
					ConnectFour_normal_undrop(cf);
					break;
				case POPOUT_VARIANT:
					bookEntry = BookFile_popout_convertToEntry(&results[i]);
					fwrite(&bookEntry, sizeof(uint8_t), 1, generatorFile);
					ConnectFour_popout_undrop(cf);
				}
			}
			if (GAME_VARIANT == POPOUT_VARIANT) {
				for (i = 0; i < COLUMNS; ++i) {
					if (ConnectFour_popout_pop(cf, i)) {
						bookEntry = BookFile_popout_convertToEntry(&results[i]);
						fwrite(&bookEntry, sizeof(uint8_t), 1, generatorFile);
						ConnectFour_popout_unpop(cf);
					}
				}
			}
			++bookFile.numRecords;
			++currentEntries;
			printf(" - %s\n", BOOKFILE_APPENDED);
		}
		else {
			printf(" - %s\n", BOOKFILE_EXISTS);
		}
		switch (GAME_VARIANT) {
		case NORMAL_VARIANT:
			BookFile_normal_generateEntries(generatorFile, cf, bookResult, plies);
			break;
		case POPOUT_VARIANT:
			BookFile_popout_generateEntries(generatorFile, cf, bookResult, plies);
		}
		if (fclose(generatorFile) == EOF) {
			fprintf(stderr, "%s%s", BOOK_UNABLE_CLOSE_FILE, BOOK_GENERATING_STRING);
			free(results);
			free(bookResult);
			return false;
		}
#ifdef _MSC_VER
		errors = fopen_s(&generatorFile, fileName, "rb+");
#else
		generatorFile = fopen(fileName, "rb+");
#endif
		if (generatorFile) {
#ifdef _MSC_VER
			if (errors) {
				fprintf(stderr, "%s%s", BOOK_UNABLE_OPEN_FILE, BOOK_UPDATING_STRING);
				free(results);
				free(bookResult);
				return false;
			}
#endif
			fseek(generatorFile, BOOKENTRY_NUMRECORDS_OFFSET, SEEK_SET);
			fwrite(&bookFile.numRecords, sizeof(unsigned), 1, generatorFile);
			if (fclose(generatorFile) == EOF) {
				fprintf(stderr, "%s%s", BOOK_UNABLE_CLOSE_FILE, BOOK_UPDATING_STRING);
				free(results);
				free(bookResult);
				return false;
			}
			printf("%u %s added successfully to the book.\n", currentEntries, (currentEntries == 1) ? "entry" : "entries");
			free(results);
			free(bookResult);
			return true;
		}
		else {
			fprintf(stderr, "%s%s", BOOK_UNABLE_OPEN_FILE, BOOK_UPDATING_STRING);
			free(results);
			free(bookResult);
			return false;
		}
	}
	else {
		fprintf(stderr, "%s%s", BOOK_UNABLE_GENERATE, BOOK_READING_STRING);
		free(results);
		free(bookResult);
		return false;
	}
}

void BookFile_normal_generateEntries(FILE *file, ConnectFour *cf, Result *bookResult, unsigned plies) {
	if (cf->plyNumber < plies) {
		unsigned i, j;
		Position hash;
		uint8_t bookEntry;
		for (i = 0u; i < COLUMNS; ++i) {
			if ((GAME_VARIANT == POPOUT_VARIANT) ? ConnectFour_popout_drop(cf, i) : ConnectFour_normal_drop(cf, i)) {
				if (cf->moves[0] <= (COLUMNS >> 1u) - !(COLUMNS & 1u)) {
					ConnectFour_printMoves(cf);
#ifdef __unix__
					fflush(stdout);
#endif
					if (!BookFile_hashLookup(file, cf, (hash = ConnectFour_getHashKey(cf)), false)) {
						if (ConnectFour_connection(cf->board[!(cf->plyNumber & 1)])) {
							printf(" - %s\n", BOOKFILE_TERMINAL);
							if (GAME_VARIANT == POPOUT_VARIANT) {
								ConnectFour_popout_undrop(cf);
							}
							else {
								ConnectFour_normal_undrop(cf);
							}
							continue;
						}
						else {
							if (GAME_VARIANT == POPOUT_VARIANT) {
								AlphaBeta_popout_getMoveScores(cf, bookResult, false);
							}
							else {
								AlphaBeta_normal_getMoveScores(cf, bookResult, false);
							}
						}
						fwrite(&hash, hashBytes, 1, file);
						for (j = 0u; j < COLUMNS_X2; ++j) {
							if (j < COLUMNS) {
								if ((GAME_VARIANT == POPOUT_VARIANT) ? ConnectFour_popout_drop(cf, j) : ConnectFour_normal_drop(cf, j)) {
									bookEntry = (GAME_VARIANT == POPOUT_VARIANT) ? BookFile_popout_convertToEntry(&bookResult[j]) : BookFile_normal_convertToEntry(&bookResult[j]);
									fwrite(&bookEntry, sizeof(uint8_t), 1, file);
									if (GAME_VARIANT == POPOUT_VARIANT) {
										ConnectFour_popout_undrop(cf);
									}
									else {
										ConnectFour_normal_undrop(cf);
									}
								}
							}
							else {
								if ((GAME_VARIANT == POPOUT_VARIANT) && ConnectFour_popout_pop(cf, j - COLUMNS)) {
									bookEntry = BookFile_popout_convertToEntry(&bookResult[j - COLUMNS]);
									fwrite(&bookEntry, sizeof(uint8_t), 1, file);
									ConnectFour_popout_unpop(cf);
								}
							}
						}
						++currentEntries;
						++bookFile.numRecords;
						printf(" - %s\n", BOOKFILE_APPENDED);
						BookFile_normal_generateEntries(file, cf, bookResult, plies);
					}
					else {
						printf(" - %s\n", BOOKFILE_EXISTS);
						BookFile_normal_generateEntries(file, cf, bookResult, plies);
					}
					if (GAME_VARIANT == POPOUT_VARIANT) {
						ConnectFour_popout_undrop(cf);
					}
					else {
						ConnectFour_normal_undrop(cf);
					}
				}
				else {
					return;
				}
			}
		}
	}
}

void BookFile_popout_generateEntries(FILE *file, ConnectFour *cf, Result *bookResult, unsigned plies) {
	if (cf->plyNumber < plies) {
		unsigned i, j;
		Position hash;
		uint8_t bookEntry;
		BookFile_normal_generateEntries(file, cf, bookResult, plies);
		for (i = 0u; i < COLUMNS; ++i) {
			if (ConnectFour_popout_pop(cf, i)) {
				ConnectFour_printMoves(cf);
				if (!BookFile_hashLookup(file, cf, (hash = ConnectFour_getHashKey(cf)), false)) {
					if (ConnectFour_connectionNoVertical(cf->board[!(cf->plyNumber & 1u)])) {
						printf(" - %s\n", BOOKFILE_TERMINAL);
						ConnectFour_popout_unpop(cf);
						continue;
					}
					else if (ConnectFour_connectionNoVertical(cf->board[(cf->plyNumber & 1u)])) {
						printf(" - %s\n", BOOKFILE_TERMINAL);
						ConnectFour_popout_unpop(cf);
						continue;
					}
					else {
						AlphaBeta_popout_getMoveScores(cf, bookResult, false);
					}
					fwrite(&hash, hashBytes, 1, file);
					for (j = 0u; j < COLUMNS_X2; ++j) {
						if (j < COLUMNS) {
							if (ConnectFour_popout_drop(cf, j)) {
								bookEntry = BookFile_popout_convertToEntry(&bookResult[j]);
								fwrite(&bookEntry, sizeof(uint8_t), 1, file);
								ConnectFour_popout_undrop(cf);
							}
						}
						else {
							if (ConnectFour_popout_pop(cf, j - COLUMNS)) {
								bookEntry = BookFile_popout_convertToEntry(&bookResult[j - COLUMNS]);
								fwrite(&bookEntry, sizeof(uint8_t), 1, file);
								ConnectFour_popout_unpop(cf);
							}
						}
					}
					++currentEntries;
					++bookFile.numRecords;
					printf(" - %s\n", BOOKFILE_APPENDED);
					BookFile_popout_generateEntries(file, cf, bookResult, plies);
				}
				else {
					printf(" - %s\n", BOOKFILE_EXISTS);
					BookFile_popout_generateEntries(file, cf, bookResult, plies);
				}
				ConnectFour_popout_unpop(cf);
			}
		}
	}
}

bool BookFile_storeToTranspositionTable(char *fileName, ConnectFour *cf, TranspositionTable *tt) {
	Position bookHash;
	Result bookResult, bestBookResult;
	int8_t bookEntry;
	unsigned i, j, blockSize = 1u;
#ifdef _MSC_VER
	FILE *tableBookFile;
	errno_t errors = fopen_s(&tableBookFile, fileName, "rb");
#else
	FILE *tableBookFile = fopen(fileName, "rb");
#endif
	if (tableBookFile) {
#ifdef _MSC_VER
		if (errors) {
			fprintf(stderr, "%s%s", BOOK_UNABLE_OPEN_FILE, BOOK_READING_STRING);
			return false;
		}
		fseek(tableBookFile, BOOKENTRY_OFFSET, SEEK_SET);
		for (i = 0; i < bookFile.numRecords; ++i) {
			fread(&bookHash, hashBytes, 1, tableBookFile);
			bestBookResult = bookResult = (Result){ LOSS_CHAR, 0 };
			switch (GAME_VARIANT) {
			case NORMAL_VARIANT:
				blockSize = sizeof(int8_t) * popcount(~bookHash & TOP);
				break;
			case POPOUT_VARIANT:
				blockSize = sizeof(int8_t) * popcount(~bookHash & TOP) + ConnectFour_countBottomDisksFromHashKey(bookHash);
			}
			for (j = 0; j < COLUMNS_X2; ++j) {
				if (j < COLUMNS) {
					if ((GAME_VARIANT == POPOUT_VARIANT) ? ConnectFour_popout_drop(cf, j) : ConnectFour_normal_drop(cf, j)) {
						fread(&bookEntry, sizeof(uint8_t), 1, tableBookFile);
						bookResult = (GAME_VARIANT == POPOUT_VARIANT) ? BookFile_popout_convertToResult(bookEntry) : BookFile_normal_convertToResult(bookEntry);
						TranspositionTable_store(tt, bookHash, (bookEntry > 0) ? PLAYER_WIN : (bookEntry < 0) ? -PLAYER_WIN : DRAW, bookResult.dtc);
						if (GAME_VARIANT == POPOUT_VARIANT) {
							ConnectFour_popout_undrop(cf);
						}
						else {
							ConnectFour_normal_undrop(cf);
						}
					}
				}
				else {
					if ((GAME_VARIANT == POPOUT_VARIANT) && ConnectFour_popout_pop(cf, j - COLUMNS)) {
						fread(&bookEntry, sizeof(uint8_t), 1, tableBookFile);
						ConnectFour_popout_unpop(cf);
					}
				}
			}
			if (fseek(tableBookFile, blockSize, SEEK_CUR)) {
				fclose(tableBookFile);
				return false;
			}
		}
		if (fclose(tableBookFile) == EOF) {
			fprintf(stderr, "%s%s", BOOK_UNABLE_CLOSE_FILE, BOOK_READING_STRING);
			return false;
		}
		return true;
#endif
	}
	else {
		fprintf(stderr, "%s%s", BOOK_UNABLE_OPEN_FILE, BOOK_READING_STRING);
		return false;
	}
	return true;
}

int8_t BookFile_normal_convertToEntry(Result* result) {
	if (result->wdl == WIN_CHAR) {
		return (result->dtc >> 1) + 1;
	}
	else if (result->wdl == LOSS_CHAR) {
		return -((result->dtc >> 1) + 1);
	}
	else if (result->wdl == DRAW_CHAR) {
		return 0;
	}
	return result->dtc;
}

Result BookFile_normal_convertToResult(int8_t entry) {
	if (entry) {
		if (entry > 0) {
			return (Result) { WIN_CHAR, (entry << 1) - 2 };
		}
		else {
			return (Result) { LOSS_CHAR, -((entry << 1) + 1) };
		}
	}
	return DRAW_RESULT;
}

int8_t BookFile_popout_convertToEntry(Result* result) {
	if (result->wdl == WIN_CHAR) {
		return (result->dtc >> 1) + 1;
	}
	else if (result->wdl == LOSS_CHAR) {
		if (!result->dtc) {
			return -1;
		}
		return -((result->dtc >> 1) + 2);
	}
	else if (result->wdl == DRAW_CHAR) {
		return 0;
	}
	return result->dtc;
}

Result BookFile_popout_convertToResult(int8_t entry) {
	if (entry) {
		if (entry > 0) {
			return (Result) { WIN_CHAR, (entry << 1) - 2 };
		}
		else {
			if (entry == -1) {
				return (Result) { LOSS_CHAR, 0 };
			}
			return (Result) { LOSS_CHAR, -((entry << 1) + 3) };
		}
	}
	return DRAW_RESULT;
}
