#include "book.h"

bool BookFile_create(char *bookName) {
#ifdef _MSC_VER
	FILE *creation;
	errno_t errors = fopen_s(&creation, bookName, "a");
#else
	FILE *creation = fopen(bookName, "a");
#endif
	if (creation) {
#ifdef _MSC_VER
		if (errors) {
			fprintf(stderr, "%s%s", BOOK_UNABLE_OPEN_FILE, BOOK_CREATING_STRING);
			return false;
		}
#endif
		if (fclose(creation) == EOF) {
			fprintf(stderr, "%s%s", BOOK_UNABLE_CLOSE_FILE, BOOK_CREATING_STRING);
			errno = EIO;
			return false;
		}
		return BookFile_write(bookName);
	}
	else {
		fprintf(stderr, "%s%s", BOOK_UNABLE_OPEN_FILE, BOOK_CREATING_STRING);
		return false;
	}
}

bool BookFile_readFromDrive(char *bookName) {
	if (GAME_VARIANT != POPTEN_VARIANT) {
#ifdef _MSC_VER
		FILE *reader;
		errno_t errors = fopen_s(&reader, bookName, "rb");
#else
		FILE *reader = fopen(bookName, "rb");
#endif
		if (reader) {
#ifdef _MSC_VER
			if (errors) {
				fprintf(stderr, "%s%s", BOOK_UNABLE_OPEN_FILE, BOOK_READING_STRING);
				return false;
			}
#endif
			// Check the header and print to stderr whenever there is inconsistency
			if (fread(bookFile.signature, sizeof(bookFile.signature), 1, reader) != 1) {
				if (strncmp(bookFile.signature, BOOK_SIGNATURE, 4)) {
					fprintf(stderr, "Signature \"%s\" was not found.\n", BOOK_SIGNATURE);
					errno = EBADF;
				}
				fclose(reader);
				return false;
			}
			if (fread(&bookFile.columns, sizeof(bookFile.columns), 1, reader) != 1) {
				if (bookFile.columns != COLUMNS) {
					fprintf(stderr, "%s%s", BOOKFILE_WRONG, "column size.\n");
					errno = EINVAL;
				}
				fclose(reader);
				return false;
			}
			if (fread(&bookFile.rows, sizeof(bookFile.rows), 1, reader) != 1) {
				if (bookFile.rows != ROWS) {
					fprintf(stderr, "%s%s", BOOKFILE_WRONG, "row size.\n");
					errno = EINVAL;
				}
				fclose(reader);
				return false;
			}
			if (fread(&bookFile.variant, sizeof(bookFile.variant), 1, reader) != 1) {
				if (bookFile.variant != GAME_VARIANT) {
					fprintf(stderr, "Variant mismatch between actual %d and book %d.\n", GAME_VARIANT, bookFile.variant);
					errno = EINVAL;
				}
				fclose(reader);
				return false;
			}
			if (fread(&bookFile.numRecords, sizeof(bookFile.numRecords), 1, reader) != 1) {
				fprintf(stderr, "Read an unknown number of records.\n");
				errno = EINVAL;
				fclose(reader);
				return false;
			}
			if (fclose(reader) == EOF) {
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
		//fprintf(stderr, "Opening books are unsupported in Pop Ten.\n");
		errno = EPERM;
		return false;
	}
}

bool BookFile_write(char *bookName) {
#ifdef _MSC_VER
	FILE *writer;
	errno_t errors = fopen_s(&writer, bookName, "rb+");
#else
	FILE *writer = fopen(bookName, "rb+");
#endif
	if (writer) {
#ifdef _MSC_VER
		if (errors) {
			fprintf(stderr, "%s%s", BOOK_UNABLE_OPEN_FILE, BOOK_WRITING_STRING);
			return false;
		}
#endif
		// Check for any errors while writing the header
		if (fwrite(BOOK_SIGNATURE, sizeof(char), 4, writer) != 4) {
			fprintf(stderr, "Could not write the %s signature to the opening book.\n", BOOK_UNABLE_OPEN_FILE);
			errno = EIO;
			fclose(writer);
			return false;
		}
		if (fputc(COLUMNS, writer) == EOF) {
			fprintf(stderr, "Could not write the column size to the opening book.\n");
			errno = EIO;
			fclose(writer);
			return false;
		}
		if (fputc(ROWS, writer) == EOF) {
			fprintf(stderr, "Could not write the row size to the opening book.\n");
			errno = EIO;
			fclose(writer);
			return false;
		}
		if (fputc(GAME_VARIANT, writer) == EOF) {
			fprintf(stderr, "Could not write the game variant to the opening book.\n");
			errno = EIO;
			fclose(writer);
			return false;
		}
		if (fwrite(&bookFile.numRecords, sizeof(unsigned), 1, writer) != 1) {
			fprintf(stderr, "Could not write the number of records to the opening book.\n");
			errno = EIO;
			fclose(writer);
			return false;
		}
		if (fclose(writer) == EOF) {
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

bool BookFile_append(char *fileName, ConnectFour *appendCF, Position appendHash, Result *appendResult) {
#ifdef _MSC_VER
	FILE *appender;
	errno_t errors = fopen_s(&appender, fileName, "ab+");
#else
	FILE *appender = fopen(fileName, "ab+");
#endif
	if (appender) {
#ifdef _MSC_VER
		if (errors) {
			fprintf(stderr, "%s%s", BOOK_UNABLE_OPEN_FILE, BOOK_APPENDING_STRING);
			return false;
		}
#endif
		// Stop if there is already an entry in the book
		if (BookFile_hashLookup(appender, appendCF, appendHash, true)) {
			fclose(appender);
			return false;
		}

		fwrite(&appendHash, hashBytes, 1, appender);

		switch (GAME_VARIANT) {
		case NORMAL_VARIANT:
			BookFile_normal_append(appender, appendCF, appendResult);
			break;
		case POPOUT_VARIANT:
			BookFile_popout_append(appender, appendCF, appendResult);
		}

		if (fclose(appender) == EOF) {
			fprintf(stderr, "%s%s", BOOK_UNABLE_CLOSE_FILE, BOOK_APPENDING_STRING);
			return false;
		}
	}
	else {
		fprintf(stderr, "%s%s", BOOK_UNABLE_OPEN_FILE, BOOK_APPENDING_STRING);
		return false;
	}
#ifdef _MSC_VER
	errors = fopen_s(&appender, fileName, "rb+");
#else
	appender = fopen(fileName, "rb+");
#endif
	if (appender) {
#ifdef _MSC_VER
		if (errors) {
			fprintf(stderr, "%s%s", BOOK_UNABLE_OPEN_FILE, BOOK_UPDATING_STRING);
			return false;
		}
#endif
		fseek(appender, BOOKENTRY_NUMRECORDS_OFFSET, SEEK_SET);

		// Try to handle write errors gracefully
		if (fwrite(&bookFile.numRecords, sizeof(unsigned), 1, appender) != 1) {
			fprintf(stderr, "A write error occurred when updating the number of records from book.\n");
			errno = EIO;
			fclose(appender);
			return false;
		}
		if (fclose(appender) == EOF) {
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

void BookFile_normal_append(FILE *normalAppender, ConnectFour *normalCF, Result *normalResult) {
	for (unsigned d = 0u; d < COLUMNS; ++d) {
		if (ConnectFour_normal_drop(normalCF, d)) {
			int8_t score = BookFile_normal_convertToEntry(&normalResult[d]);
			fwrite(&score, sizeof(score), 1, normalAppender);
			ConnectFour_normal_undrop(normalCF);
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
		int hashResult = 0, movesHashResult;
		unsigned i;
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
			subMovesNotInBook = false;

			if (GAME_VARIANT == NORMAL_VARIANT) {
				if (!BookFile_normal_retrieve(readFile, cf, hash, r, &hashResult)) {
					return false;
				}
			}
			else if (GAME_VARIANT == POPOUT_VARIANT) {
				if (!BookFile_popout_retrieve(readFile, cf, hash, r, &hashResult, 0, false)) {
					return false;
				}
			}
			else if (GAME_VARIANT == POWERUP_VARIANT) {

			}
			else if (GAME_VARIANT == FIVEINAROW_VARIANT) {

			}

			if (hashResult == BOOKENTRY_MIRRORED) {
				ConnectFour_reverseBoard(cf);
			}

			if (GAME_VARIANT == NORMAL_VARIANT) {
				for (i = 0; i < COLUMNS_X2; ++i) {
					if (ConnectFour_normal_drop(cf, i)) {
						if (!BookFile_normal_retrieve(readFile, cf, ConnectFour_getHashKey(cf), &rs[i], &movesHashResult)) {
							rs[i] = UNKNOWN_RESULT;
							subMovesNotInBook = true;
						}
						ConnectFour_normal_undrop(cf);
					}
				}
			}
			else if (GAME_VARIANT == POPOUT_VARIANT) {
				for (i = 0; i < COLUMNS_X2; ++i) {
					if (i < COLUMNS) {
						if (ConnectFour_popout_drop(cf, i)) {
							if (!BookFile_popout_retrieve(readFile, cf, ConnectFour_getHashKey(cf), &rs[i], &movesHashResult, -i, true)) {
								rs[i] = UNKNOWN_RESULT;
								subMovesNotInBook = true;
							}
							ConnectFour_popout_undrop(cf);
						}
					}
					else {
						if (ConnectFour_popout_pop(cf, i)) {
							if (!BookFile_popout_retrieve(readFile, cf, ConnectFour_getHashKey(cf), &rs[i], &movesHashResult, -i, true)) {
								rs[i] = UNKNOWN_RESULT;
								subMovesNotInBook = true;
							}
							ConnectFour_popout_unpop(cf);
						}
					}
				}
			}

			for (i = 0; i < COLUMNS_X2; ++i) {
				Result_increment(&rs[i]);
			}

			if (hashResult == BOOKENTRY_MIRRORED) {
				ConnectFour_reverseBoard(cf);
				if (GAME_VARIANT == POPOUT_VARIANT) {
					Result_popout_reverse(rs);
				}
				Result_normal_reverse(rs);
			}

			/*switch (GAME_VARIANT) {
			case NORMAL_VARIANT:
				if (!BookFile_normal_retrieve(readFile, cf, hash, r, &hashResult)) {
					return false;
				}
				if (hashResult == BOOKENTRY_MIRRORED) {
					ConnectFour_reverseBoard(cf);
				}
				if (hashResult == BOOKENTRY_MIRRORED) {
					ConnectFour_reverseBoard(cf);
					Result_normal_reverse(rs);
				}
				break;
			case POPOUT_VARIANT:
				if (!BookFile_popout_retrieve(readFile, cf, hash, r, &hashResult, 0, false)) {
					return false;
				}
				if (hashResult == BOOKENTRY_MIRRORED) {
					ConnectFour_reverseBoard(cf);
				}
				for (i = 0; i < COLUMNS_X2; ++i) {
					if (i < COLUMNS) {
						if (ConnectFour_popout_drop(cf, i)) {
							if (!BookFile_popout_retrieve(readFile, cf, ConnectFour_getHashKey(cf), &rs[i], &movesHashResult, -i, true)) {
								rs[i] = UNKNOWN_RESULT;
							}
							ConnectFour_popout_undrop(cf);
						}
					}
					else {
						if (ConnectFour_popout_pop(cf, i)) {
							if (!BookFile_popout_retrieve(readFile, cf, ConnectFour_getHashKey(cf), &rs[i], &movesHashResult, -i, true)) {
								rs[i] = UNKNOWN_RESULT;
							}
							ConnectFour_popout_unpop(cf);
						}
					}
				}
				for (i = 0; i < COLUMNS_X2; ++i) {
					Result_increment(&rs[i]);
				}
				if (hashResult == BOOKENTRY_MIRRORED) {
					ConnectFour_reverseBoard(cf);
					Result_normal_reverse(rs);
					Result_popout_reverse(rs);
				}
				break;
			default:
				return false;
			}*/

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

bool BookFile_normal_retrieve(FILE *file, ConnectFour *cf, Position hash, Result *r, int *hashResult) {
	int8_t bookResult;
	if ((*hashResult = BookFile_hashLookup(file, cf, hash, false))) {
		if (*hashResult == BOOKENTRY_MIRRORED) {
			ConnectFour_reverseBoard(cf);
		}
		*r = (fread(&bookResult, sizeof(int8_t), 1, file) == 1) ? BookFile_normal_convertToResult(bookResult) : UNKNOWN_RESULT;
	}
	else {
		return false;
	}
	return true;
}

bool BookFile_popout_retrieve(FILE *file, ConnectFour *cf, Position hash, Result *r, int *hashResult, int mirrorIndex, bool columnSolve) {
	int8_t bookResult;
	if ((*hashResult = BookFile_hashLookup(file, cf, hash, false))) {
		if (*hashResult == BOOKENTRY_MIRRORED) {
			if (columnSolve) {
				*r = r[(int)(mirrorIndex + (mirrorIndex + COLUMNS_M1))];
				return true;
			}
		}
		*r = (fread(&bookResult, sizeof(int8_t), 1, file) == 1) ? BookFile_popout_convertToResult(bookResult) : UNKNOWN_RESULT;
	}
	else {
		return false;
	}
	return true;
}

int BookFile_hashLookup(FILE *file, ConnectFour *cf, Position targetHash, bool appendToBook) {
	Position bookHash = 0ull;
	unsigned r;
	assert(ftell(file) != -1);
	fseek(file, BOOKENTRY_OFFSET, SEEK_SET);
	for (r = 0u; r < bookFile.numRecords; ++r) {
		if (fread(&bookHash, hashBytes, 1, file) == 1) {
			if (targetHash == bookHash) {
				return BOOKENTRY_EXACT;
			}
			if (ConnectFour_reverse(targetHash) == bookHash) {
				return BOOKENTRY_MIRRORED;
			}
			if (fseek(file, 1, SEEK_CUR)) {
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

bool BookFile_generateBook(char *fileName, ConnectFour *cf, unsigned plies) {
	unsigned p;
	uint8_t bookEntry;
	Position hash = 0ull;
	Result result;
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
		if (!BookFile_hashLookup(generatorFile, cf, (hash = ConnectFour_getHashKey(cf)), false)) {
			fwrite(&hash, hashBytes, 1, generatorFile);
			switch (GAME_VARIANT) {
			case NORMAL_VARIANT:
				result = AlphaBeta_normal_solve(cf, false);
				bookEntry = BookFile_normal_convertToEntry(&result);
				fwrite(&bookEntry, sizeof(uint8_t), 1, generatorFile);
				break;
			case POPOUT_VARIANT:
				result = AlphaBeta_popout_solve(cf, false);
				bookEntry = BookFile_popout_convertToEntry(&result);
				fwrite(&bookEntry, sizeof(uint8_t), 1, generatorFile);
			}
			/*for (i = 0; i < COLUMNS; ++i) {
				switch (GAME_VARIANT) {
				case NORMAL_VARIANT:
					bookEntry = BookFile_normal_convertToEntry(&results[i]);
					fwrite(&bookEntry, sizeof(uint8_t), 1, generatorFile);
					//ConnectFour_normal_undrop(cf);
					break;
				case POPOUT_VARIANT:
					bookEntry = BookFile_popout_convertToEntry(&results[i]);
					fwrite(&bookEntry, sizeof(uint8_t), 1, generatorFile);
					//ConnectFour_popout_undrop(cf);
				}
			}
			if (GAME_VARIANT == POPOUT_VARIANT) {
				for (i = 0; i < COLUMNS; ++i) {
					if (ConnectFour_popout_pop(cf, i)) {
						bookEntry = BookFile_popout_convertToEntry(&results[i]);
						fwrite(&bookEntry, sizeof(uint8_t), 1, generatorFile);
						//onnectFour_popout_unpop(cf);
					}
				}
			}*/
			++bookFile.numRecords;
			++currentEntries;
			//printf(" - %s\n", BOOKFILE_APPENDED);
		}
		/*else {
			printf(" - %s\n", BOOKFILE_EXISTS);
		}*/
		switch (GAME_VARIANT) {
		case NORMAL_VARIANT:
			for (p = 1; p <= plies; ++p) {
				BookFile_normal_generateEntries(generatorFile, cf, p);
			}
			break;
		/*case POPOUT_VARIANT:
			BookFile_popout_generateEntries(generatorFile, cf, plies);*/
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

void BookFile_normal_generateEntries(FILE *file, ConnectFour *cf, unsigned plies) {
	if (cf->plyNumber < plies) {
		unsigned cStart, cEnd;
		Position hashEntry;
		uint8_t bookEntry;
		Result bookResult;
		cEnd = ConnectFour_symmetrical((hashEntry = ConnectFour_getHashKey(cf))) ? COLUMNS_D2 + (COLUMNS & 1u) : COLUMNS;
		for (cStart = 0u; cStart < cEnd; ++cStart) {
			if ((GAME_VARIANT == POPOUT_VARIANT) ? ConnectFour_popout_drop(cf, cStart) : ConnectFour_normal_drop(cf, cStart)) {
				if (!BookFile_hashLookup(file, cf, (hashEntry = ConnectFour_getHashKey(cf)), false)) {
					if (ConnectFour_connection(cf->board[!(cf->plyNumber & 1)]) || cf->plyNumber == AREA) {
						if (GAME_VARIANT == POPOUT_VARIANT) {
							ConnectFour_popout_undrop(cf);
						}
						else {
							ConnectFour_normal_undrop(cf);
						}
						continue;
					}
					else {
						TranspositionTable_dynamicReset(&table, tableSize);
						if (GAME_VARIANT == POPOUT_VARIANT) {
							bookResult = AlphaBeta_popout_solve(cf, false);
						}
						else {
							bookResult = AlphaBeta_normal_solve(cf, false);
						}
					}
					bookEntry = (GAME_VARIANT == POPOUT_VARIANT) ? BookFile_popout_convertToEntry(&bookResult) : BookFile_normal_convertToEntry(&bookResult);
					fwrite(&hashEntry, hashBytes, 1, file);
					fwrite(&bookEntry, sizeof(uint8_t), 1, file);
					++currentEntries;
					++bookFile.numRecords;
					/*ConnectFour_printMoves(cf);
					printf(" - %s\n", BOOKFILE_APPENDED);*/
					BookFile_normal_generateEntries(file, cf, plies);
				}
				else {
					/*ConnectFour_printMoves(cf);
					printf(" - %s\n", BOOKFILE_EXISTS);*/
					BookFile_normal_generateEntries(file, cf, plies);
				}
				if (GAME_VARIANT == POPOUT_VARIANT) {
					ConnectFour_popout_undrop(cf);
				}
				else {
					ConnectFour_normal_undrop(cf);
				}
			}
		}
	}
	else {
		return;
	}
			/*if ((GAME_VARIANT == POPOUT_VARIANT) ? ConnectFour_popout_drop(cf, i) : ConnectFour_normal_drop(cf, i)) {
				if (cf->moves[0] <= (COLUMNS >> 1u) - !(COLUMNS & 1u)) {
					//ConnectFour_printMoves(cf);
#ifdef __unix__
					fflush(stdout);
#endif
					if (!BookFile_hashLookup(file, cf, (hash = ConnectFour_getHashKey(cf)), false)) {
						if (ConnectFour_connection(cf->board[!(cf->plyNumber & 1)]) || cf->plyNumber == AREA) {
							//printf(" - %s\n", BOOKFILE_TERMINAL);
							if (GAME_VARIANT == POPOUT_VARIANT) {
								ConnectFour_popout_undrop(cf);
							}
							else {
								ConnectFour_normal_undrop(cf);
							}
							continue;
						}
						else {
							TranspositionTable_dynamicReset(&table, tableSize);
							if (GAME_VARIANT == POPOUT_VARIANT) {
								bookResult = AlphaBeta_popout_solve(cf, false);
							}
							else {
								bookResult = AlphaBeta_normal_solve(cf, false);
							}
						}
						bookEntry = (GAME_VARIANT == POPOUT_VARIANT) ? BookFile_popout_convertToEntry(&bookResult) : BookFile_normal_convertToEntry(&bookResult);
						fwrite(&hash, hashBytes, 1, file);
						fwrite(&bookEntry, sizeof(uint8_t), 1, file);
						++currentEntries;
						++bookFile.numRecords;
						//printf(" - %s\n", BOOKFILE_APPENDED);
						BookFile_normal_generateEntries(file, cf, plies);
					}
					else {
						printf(" - %s\n", BOOKFILE_EXISTS);
						BookFile_normal_generateEntries(file, cf, plies);
					}
				}*/



		/*
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
		}*/
}

void BookFile_popout_generateEntries(FILE *file, ConnectFour *cf, Result *bookResult, unsigned plies) {
	if (cf->plyNumber < plies) {
		unsigned i, j;
		Position hash;
		uint8_t bookEntry;
		BookFile_normal_generateEntries(file, cf, plies);
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
						AlphaBeta_popout_getMoveScores(cf, bookResult, NULL, false);
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

bool BookFile_storeToTranspositionTable(char *fileName, ConnectFour *cf, TranspositionTable *tt, const bool FIRST_TIME) {
	Position bookHash = 0ull;
	int8_t bookEntry = 0;
	unsigned i;
		if (BookFile_readFromDrive(fileName)) {
#ifdef _MSC_VER
		FILE *tableBookFile;
		errno_t errors = fopen_s(&tableBookFile, fileName, "rb");
#else
		FILE *tableBookFile = fopen(fileName, "rb");
#endif
		if (tableBookFile) {
			fseek(tableBookFile, BOOKENTRY_OFFSET, SEEK_SET);
			for (i = 0; i < bookFile.numRecords; ++i) {
				fread(&bookHash, hashBytes, 1, tableBookFile);
				fread(&bookEntry, sizeof(bookEntry), 1, tableBookFile);
				if (FIRST_TIME && TranspositionTable_isUnique(tt, bookHash)) {
					printf("%llX: duplicated entry\n", bookHash);
				}
				if (!TranspositionTable_BookFile_store(tt, bookHash, bookEntry)) {
					fprintf(stderr, "No table memory left for additional entries.");
					break;
				}

			}
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
		fclose(tableBookFile);
		}
		else {
			fprintf(stderr, "%s%s", BOOK_UNABLE_OPEN_FILE, BOOK_READING_STRING);
			return false;
		}
	}
	return true;
}

int BookFile_loadFromTranspositionTable(char *fileName, TranspositionTable *tt, Position hash, Result *tableResult) {
	//if (BookFile_readFromDrive(fileName)) {
		uint8_t loader; bool mirroredHash, loadStatus;
		if ((loadStatus = TranspositionTable_BookFile_load(tt, hash, &loader))) {
			mirroredHash = false;
		}
		else if ((loadStatus = TranspositionTable_BookFile_load(tt, ConnectFour_reverse(hash), &loader))) {
			mirroredHash = true;
		}
		if (loadStatus) {
			switch (GAME_VARIANT) {
			case NORMAL_VARIANT:
				*tableResult = BookFile_normal_convertToResult(loader);
				break;
			case POPOUT_VARIANT:
				*tableResult = BookFile_popout_convertToResult(loader);
				break;
			}
			return mirroredHash ? BOOKENTRY_MIRRORED : BOOKENTRY_EXACT;
		}
	//}
	return 0;
}

bool BookFile_loadSubMovesFromTranspositionTable(char *fileName, ConnectFour *cf, TranspositionTable *tt, Result *subResults) {
	if (BookFile_readFromDrive(fileName)) {
		unsigned i;

		if (GAME_VARIANT == NORMAL_VARIANT) {
			for (i = 0; i < COLUMNS; ++i) {
				subResults[i] = UNKNOWN_RESULT;
				if (ConnectFour_normal_drop(cf, i)) {
					if (!BookFile_loadFromTranspositionTable(fileName, tt, ConnectFour_getHashKey(cf), &subResults[i])) {
						ConnectFour_normal_undrop(cf);
						return false;
					}
					ConnectFour_normal_undrop(cf);
				}
			}
			for (i = 0; i < COLUMNS; ++i) {
				Result_increment(&subResults[i]);
			}
		}
		else if (GAME_VARIANT == POPOUT_VARIANT) {
			for (i = 0; i < COLUMNS_X2; ++i) {
				subResults[i] = UNKNOWN_RESULT;
				if (i < COLUMNS) {
					if (ConnectFour_popout_drop(cf, i)) {
						if (!BookFile_loadFromTranspositionTable(fileName, tt, ConnectFour_getHashKey(cf), &subResults[i])) {
							ConnectFour_popout_undrop(cf);
							return false;
						}
						ConnectFour_popout_undrop(cf);
					}
				}
				else {
					if (ConnectFour_popout_pop(cf, i - COLUMNS)) {
						if (!BookFile_loadFromTranspositionTable(fileName, tt, ConnectFour_getHashKey(cf), &subResults[i])) {
							ConnectFour_popout_unpop(cf);
							return false;
						}
						ConnectFour_popout_unpop(cf);
					}
				}
			}
			for (i = 0; i < COLUMNS_X2; ++i) {
				Result_increment(&subResults[i]);
			}
		}
		return true;
	}
	return false;
}

bool BookFile_checkVaildEntry(char *bookName, ConnectFour *cf, TranspositionTable *tt, unsigned plies) {
	for (unsigned r = 0u; r < plies; ++r) {
		switch (GAME_VARIANT) {
		case POPOUT_VARIANT:
			if (BookFile_popout_checkVaildEntry(bookName, cf, tt, plies)) {
				return false;
			}
			break;
		default:
			if (BookFile_normal_checkVaildEntry(bookName, cf, tt, plies)) {
				return false;
			}
		}
	}
	return true;
}

bool BookFile_normal_checkVaildEntry(char *bookName, ConnectFour *cf, TranspositionTable *tt, unsigned plies) {
	static bool dropsMissing = false;
	if (cf->plyNumber < plies) {
		unsigned cStart, cEnd;
		uint8_t bookEntry;
		Position bookHash;
		cEnd = ConnectFour_symmetrical((bookHash = ConnectFour_getHashKey(cf))) ? COLUMNS_D2 + (COLUMNS & 1u) : COLUMNS;
		for (cStart = 0; cStart < cEnd; ++cStart) {
			if ((GAME_VARIANT == POPOUT_VARIANT) ? ConnectFour_popout_drop(cf, cStart) : ConnectFour_normal_drop(cf, cStart)) {
				if (ConnectFour_connection(cf->board[!(cf->plyNumber & 1)]) || ((GAME_VARIANT == NORMAL_VARIANT && cf->plyNumber == AREA))) {
					if (GAME_VARIANT == POPOUT_VARIANT) {
						ConnectFour_popout_undrop(cf);
					}
					else {
						ConnectFour_normal_undrop(cf);
					}
					continue;
				}
				else {
					if (TranspositionTable_BookFile_load(tt, (bookHash = ConnectFour_getHashKey(cf)), &bookEntry) || TranspositionTable_BookFile_load(tt, ConnectFour_reverse(bookHash), &bookEntry)) {
						BookFile_normal_checkVaildEntry(bookName, cf, tt, plies);
					}
					else {
						ConnectFour_printMoves(cf);
						printf(": %llX: missing entry\n", bookHash);
						dropsMissing = true;
					}
				}
				if (GAME_VARIANT == POPOUT_VARIANT) {
					ConnectFour_popout_undrop(cf);
				}
				else {
					ConnectFour_normal_undrop(cf);
				}
			}
		}
	}
	return dropsMissing;
}

bool BookFile_popout_checkVaildEntry(char *bookName, ConnectFour *cf, TranspositionTable *tt, unsigned plies) {
	static bool popsMissing = false;
	if (BookFile_normal_checkVaildEntry(bookName, cf, tt, plies)) {
		if (cf->plyNumber < plies) {
			unsigned cStart, cEnd;
			uint8_t bookEntry;
			Position bookHash;
			cEnd = ConnectFour_symmetrical((bookHash = ConnectFour_getHashKey(cf))) ? COLUMNS_D2 + (COLUMNS & 1u) : COLUMNS;
			for (cStart = 0; cStart < cEnd; ++cStart) {
				if (ConnectFour_popout_pop(cf, cStart)) {
					if (ConnectFour_connectionNoVertical(cf->board[!(cf->plyNumber & 1u)]) || ConnectFour_connectionNoVertical(cf->board[cf->plyNumber & 1u])) {
						ConnectFour_popout_unpop(cf);
						continue;
					}
					else {
						if (TranspositionTable_BookFile_load(tt, (bookHash = ConnectFour_getHashKey(cf)), &bookEntry) || TranspositionTable_BookFile_load(tt, ConnectFour_reverse(bookHash), &bookEntry)) {
							BookFile_popout_checkVaildEntry(bookName, cf, tt, plies);
						}
						else {
							ConnectFour_printMoves(cf);
							printf(": %llX: missing entry\n", bookHash);
							popsMissing = true;
						}
					}
					ConnectFour_popout_unpop(cf);
				}
			}
		}
	}
	return popsMissing;
}
/*
bool BookFile_checkUnique(char *bookName, ConnectFour *cf, TranspositionTable *tt, int plies) {
	UniqueTable unique;
	unique.size = TranspositionTable_getPrimeClosestAndGreaterThanPowerOf2(bookFile.numRecords);
	unique.entry = calloc(1, sizeof(Position) * unique.size);
	switch (GAME_VARIANT) {
	case POPOUT_VARIANT:
		if (BookFile_popout_checkUnique(bookName, cf, tt, &unique, plies)) {
			return false;
		}
		break;
	default:
		if (BookFile_normal_checkUnique(bookName, cf, tt, &unique, plies)) {
			return false;
		}
	}
	free(unique.entry);
	return true;
}

bool BookFile_normal_checkUnique(char *bookName, ConnectFour *cf, TranspositionTable *tt, UniqueTable *unique, int plies) {
	static bool duplicateDrops = false;
	if (cf->plyNumber < plies) {
		long long uniqueIndex, nextUnique;
		unsigned cStart, cEnd;
		uint8_t bookEntry;
		Position bookHash;
		cEnd = ConnectFour_symmetrical((bookHash = ConnectFour_getHashKey(cf))) ? COLUMNS_D2 + (COLUMNS & 1u) : COLUMNS;
		for (cStart = 0; cStart < cEnd; ++cStart) {
			if ((GAME_VARIANT == POPOUT_VARIANT) ? ConnectFour_popout_drop(cf, cStart) : ConnectFour_normal_drop(cf, cStart)) {
				if (ConnectFour_connection(cf->board[!(cf->plyNumber & 1)]) || ((GAME_VARIANT == NORMAL_VARIANT && cf->plyNumber == AREA))) {
					if (GAME_VARIANT == POPOUT_VARIANT) {
						ConnectFour_popout_undrop(cf);
					}
					else {
						ConnectFour_normal_undrop(cf);
					}
					continue;
				}
				else {
					if (!unique->entry[(uniqueIndex = (bookHash = ConnectFour_getHashKey(cf)) % unique->size)]) {
						unique->entry[uniqueIndex] = bookHash;
						BookFile_normal_checkUnique(bookName, cf, tt, unique, plies);
					}
					else if (unique->entry[uniqueIndex] != bookHash) {
						for (nextUnique = uniqueIndex + 1ll; nextUnique != uniqueIndex;) {
							if (!unique->entry[nextUnique]) {
								unique->entry[nextUnique] = bookHash;
								BookFile_normal_checkUnique(bookName, cf, tt, unique, plies);
								break;
							}
							if (++nextUnique == unique->size) {
								nextUnique = 0;
							}
						}
					}
					else {
						ConnectFour_printMoves(cf);
						printf(": %llX: duplicate entry\n", bookHash);
						duplicateDrops = true;
					}
				}
				if (GAME_VARIANT == POPOUT_VARIANT) {
					ConnectFour_popout_undrop(cf);
				}
				else {
					ConnectFour_normal_undrop(cf);
				}
			}
		}
	}
	return duplicateDrops;
}

bool BookFile_popout_checkUnique(char *bookName, ConnectFour *cf, TranspositionTable *tt, UniqueTable *unique, int plies) {
	static bool duplicatePops = false;
	if (BookFile_normal_checkUnique(bookName, cf, tt, unique, plies)) {
		if (cf->plyNumber < plies) {
			long long uniqueIndex, nextUnique;
			unsigned cStart, cEnd;
			uint8_t bookEntry;
			Position bookHash;
			cEnd = ConnectFour_symmetrical((bookHash = ConnectFour_getHashKey(cf))) ? COLUMNS_D2 + (COLUMNS & 1u) : COLUMNS;
			for (cStart = 0; cStart < cEnd; ++cStart) {
				if (ConnectFour_popout_pop(cf, cStart)) {
					if (ConnectFour_connectionNoVertical(cf->board[!(cf->plyNumber & 1u)]) || ConnectFour_connectionNoVertical(cf->board[cf->plyNumber & 1u])) {
						ConnectFour_popout_unpop(cf);
						continue;
					}
					else {
						if (!unique->entry[(uniqueIndex = (bookHash = ConnectFour_getHashKey(cf)) % unique->size)]) {
							unique->entry[uniqueIndex] = bookHash;
							BookFile_popout_checkUnique(bookName, cf, tt, unique, plies);
						}
						else if (unique->entry[uniqueIndex] != bookHash) {
							for (nextUnique = uniqueIndex + 1ll; nextUnique != uniqueIndex;) {
								if (!unique->entry[nextUnique]) {
									unique->entry[nextUnique] = bookHash;
									BookFile_popout_checkUnique(bookName, cf, tt, unique, plies);
									break;
								}
								if (++nextUnique == unique->size) {
									nextUnique = 0;
								}
							}
						}
						else {
							ConnectFour_printMoves(cf);
							printf(": %llX: duplicate entry\n", bookHash);
							duplicatePops = true;
						}
					}
					ConnectFour_popout_unpop(cf);
				}
			}
		}
	}
	return duplicatePops;
}
*/
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

