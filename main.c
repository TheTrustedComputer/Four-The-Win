/*
	Copyright (C) 2019 TheTrustedComputer

	Four the Win! main entry point. It has multiple main functions for different target platforms.
*/

// Include required standard C libraries
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <float.h>
#include <math.h>
#include <limits.h>
#include <assert.h>
#include <wchar.h>
#include <errno.h>
#include <pthread.h>

// Include platform-dependent headers for Windows and Unix-based systems
#ifdef _WIN32
#include <windows.h>
#include <commctrl.h>
#elif __unix__
#include <sys/resource.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <signal.h>
#endif

// Include Four the Win! dependency sources
#include "mt19937-64.h"
#include "largeboard.c"
#include "connectfour.c"
#include "transpositiontable.c"
#include "alphabeta.c"
#include "book.c"

/** GUI Win32 main entry point **/
#ifdef _WIN32
/*
#include "resource.h"
#include "windowgui.c"

INT WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ INT nShowCmd) {
	WNDCLASSW windowClass;
	MSG message;
	DWORD errorCode;
	INITCOMMONCONTROLSEX controls;
	WCHAR handleError[70], windowTitle[] = L"Four the Win!";
	controls.dwSize = sizeof(INITCOMMONCONTROLSEX);
	controls.dwICC = ICC_STANDARD_CLASSES | ICC_BAR_CLASSES;
	InitCommonControlsEx(&controls);
	SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);
	if (WindowGUI_initialize(NORMAL_VARIANT, windowTitle)) {
		if ((mainHandle = WindowGUI_createWindow(&windowClass, hInstance, windowTitle))) {
			ShowWindow(mainHandle, nShowCmd);
			for (message = (MSG){ 0 }; GetMessage(&message, NULL, 0, 0);) {
				DispatchMessageW(&message);
				TranslateMessage(&message);
			}
			return (INT)message.lParam;
		}
		else {
			errorCode = GetLastError();
			swprintf_s(handleError, 64, L"Unable to show the window handle. The last error code is %d.", errorCode);
			MessageBoxW(NULL, handleError, windowTitle, MB_ICONERROR);
			return errorCode;
		}
	}
	return EXIT_FAILURE;
}
*/
#endif

// Common string macros
#define DRAWN_OR_WON_POSITION "Terminal position (drawn or won)."
#define POSSIBLE_DRAW "\e[1mPossible DRAW by repetition\e[0m (%d)\n"

// Function to handle the interrupt signal
void SignalTrapper(int Q) {
	printf("Interrupt.\n");
	exit(0);
}

// The search runs on a different thread so that the main thread can still respond to input
void *SolverThread_solve(void *SolverThread_args) {
	//printf("Thread created.");
	return NULL;
}

