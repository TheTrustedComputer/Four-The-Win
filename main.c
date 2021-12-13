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

// Include platform-dependent headers for Windows and Unix-based systems
#ifdef _WIN32
#include <windows.h>
#include <commctrl.h>
#elif __unix__
#include <sys/resource.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#endif

// Include mandatory Four the Win! sources
#include "mt19937-64.h"
#include "largeboard.c"
#include "connectfour.c"
#include "transpositiontable.c"
#include "alphabeta.c"
#include "book.c"

#define CF_PRINTBOARD system("cls"); ConnectFour_printBoard(cf);

/** Win32 main entry point **/
#ifdef _WIN32

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

#endif

#ifdef _MSC_VER
// #define USE_FIXED_TTSIZE
#define SCORE_TEST
#endif

/** Console main entry point **/
int main(int argc, char **argv) {
	// Statistical infomation whenever a solution is found
	clock_t stopwatch;
	double npsec, sec;

	// An opening book to remember solutions afterward
	unsigned i, j, best, variant, bookPly;
	char input, bookFileName[22];
	bool notInBook, solvingFlag;

	// The Connect Four data structure and solution results
	ConnectFour cf;
	Position hash, oldBoard[2], oldPowerCheckers[4];
	Result result, *results;

	/* Start of main function begins here */
	solvingFlag = true;
	hash = 0ull;
	results = NULL;
	moveOrder = NULL;
	pv = NULL;

	// Use variant depending on compiler flags
#ifdef USE_MACROS
#ifdef NORMAL_RULESET
	variant = NORMAL_VARIANT;
#elif defined(POPOUT_RULESET)
	variant = POPOUT_VARIANT;
#elif defined(POWERUP_RULESET)
	variant = POWERUP_VARIANT;
#elif defined(POPTEN_RULESET)
	variant = POPTEN_VARIANT;
#elif defined(FIVEINAROW_RULESET)
	variant = FIVEINAROW_VARIANT;
#endif
#else
	variant = POPOUT_VARIANT;
#endif

	// Lower the process priority to make the operating system more responsive to other tasks
#ifdef _WIN32 // Microsoft Windows API
	SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);
#elif __unix__ // Unix and Linux POSIX
	setpriority(PRIO_PROCESS, getpid(), 5);
#endif

	// Initialize the ConnectFour structure and therefore the game itself
	ConnectFour_setSizeAndVariant(7, 6, variant);
	ConnectFour_setupBitmaps();
	ConnectFour_initialize(&cf);

	// Compute the number of bytes used by the encoded Connect Four position--for opening book generation
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
#ifdef USE_FIXED_TTSIZE // Use a fixed, constant transposition table size or an automatic one
	TranspositionTable_initialize(&table, (tableSize = 2 * TT_TABLESIZE));
#else
	{
#ifdef _WIN32 // Microsoft Windows
		unsigned long long totalGigabytes;

		if (GetPhysicallyInstalledSystemMemory(&totalGigabytes)) { // In kilobytes. Must divide twice to get megabytes and then gigabytes.
			totalGigabytes /= intpow(1024, 2);
		}
		else {
			fprintf(stderr, "WARNING: Could not determine system memory size; defaulting to one gigabyte.\n");
			totalGigabytes = 1ull;
		}

		for (table.entries = NULL; totalGigabytes && !table.entries;) {
			if (!TranspositionTable_initialize(&table, (tableSize = TT_TABLESIZE * totalGigabytes))) {
				totalGigabytes /= 2ull;
			}
		}

#elif defined(__unix__) // Unix and Linux
		struct sysinfo systemInformation;
		unsigned long totalGigabytes;

		// Get hardware information about this computer.
		if (sysinfo(&systemInformation) != -1) {
			totalGigabytes = systemInformation.totalram / intpow(1000, 3);
		}
		else {
			fprintf(stderr, "WARNING: Could not determine system memory size; defaulting to one gigabyte.\n");
			totalGigabytes = 1ul;
		}

		for (table.entries = NULL; totalGigabytes && !table.entries;) {
			if (!TranspositionTable_initialize(&table, (tableSize = TT_TABLESIZE * totalGigabytes))) {
				totalGigabytes /= 2ul;
			}
		}
#else
		TranspositionTable_initialize(&table, (tableSize = TT_TABLESIZE));
#endif
	}