/** Console main entry point **/
int main(int argc, char **argv) {
	// Statistical information whenever a solution is found
	clock_t stopwatch; // A stopwatch to count how many seconds solving this position
	double npsec, sec; // Nodes or positions per seconds; time in seconds spent solving

	// An opening book to remember solutions afterward
	bool startMainFlag, startPopTenFlag, hasUndone, hasRedone, solvingFlag, bestFlag, useBookFlag, generateBookFlag, storingEntriesToTable;
	unsigned m, bestMove, variant;
	char input, bookFileName[22], *argSequence;
	int bookEntryStatus, bookSubMovesEntryStatus, argBookDepth, argTableSize, redoBufferPointer;
	unsigned short oldPlayedPowerCheckers;
	uint8_t *popTenFlagRedoBuffer;
	// The Connect Four data structure and the redo buffer
	ConnectFour connectFour, *redoBuffer;

	// Solution results; assume the board position is the same after solving and error out if not
	Position oldBoard[2], oldPowerCheckers[4], oldHash;
	Result result, *results;

	// A separate thread that does the actual solving -- main thread handles input exclusively
	pthread_t SolverThread;

	/* Start of main function begins here */
	solvingFlag = startPopTenFlag = storingEntriesToTable = true;
	bestFlag = useBookFlag = generateBookFlag = hasUndone = hasRedone = false;
	results = NULL;
	moveOrder = NULL;
	pv = NULL;
	redoBuffer = NULL;
	popTenFlagRedoBuffer = NULL;
	subMovesNotInBook = true;
	redoBufferPointer = bookEntryStatus = bookSubMovesEntryStatus = argBookDepth = 0;

	// Trap the SIGINT signal whenever the user hits Control-C
	signal(SIGINT, SignalTrapper);

	// Read command-line arguments and change program behavior accordingly
	{
		// Default valuse
		int argColumns, argRows, argVariant;
		bool sizeNotDone = true, variantNotDone = true, bestNotDone = true, bookNotDone = true, generateNotDone = true, tableNotDone = true;
		argColumns = 7;
		argRows = 6;
		argVariant = NORMAL_VARIANT;
		argTableSize = 1;

		// Allocate a buffer to copy move parameters from the program
		if (!(argSequence = calloc(1, sizeof(argSequence) * MOVESIZE))) {
			fprintf(stderr, "Error loading moves from command-line arguments.\n");
			exit(EXIT_FAILURE);
		}

		// argv[0] is always the name of the program itself -- the rest after the whitespace are passed as arguments
		if (argc >= 2) {
			int switches; size_t c;
			for (switches = 1; switches < argc; ++switches) { // Display help message
				if (argv[switches][0] == '-') { // Search for command-line switches
					if (!(strcmp(argv[switches], "-h") && strcmp(argv[switches], "-?") && strcmp(argv[switches], "--help"))) { // strcmp returns zero on string equality
						ConnectFour_displayHelpMessage(argv[0]);
						return 0;
					}
					else {
						int switchesPlusOne = switches + 1; size_t argSizeLength;
						if (sizeNotDone && !(strcmp(argv[switches], "-s") && strcmp(argv[switches], "--size"))) { // Read rows and columns -- needs an extra parameter to set size
							char charArgColumns[3] = "", charArgRows[3] = "";
							bool readColumn = true; int readBytes = 0, intColumns, intRows;
							if (switchesPlusOne < argc && (argSizeLength = strlen(argv[switchesPlusOne])) >= 3) { // Tokenize characters -- an X separates rows and coluns
								for (c = 0; c < argSizeLength; ++c) {
									if (argv[switchesPlusOne][c] == 'x' || argv[switchesPlusOne][c] == 'X') {
										readColumn = false;
										readBytes = 0;
										continue;
									}
									if (readBytes < 3) {
										readColumn ? (charArgColumns[readBytes++] = argv[switchesPlusOne][c]) : (charArgRows[readBytes++] = argv[switchesPlusOne][c]);
									}
								}
								if (((intColumns = atoi(charArgColumns)) && (intRows = atoi(charArgRows)))) {
									if (((intColumns >= 4) && (intRows >= 4)) && ((intColumns <= 16) && (intRows <= 16)) && (intColumns * (intRows + 1) <= 64)) {
										argColumns = intColumns;
										argRows = intRows;
									}
								}
							sizeNotDone = false;
							++switches;
							}
						}
						else if (variantNotDone && !(strcmp(argv[switches], "-v") && strcmp(argv[switches], "--variant"))) { // Read variant
							char possibleVariants[5][11] = {"normal", "popout","powerup", "popten", "fiveinarow"};
							if (switchesPlusOne < argc) {
								for (c = 0; c < 5; ++c) {
									if (!strcmp(argv[switchesPlusOne], possibleVariants[c])) {
										argVariant = c;
										break;
									}
								}
								variantNotDone = false;
								++switches;
							}
						}
						else if (bestNotDone && !(strcmp(argv[switches], "-B") && strcmp(argv[switches], "--best"))) { // Read best move option
							bestFlag = true;
							useBookFlag = false;
							bestNotDone = false;
						}
						else if (bookNotDone && !(strcmp(argv[switches], "-b") && strcmp(argv[switches], "--book"))) { // Read opening book file
							useBookFlag = true;
							bookNotDone = false;
						}
						else if (generateNotDone && !(strcmp(argv[switches], "-g") && strcmp(argv[switches], "--generate"))) { // Read opening book generation
							if (switchesPlusOne < argc) {
								argBookDepth = atoi(argv[switchesPlusOne]);
								useBookFlag = generateBookFlag = true;
								bookNotDone = generateNotDone = false;
								++switches;
							}
						}
						else if (tableNotDone && !(strcmp(argv[switches], "-t") && strcmp(argv[switches], "--table"))) { // Read transposition table size
							if (switchesPlusOne < argc) {
								if ((argTableSize = atoi(argv[switchesPlusOne])) <= 0) {
									argTableSize = 1;
								}
								tableNotDone = false;
								++switches;
							}
						}
						else {
							fprintf(stderr, "Could not understand what the switch '%s' meant. Type '%s --help' for basic help.\n", argv[switches], argv[0]);
							return 1;
						}
					}
				}
				// Moves input
				else if (((argv[switches][0] >= '1') && (argv[switches][0] <= '9')) || ((argv[switches][0] >= 'A') && (argv[switches][0] <= 'Z')) || ((argv[switches][0] >= 'a') && (argv[switches][0] <= 'z'))) {
					strncpy(argSequence, argv[switches], strlen(argv[switches])); // Copy move input
				}
			}
		}

		// The default variant is normal Connect Four unless given command-line switches
		variant = argVariant;

		// Initialize the ConnectFour structure and therefore the game itself
		ConnectFour_setSizeAndVariant(argColumns, argRows, variant);
		ConnectFour_setupBitmaps();
		ConnectFour_initialize(&connectFour);

		if (tableNotDone) {
			{
#ifdef _WIN32 // Microsoft Windows
				unsigned long long totalGigabytes, halfGigabytes;

				if (GetPhysicallyInstalledSystemMemory(&totalGigabytes)) { // In kilobytes. Must divide twice to get megabytes and then gigabytes.
					totalGigabytes /= 1048576;
					halfGigabytes = totalGigabytes /= 2;
				}
				else {
					fprintf(stderr, "WARNING: Could not determine system memory size; fallback to one gigabyte.\n");
					halfGigabytes = 1ull;
				}
				for (table.entries = NULL; halfGigabytes && !table.entries;) {
					if (!TranspositionTable_initialize(&table, (tableSize = TT_TABLESIZE * halfGigabytes))) {
						halfGigabytes /= 2ull;
					}
				}

#elif defined(__unix__) // Unix and Unix-like (Linux, BSD, Mac)
				struct sysinfo systemInformation;
				unsigned long totalGigabytes, halfGigabytes;

				// Get hardware information about this computer.
				if (sysinfo(&systemInformation) != -1) {
					totalGigabytes = (unsigned long)ceil(systemInformation.totalram / 1073741824.0);
					halfGigabytes = totalGigabytes / 2;
				}
				else {
					fprintf(stderr, "WARNING: Could not determine system memory size; fallback to one gigabyte.\n");
					halfGigabytes = 1ul;
				}
				for (table.entries = NULL; halfGigabytes && !table.entries;) {
					if (!TranspositionTable_initialize(&table, (tableSize = TT_TABLESIZE * halfGigabytes))) {
						halfGigabytes /= 2ul;
					}
				}
#else
				TranspositionTable_initialize(&table, (tableSize = TT_TABLESIZE));
#endif
			}
		}
		else { // Use a transposition table size from command-line arguments or an automatic one
			(GAME_VARIANT == POWERUP_VARIANT) ?  TranspositionTable_initialize(&table, (tableSize = argTableSize * TT_TABLESIZE / 3)) : TranspositionTable_initialize(&table, (tableSize = argTableSize * TT_TABLESIZE));
		}
	}

	// Lower the process priority to make the operating system more responsive to other tasks
#ifdef _WIN32 // Microsoft Windows API
	SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);