#endif

	// Check if memory allocations were successful
	if (!table.entries) {
		fprintf(stderr, "Unable to allocate memory for the hashtable entry data.\n");
		exit(EXIT_FAILURE);
	}
	if (!(results = malloc(sizeof(Result) * ConnectFour_getMoveSize()))) {
		fprintf(stderr, "Error allocating memory for the game solution results.");
		exit(EXIT_FAILURE);
	}
	if (!(moveOrder = malloc(sizeof(int) * COLUMNS))) {
		fprintf(stderr, "Error allocating memory for the alpha-beta move order.");
		exit(EXIT_FAILURE);
	}
	if (!(pv = malloc(sizeof(char) * MOVESIZE))) {
		fprintf(stderr, "Error allocating memory for the principal variation.\n");
	}

	// Prepare for alpha-beta pruning static move ordering
	AlphaBeta_getColumnMoveOrder();

	// Print basic information on the console or terminal
	puts("Four the Win! version 0.1 alpha by TheTrustedComputer");
	printf("Board size configuration is set to %dx%d\n", COLUMNS, ROWS);
	printf("Transposition table consists of %u entries\n", table.size);
	printf("Game ruleset is ");

	switch (GAME_VARIANT) {
	case NORMAL_VARIANT:
		puts("Normal");
		break;
	case POPOUT_VARIANT:
		puts("PopOut");
		break;
	case POWERUP_VARIANT:
		puts("Power Up");
		break;
	case POPTEN_VARIANT:
		puts("Pop Ten");
		break;
	case FIVEINAROW_VARIANT:
		puts("Five-In-A-Row");
		break;
	default:
		puts("Unknown");
	}

	// Information for debugging purposes--irrelavent for the user
	printf("Address of Connect Four board: 0x%llX\n", (unsigned long long)&cf.board[0]);

	// Code handling command-line switches
	if (argc == 2 && argv[1] == "-bench") {
		ConnectFour_benchmark(&cf, 1000000000);
		return 0;
	}

	// Read and process the opening book if it exists
	if (GAME_VARIANT != POPTEN_VARIANT && !BookFile_readFromDrive(bookFileName)) {
		puts("Creating an empty opening book...");
		if (BookFile_create(bookFileName)) {
			fprintf(stderr, "Opening book creation succeeded.\n");
		}
	}

	// An infinite loop that constantly keep asking for input until closed
	for (i = 0; !i;) {
		if (argc >= 2) {
			if (!strcmp(argv[1], "-generate")) {
				if ((bookPly = atoi(argv[2]))) {
					ConnectFour_sequence(&cf, argv[3]);
					printf("Generating a %u-ply opening book from position %s...\n", bookPly, argv[3]);
					BookFile_generateBook(bookFileName, &cf, bookPly + cf.plyNumber);
				}
				else {
					puts("Cannot generate the opening book because there are invalid characters or a zero-ply length was given.");
				}
				break;
			}
			else {
				if (ConnectFour_sequence(&cf, argv[1])) {
					++i;
				}
				else {
					break;
				}
			}
		}
		else {
			while ((input = getchar()) != EOF) {
				if (ConnectFour_gameOver(&cf)) {
					puts("The position you entered is a drawn or won position.");
					ConnectFour_reset(&cf, false);
					while (input != '\n') {
						input = getchar();
					}
					continue;
				}
				if (!ConnectFour_play(&cf, input)) {
					if (input == '?') {
						solvingFlag = !solvingFlag;
						while (input != '\n') {
							input = getchar();
						}
						break;
					}
					if (input == '\n') {
						break;
					}
				}
			}
		}
		oldBoard[0] = cf.board[0];
		oldBoard[1] = cf.board[1];
		if (GAME_VARIANT == POWERUP_VARIANT) {
			oldPowerCheckers[0] = cf.pc->anvil;
			oldPowerCheckers[1] = cf.pc->bomb;
			oldPowerCheckers[2] = cf.pc->wall;
			oldPowerCheckers[3] = cf.pc->x2;
		}
		ConnectFour_printBoard(&cf);
		if (GAME_VARIANT == POPOUT_VARIANT) {
			ConnectFour_clearHistory(&cf);
		}
		if (pv) {
			for (j = 0; j < MOVESIZE; ++j) {
				pv[j] = '\xff';
			}
		}
#ifdef SCORE_TEST
		int scoretest = DRAW, possibleFlag = 1, deepPly = MOVESIZE - cf.plyNumber, currbestMove = 0;
		pv[0] = moveOrder[0] + '1';
		nodes = 0;
		repetitionFlag = 1;
		stopwatch = clock();
		for (j = 0; j < deepPly; ++j) {
			switch (GAME_VARIANT) {
			case NORMAL_VARIANT:
				if ((scoretest = AlphaBeta_negamax_normal_withMoves(&cf, 0, j, -1, 1))) {
					goto bestMoveFound;
				}
				break;
			case POPOUT_VARIANT:
				if (abs((scoretest = AlphaBeta_negamax_popout_withMoves(&cf, 0, j, -PLAYER_WIN, PLAYER_WIN))) >= PLAYER_WIN) {
					goto bestMoveFound;
				}
				else if (possibleFlag && abs(scoretest) == DRAW_OR_WIN) {
					printf("Position is a possible draw by repetition. (%d)\n", j);
					possibleFlag = 0;
				}
				break;
			case POPTEN_VARIANT:
				if ((scoretest = AlphaBeta_negamax_popten_withMoves(&cf, 0, j, -1, 1))) {
					goto bestMoveFound;
				}
			}
			if (currbestMove != pv[0]) {
				printf("Currently searched best move: %c\n", (currbestMove = pv[0]));
				repetitionFlag = 1;
			}
		}
		bestMoveFound:
		stopwatch = clock() - stopwatch;
		sec = (double)stopwatch / CLOCKS_PER_SEC;
		npsec = (double)nodes / sec;
		printf("\a");
		if (GAME_VARIANT != POPOUT_VARIANT) {
			if (scoretest > DRAW) {
				printf("%s", WIN_TEXT);
			}
			else if (scoretest < DRAW) {
				printf("%s", LOSS_TEXT);
			}
			else {
				printf("%s", DRAW_TEXT);
			}
		}
		else {
			if (scoretest == PLAYER_WIN) {
				printf("%s", WIN_TEXT);
			}
			else if (scoretest == -PLAYER_WIN) {
				printf("%s", LOSS_TEXT);
			}
			else {
				printf("%s", DRAW_TEXT);
			}
		}
		printf("%c %llu %.0f %.3f (%d)\n", pv[0], nodes, npsec, sec, j);
		ConnectFour_reset(&cf, false);
#ifdef __GNUC__
		TranspositionTable_destroy(&table);
		TranspositionTable_initialize(&table, tableSize);
#else
		TranspositionTable_reset(&table);
#endif
		continue;
#endif
		if (!solvingFlag) {
			printf("Moves: ");
			switch (GAME_VARIANT) {
			case NORMAL_VARIANT:
			case FIVEINAROW_VARIANT:
				for (j = 0; j < COLUMNS; ++j) {
					if (ConnectFour_normal_drop(&cf, j)) {
						printf("%c ", j < 10 ? j + '1' : j + 'A');
						ConnectFour_normal_undrop(&cf);
					}
				}
				break;
			case POPOUT_VARIANT:
				for (j = 0; j < COLUMNS; ++j) {
					if (ConnectFour_popout_drop(&cf, j)) {
						printf("%c ", j < 10 ? j + '1' : j + 'A');
						ConnectFour_popout_undrop(&cf);
					}
				}
				for (j = 0; j < COLUMNS; ++j) {
					if (ConnectFour_popout_pop(&cf, j)) {
						printf("%c ", j < 10 ? j + 'A' : j + 'a');
						ConnectFour_popout_unpop(&cf);
					}
				}
				break;
			case POPTEN_VARIANT:
				for (j = 0; j < COLUMNS; ++j) {
					if (ConnectFour_popten_drop(&cf, j)) {
						printf("%c ", j < 10 ? j + '1' : j + 'A');
						ConnectFour_popten_undrop(&cf);
					}
				}
				for (j = 0; j < COLUMNS; ++j) {
					if (ConnectFour_popten_pop(&cf, j)) {
						printf("%c ", j < 10 ? j + 'A' : j + 'a');
						ConnectFour_popten_unpop(&cf);
					}
				}
				if (ConnectFour_popten_pass(&cf)) {
					printf("Pass");
					ConnectFour_popten_unpass(&cf);
				}
			}
			puts("");
			continue;
		}
		stopwatch = clock();
		if ((notInBook = !BookFile_retrieve(bookFileName, &cf, ConnectFour_getHashKey(&cf), &result, results))) {
			switch (GAME_VARIANT) {
			case NORMAL_VARIANT:
				result = AlphaBeta_normal_solve(&cf);
				break;
			case POPOUT_VARIANT:
				result = AlphaBeta_popout_solve(&cf);
				break;
			case POWERUP_VARIANT:
				result = AlphaBeta_powerup_solve(&cf);
				break;
			case POPTEN_VARIANT:
				result = AlphaBeta_popten_solve(&cf);
				break;
			case FIVEINAROW_VARIANT:
				break;
			}
		}
		stopwatch = clock() - stopwatch;
		sec = (double)stopwatch / CLOCKS_PER_SEC;
		npsec = (double)nodes / (sec ? sec : sec + 1.0);
		best = -1;
		printf("\a");
		assert(oldBoard[0] == cf.board[0] && oldBoard[1] == cf.board[1]);
		if (GAME_VARIANT == POWERUP_VARIANT) {
			assert(oldPowerCheckers[0] == cf.pc->anvil && oldPowerCheckers[1] == cf.pc->bomb && oldPowerCheckers[2] == cf.pc->wall && oldPowerCheckers[3] == cf.pc->x2);
		}
		if (GAME_VARIANT != POPTEN_VARIANT) {
			assert(result.wdl == WIN_CHAR && !(result.dtc & 1) || result.wdl == LOSS_CHAR && result.dtc & 1 || result.wdl == DRAW_CHAR);
		}
		switch (result.wdl) {
		case UNKNOWN_CHAR:
			printf("??? ");
			break;
		case DRAW_CHAR:
			printf("%s", DRAW_TEXT);
			break;
		default:
			if (result.wdl == WIN_CHAR && !result.dtc) {
				printf("%s", WIN_TEXT);
			}
			else {
				printf("%c%d ", result.wdl, result.dtc);
			}
		}
		if (notInBook) {
			printf("%llu %.0f %.3f\n", nodes, npsec, sec);
			if (i) {
				break;
			}
			switch (GAME_VARIANT) {
			case NORMAL_VARIANT:
				best = AlphaBeta_normal_getBestMove(&cf, results, true);
				break;
			case POPOUT_VARIANT:
				best = AlphaBeta_popout_getBestMove(&cf, results, true);
				break;
			case POPTEN_VARIANT:
				best = AlphaBeta_popten_getBestMove(&cf, results, true);
				break;
			}
			puts("");
		}
		else {
			puts("book");
			if (i) {
				break;
			}
			for (j = 0; j < COLUMNS; ++j) {
				Result_print(&results[j]);
			}
			if (GAME_VARIANT == POPOUT_VARIANT) {
				puts("");
				for (; j < COLUMNS_X2; ++j) {
					Result_print(&results[j]);
				}
			}
			best = Result_getBestMove(results, ConnectFour_getMoveSize());
			puts("");
		}
		printf("\aBest: %c\n", (best < COLUMNS) ? best + '1' : best + 'A' - COLUMNS);
		if (GAME_VARIANT != POWERUP_VARIANT) {
			unsigned moveSize = ConnectFour_getMoveSize();
			for (j = 0; j < moveSize; ++j) {
				assert(results[i].wdl == UNKNOWN_CHAR || results[i].wdl == WIN_CHAR || results[i].wdl == DRAW_CHAR || results[i].wdl == LOSS_CHAR);
				switch (result.wdl) {
				case WIN_CHAR:
					assert(results[i].wdl == UNKNOWN_CHAR || (results[i].wdl == WIN_CHAR && results[i].dtc >= result.dtc) || results[i].wdl == DRAW_CHAR || results[i].wdl == LOSS_CHAR);
					break;
				case DRAW_CHAR:
					assert(results[i].wdl == UNKNOWN_CHAR || results[i].wdl == DRAW_CHAR || results[i].wdl == LOSS_CHAR);
					break;
				case LOSS_CHAR:
					assert(results[i].wdl == UNKNOWN_CHAR || (results[i].wdl == LOSS_CHAR && results[i].dtc <= result.dtc));
				}
			}
		}
		/*if (GAME_VARIANT != POPTEN_VARIANT) {
			if (hash != ConnectFour_getHashKey(&cf)) {
				if (BookFile_write(book)) {
					BookFile_append(book, &cf, (hash = ConnectFour_getHashKey(&cf)), results);
				}
			}
		}*/
		if (notInBook) {
#ifdef __GNUC__
			TranspositionTable_destroy(&table);
			TranspositionTable_initialize(&table, tableSize);
#else
			TranspositionTable_reset(&table);
#endif
		}
		ConnectFour_reset(&cf, GAME_VARIANT == POPTEN_VARIANT);
		nodes = 0ull;
	}
	ConnectFour_destroy(&cf);
	TranspositionTable_destroy(&table);
	free(results);
	free(moveOrder);
	return EXIT_SUCCESS;
}