#elif __unix__ // Unix and Linux POSIX
	setpriority(PRIO_PROCESS, getpid(), 10);
#endif

	// Compute the number of minimal bytes used by the encoded Connect Four position--for opening book generation
	hashBytes = ((size_t)COLUMNS * ROWS_P1 / sizeof(Position) + ((size_t)COLUMNS * ROWS_P1 % sizeof(Position) > 0ull));

	// Obtain the file name of the opening book depending on the compiler
	switch (GAME_VARIANT) { // Retrieve the game variant described in GAME_VARIANT and attach that to the book file
	case NORMAL_VARIANT:
#ifdef _MSC_VER  // Microsoft Visual Studio specific safe code
		strcpy_s(bookFileName, sizeof(BOOKFILE_NORMAL), BOOKFILE_NORMAL);
#else	// The rest of C compilers have this vanilla function
		strcpy(bookFileName, BOOKFILE_NORMAL);
#endif
		break;
	case POPOUT_VARIANT:
#ifdef _MSC_VER
		strcpy_s(bookFileName, sizeof(BOOKFILE_POPOUT), BOOKFILE_POPOUT);
#else
		strcpy(bookFileName, BOOKFILE_POPOUT);
#endif
		break;
	case POWERUP_VARIANT:
#ifdef _MSC_VER
		strcpy_s(bookFileName, sizeof(BOOKFILE_POWERUP), BOOKFILE_POWERUP);
#else
		strcpy(bookFileName, BOOKFILE_POWERUP);
#endif
		break;
	case FIVEINAROW_VARIANT:
#ifdef _MSC_VER
		strcpy_s(bookFileName, sizeof(BOOKFILE_FIVEINAROW), BOOKFILE_FIVEINAROW);
#else
		strcpy(bookFileName, BOOKFILE_POPTEN);
#endif
	}
	{	// Fetch the board size and append it to the book file name.
		char columnsString[3], rowsString[3], boardSizeString[5];
		snprintf(columnsString, 3, "%u", COLUMNS);
		snprintf(rowsString, 3, "%u", ROWS);
#ifdef _MSC_VER // Microsoft Visual Studio
		strcpy_s(boardSizeString, sizeof(boardSizeString), columnsString);
		strcat_s(boardSizeString, sizeof(boardSizeString), "x");
		strcat_s(boardSizeString, sizeof(boardSizeString), rowsString);
		strcat_s(bookFileName, sizeof(bookFileName), boardSizeString);
		strcat_s(bookFileName, sizeof(bookFileName), BOOKFILE_FTWB_EXTENSION);
#else	// GCC, Clang, and other C compilers
		strcpy(boardSizeString, columnsString);
		strcat(boardSizeString, "x");
		strcat(boardSizeString, rowsString);
		strcat(bookFileName, boardSizeString);
		strcat(bookFileName, BOOKFILE_FTWB_EXTENSION);
#endif
	}

	// Check if memory allocations were successful
	if (!table.entries) {
		fprintf(stderr, "Error allocate memory for the hashtable entry data.\n");
		exit(EXIT_FAILURE);
	}
	if (!(results = malloc(sizeof(Result) * ConnectFour_getMoveSize()))) {
		fprintf(stderr, "Error allocating memory for the game solution results.\n");
		exit(EXIT_FAILURE);
	}
	if (!(moveOrder = malloc(sizeof(int) * COLUMNS))) {
		fprintf(stderr, "Error allocating memory for the alpha-beta move order.\n");
		exit(EXIT_FAILURE);
	}
	if (!(pv = malloc(sizeof(char) * MOVESIZE))) {
		fprintf(stderr, "Error allocating memory for the principal variation.\n");
		exit(EXIT_FAILURE);
	}
	if (GAME_VARIANT == POPTEN_VARIANT) {
		if (!(popTenFlagRedoBuffer = malloc(sizeof(ConnectFour) * (MOVESIZE + 1u)))) {
			fprintf(stderr, "Error allocating memory for Pop Ten flags undo and redo buffer.\n");
			exit(EXIT_FAILURE);
		}
		if (!(redoBuffer = malloc(sizeof(ConnectFour) * MOVESIZE))) {
			fprintf(stderr, "Error allocating memory for the Connect Four undo and redo buffer.\n");
			exit(EXIT_FAILURE);
		}
		else {
			for (unsigned b = 0u; b < MOVESIZE; ++b) {
				ConnectFour_initialize(&redoBuffer[b]);
			}
		}
	}


#ifdef __unix__
	// Create the solver thread
	if (pthread_create(&SolverThread, NULL, SolverThread_solve, NULL)) {
		fprintf(stderr, "Unable to create a new thread for solving.");
		return 1;
	}
#endif

	// Prepare for alpha-beta pruning static move ordering
	AlphaBeta_getColumnMoveOrder();

	// Print basic information on the console or terminal
	puts("Four the Win! by TheTrustedComputer");
	printf("Using board size %dx%d under ", COLUMNS, ROWS);

	switch (GAME_VARIANT) {
	case NORMAL_VARIANT:
		printf("Normal");
		break;
	case POPOUT_VARIANT:
		printf("PopOut");
		break;
	case POWERUP_VARIANT:
		printf("Power Up");
		break;
	case POPTEN_VARIANT:
		printf("Pop Ten");
		break;
	case FIVEINAROW_VARIANT:
		printf("Five-In-A-Row");
		break;
	default:
		printf("Unknown");
	}

	printf(" ruleset\nHash table of %llu entries\n", table.size);

	if (GAME_VARIANT == POPTEN_VARIANT) {
		printf("Position I.D. is %llu\nType '?' to solve and '<' to undo\n", ConnectFour_getHashKey(&connectFour));
		solvingFlag = false;
	}

	// Information for debugging purposes
	//printf("Address of Connect Four board: 0x%llX\n", (unsigned long long)&connectFour.board[0]);

	// Read and process the opening book if it exists
	if (useBookFlag && GAME_VARIANT != POPTEN_VARIANT && !BookFile_readFromDrive(bookFileName)) {
		if (!BookFile_create(bookFileName)) {
			fprintf(stderr, "Opening book creation unsucceesful.\n");
		}

	}

	if (useBookFlag && generateBookFlag) {
		if (argBookDepth) {
			printf("Generating a %u-ply opening book from the starting position...\n", argBookDepth);
			BookFile_generateBook(bookFileName, &connectFour, argBookDepth);
			return 0;
		}
		else {
			puts("Cannot generate the opening book because a zero-ply length was given.");
		}
	}

	if (pv) {
		for (m = 0; m < MOVESIZE; ++m) {
			pv[m] = '\xff';
		}
	}

	// An infinite loop that constantly keep asking for input until closed by the user
	for (startMainFlag = true; startMainFlag;) {
		if (useBookFlag && !bestFlag) {
			// Add book entries to the transposition table
			if (!BookFile_storeToTranspositionTable(bookFileName, &connectFour, &table, storingEntriesToTable)) {
				fprintf(stderr, "Could not open the book file for storing entries to the transposition table.\n");
			}
			storingEntriesToTable = false;
		}
		if (argc >= 2 && argSequence[0] != '\0') {
			if (!ConnectFour_sequence(&connectFour, argSequence)) {
				fprintf(stderr, "Move sequence contains illegal moves.\n");
				break;
			}
			if (ConnectFour_gameOver(&connectFour)) {
				puts(DRAWN_OR_WON_POSITION);
				break;
			}
			startMainFlag = false;
		}
		else {
			if (startPopTenFlag && GAME_VARIANT == POPTEN_VARIANT) {
				ConnectFour_printBoard(&connectFour);
				ConnectFour_popten_copy(&connectFour, &redoBuffer[0]);
				popTenFlagRedoBuffer[0] = popTenFlags;
				startPopTenFlag = false;
				goto printPossibleMoves;
			}
			while ((input = getchar()) != EOF) {
				if (ConnectFour_gameOver(&connectFour)) {
					puts(DRAWN_OR_WON_POSITION);
					(GAME_VARIANT == POPTEN_VARIANT) ? ConnectFour_popten_reset(&connectFour, true) : ConnectFour_reset(&connectFour, false);
					while (input != '\n') {
						input = getchar();
					}
					continue;
				}
				if (!ConnectFour_play(&connectFour, input)) {
					if (GAME_VARIANT == POPTEN_VARIANT) {
						if (input == '?') {
							solvingFlag = !solvingFlag;
							while (input != '\n') {
								input = getchar();
							}
							break;
						}
						if (input == '<') {
							if (redoBufferPointer) {
								ConnectFour_popten_copy(&redoBuffer[--redoBufferPointer], &connectFour);
								popTenFlags = popTenFlagRedoBuffer[redoBufferPointer];
								hasUndone = true;
								hasRedone = false;
							}
							continue;
						}
					}
					if (input == '\n') {
						break;
					}
				}
				else {
					if (GAME_VARIANT == POPTEN_VARIANT && (redoBufferPointer + 1) < (int)(MOVESIZE)) {
						ConnectFour_popten_copy(&connectFour, &redoBuffer[++redoBufferPointer]);
						popTenFlagRedoBuffer[redoBufferPointer] = popTenFlags;
					}
				}
			}
		}

		// DEBUG: Keep old boards and test if they are the same after solving
		oldHash = ConnectFour_getHashKey(&connectFour);
		oldBoard[0] = connectFour.board[0];
		oldBoard[1] = connectFour.board[1];
		if (GAME_VARIANT == POWERUP_VARIANT) {
			oldPowerCheckers[0] = connectFour.pc->anvil;
			oldPowerCheckers[1] = connectFour.pc->bomb;
			oldPowerCheckers[2] = connectFour.pc->wall;
			oldPowerCheckers[3] = connectFour.pc->x2;
			oldPlayedPowerCheckers = connectFour.playedPowerCheckers;
		}

		ConnectFour_printBoard(&connectFour);

		/*if (GAME_VARIANT == POWERUP_VARIANT) {

			continue;
		}*/


		if (GAME_VARIANT == POPOUT_VARIANT) {
			ConnectFour_clearHistory(&connectFour);
		}
#ifdef HASH_TEST
		printf("%llX\n", ConnectFour_getHashKey(&connectFour));
		ConnectFour_reset(&connectFour, GAME_VARIANT == POPTEN_VARIANT);
		continue;
#endif
		if (bestFlag) {
			int scoretest = DRAW, possibleFlag = 1, currbestMove = 0;
			unsigned deepPly = MOVESIZE - connectFour.plyNumber;
			pv[0] = moveOrder[0] + '1';
			nodes = 0;
			repetitionFlag = 1;
			if (!solvingFlag) {
				goto printPossibleMoves;
			}
			stopwatch = clock();
			for (m = 0; m < deepPly; ++m) {
				switch (GAME_VARIANT) {
				case NORMAL_VARIANT:
					if ((scoretest = AlphaBeta_negamax_normal_withMoves(&connectFour, 0, m, -PLAYER_WIN, PLAYER_WIN)) >= PLAYER_WIN) {
						goto bestMoveFound;
					}
					break;
				case POPOUT_VARIANT:
					if (abs((scoretest = AlphaBeta_negamax_popout_withMoves(&connectFour, 0, m, -PLAYER_WIN, PLAYER_WIN))) >= PLAYER_WIN) {
						goto bestMoveFound;
					}
					else if (possibleFlag && abs(scoretest) == DRAW_WIN) {
						printf(POSSIBLE_DRAW, m);
						possibleFlag = 0;
					}
					break;
				case POPTEN_VARIANT:
					if (abs(scoretest = AlphaBeta_negamax_popten_withMoves(&connectFour, 0, m, -PLAYER_WIN, PLAYER_WIN)) >= PLAYER_WIN || !scoretest) {
						goto bestMoveFound;
					}
				}
				if (currbestMove != pv[0]) {
					repetitionFlag = 1;
				}
				printf("\rCurrent best: %c (%d)  \r", (currbestMove = pv[0]), m);
#ifdef __unix__
				fflush(stdout);
#endif
			}
			bestMoveFound:
			stopwatch = clock() - stopwatch;
			sec = (double)stopwatch / CLOCKS_PER_SEC;
			npsec = (double)nodes / sec;
			printf("\a");
			if (abs(scoretest) == IN_PROGRESS) {
				puts("Search exhausted; assuming draw.");
				scoretest = 0;
			}
			switch (GAME_VARIANT) {
			case POPOUT_VARIANT:
				if (scoretest == PLAYER_WIN) {
					printf("\e[1;32m%s\e[0m ", WIN_TEXT);
				}
				else if (scoretest == -PLAYER_WIN) {
					printf("\e[1;31m%s\e[0m ", LOSS_TEXT);
				}
				else {
					printf("\e[1;33m%s\e[0m ", DRAW_TEXT);
				}
				break;
			default:
				if (scoretest > DRAW) {
					printf("\e[1;32m%s\e[0m ", WIN_TEXT);
				}
				else if (scoretest < DRAW) {
					printf("\e[1;31m%s\e[0m ", LOSS_TEXT);
				}
				else {
					printf("\e[1;33m%s\e[0m ", DRAW_TEXT);
				}
			}
			printf("%c %llu %.0f %.3f (%d)\n", pv[0], nodes, npsec, sec, m);
			if (GAME_VARIANT != POPTEN_VARIANT) {
				ConnectFour_reset(&connectFour, false);
			}
			TranspositionTable_dynamicReset(&table, tableSize);
			continue;
		}
		if (!solvingFlag) {
			printPossibleMoves:
			switch (GAME_VARIANT) {
			case NORMAL_VARIANT:
			case FIVEINAROW_VARIANT:
				for (m = 0; m < COLUMNS; ++m) {
					if (ConnectFour_normal_drop(&connectFour, m)) {
						printf("%c ", m < 10 ? m + '1' : m + 'A');
						ConnectFour_normal_undrop(&connectFour);
					}
				}
				break;
			case POPOUT_VARIANT:
				for (m = 0; m < COLUMNS; ++m) {
					if (ConnectFour_popout_drop(&connectFour, m)) {
						printf("%c ", m < 10 ? m + '1' : m + 'A');
						ConnectFour_popout_undrop(&connectFour);
					}
				}
				for (m = 0; m < COLUMNS; ++m) {
					if (ConnectFour_popout_pop(&connectFour, m)) {
						printf("%c ", m < 10 ? m + 'A' : m + 'a');
						ConnectFour_popout_unpop(&connectFour);
					}
				}
				break;
			case POPTEN_VARIANT:
				for (m = 0; m < COLUMNS; ++m) {
					if (ConnectFour_popten_drop(&connectFour, m)) {
						printf("%c ", m < 10 ? m + '1' : m + 'A');
						ConnectFour_popten_undrop(&connectFour);
					}
				}
				for (m = 0; m < COLUMNS; ++m) {
					if (ConnectFour_popten_pop(&connectFour, m)) {
						printf("%c ", m < 10 ? m + 'A' : m + 'a');
						ConnectFour_popten_unpop(&connectFour);
					}
				}
				if (ConnectFour_popten_pass(&connectFour)) {
					printf("pass");
					ConnectFour_popten_unpass(&connectFour);
				}
			}
			puts("");
			continue;
		}
		pthread_join(SolverThread, NULL);
		stopwatch = clock();
		if (useBookFlag) {
			if (!(bookEntryStatus = BookFile_loadFromTranspositionTable(bookFileName, &table, ConnectFour_getHashKey(&connectFour), &result))) {
				TranspositionTable_dynamicReset(&table, tableSize);
				goto solve;
			}
		}
		else {
			solve:
			switch (GAME_VARIANT) {
			case NORMAL_VARIANT:
				result = AlphaBeta_normal_solve(&connectFour, true);
				break;
			case POPOUT_VARIANT:
				result = AlphaBeta_popout_solve(&connectFour, true);
				break;
			case POWERUP_VARIANT:
				result = AlphaBeta_powerup_solve(&connectFour, true);
				break;
			case POPTEN_VARIANT:
				result = AlphaBeta_popten_solve(&connectFour, true);
				break;
			case FIVEINAROW_VARIANT:
				return 0;
				break;
			}
		}
		stopwatch = clock() - stopwatch;
		sec = (double)stopwatch / CLOCKS_PER_SEC;
		npsec = (double)nodes / (sec ? sec : sec + 1.0);
		bestMove = -1;
		printf("\a");

		// Error out if the two boards are different after solving this position
		assert(oldHash == ConnectFour_getHashKey(&connectFour) && oldBoard[0] == connectFour.board[0] && oldBoard[1] == connectFour.board[1]);
		if (GAME_VARIANT == POWERUP_VARIANT) {
			assert(oldPowerCheckers[0] == connectFour.pc->anvil && oldPowerCheckers[1] == connectFour.pc->bomb && oldPowerCheckers[2] == connectFour.pc->wall && oldPowerCheckers[3] == connectFour.pc->x2 && oldPlayedPowerCheckers == connectFour.playedPowerCheckers);
		}

		// No erroneous results depending on the game
		if (GAME_VARIANT != POPTEN_VARIANT && GAME_VARIANT != POWERUP_VARIANT) {
			assert((result.wdl == WIN_CHAR) && !(result.dtc & 1) || (result.wdl == LOSS_CHAR) && (result.dtc & 1) || (result.wdl == DRAW_CHAR) || (result.wdl == UNKNOWN_CHAR));
		}
		if (result.wdl == DRAW_CHAR) {
			printf("\e[1;33m%s\e[0m ", DRAW_TEXT);
		}
		else {
			Result_print(&result, &result);
		}
		if (useBookFlag) {
			if (bookEntryStatus) {
				puts("book");
			}
			else {
				goto stats;
			}
		}
		else {
			stats:
			printf("%llu %.0f %.3f\n", nodes, npsec, sec);
			TranspositionTable_dynamicReset(&table, tableSize);
		}
		if (!startMainFlag) {
			break;
		}
		if (useBookFlag) {
			if ((bookSubMovesEntryStatus = BookFile_loadSubMovesFromTranspositionTable(bookFileName, &connectFour, &table, results))) {
				for (m = 0; m < COLUMNS; ++m) {
					Result_print(&results[m], &result);
				}
				if (GAME_VARIANT == POPOUT_VARIANT) {
					puts("");
					for (; m < COLUMNS_X2; ++m) {
						Result_print(&results[m], &result);
					}
				}
			}
			else {
				goto best;
			}
		}
		else {
			best:
			switch (GAME_VARIANT) {
			case NORMAL_VARIANT:
				bestMove = AlphaBeta_normal_getBestMove(&connectFour, results, &result, true);
				break;
			case POPOUT_VARIANT:
				bestMove = AlphaBeta_popout_getBestMove(&connectFour, results, &result, true);
				break;
			case POWERUP_VARIANT:
				bestMove = AlphaBeta_powerup_getBestMove(&connectFour, results, &result, true);
				break;
			case POPTEN_VARIANT:
				bestMove = AlphaBeta_popten_getBestMove(&connectFour, results, &result, true);
				break;
			case FIVEINAROW_VARIANT:
				return 0;
				//bestMove =  AlphaBeta_fiveinarow_getBestMove(&connectFour, results, &result, true);
				break;
			}
		}
		if (GAME_VARIANT == POPTEN_VARIANT && popTenFlags == POPTEN_DROP && !(connectFour.board[connectFour.plyNumber & 1u] & BOT)) {
			puts("\n\aBest: pass");
		}
		else {
			bestMove = Result_getBestMove(results, ConnectFour_getMoveSize());
			puts("");
			printf("\aBest: %c\n", (bestMove < COLUMNS) ? bestMove + '1' : bestMove + 'A' - COLUMNS);
		}
		if (GAME_VARIANT != POWERUP_VARIANT) {
			unsigned moveSize = ConnectFour_getMoveSize();
			for (m = 0; m < moveSize; ++m) {
				assert(results[m].wdl == UNKNOWN_CHAR || results[m].wdl == WIN_CHAR || results[m].wdl == DRAW_CHAR || results[m].wdl == LOSS_CHAR);
				switch (result.wdl) {
				case WIN_CHAR:
					assert(results[m].wdl == UNKNOWN_CHAR || (results[m].wdl == WIN_CHAR && results[m].dtc >= result.dtc) || results[m].wdl == DRAW_CHAR || results[m].wdl == LOSS_CHAR);
					break;
				case DRAW_CHAR:
					assert(results[m].wdl == UNKNOWN_CHAR || results[m].wdl == DRAW_CHAR || results[m].wdl == LOSS_CHAR);
					break;
				case LOSS_CHAR:
					assert(results[m].wdl == UNKNOWN_CHAR || (results[m].wdl == LOSS_CHAR && results[m].dtc <= result.dtc));
				}
			}
		}
		if (!(bookEntryStatus && bookSubMovesEntryStatus)) {
			TranspositionTable_dynamicReset(&table, tableSize);
		}
		if (solvingFlag) {
			if (GAME_VARIANT != POPTEN_VARIANT) {
				ConnectFour_reset(&connectFour, false);
			}
		}
		nodes = 0ull;
	}
	ConnectFour_destroy(&connectFour);
	TranspositionTable_destroy(&table);
	free(results);
	free(moveOrder);
	free(argSequence);
	free(redoBuffer);
	return EXIT_SUCCESS;
}
