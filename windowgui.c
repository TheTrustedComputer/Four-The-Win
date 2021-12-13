#include "windowgui.h"

HWND WindowGUI_createWindow(WNDCLASSW *windowClass, HINSTANCE hInstance, LPWSTR windowName) {
	*windowClass = (WNDCLASSW){ 0 };
	windowClass->lpszClassName = L"Four the Win! Window Class";
	windowClass->hInstance = hInstance;
	windowClass->lpfnWndProc = WindowGUI_windowProcedure;
	if (!RegisterClassW(windowClass)) {
		MessageBoxW(NULL, L"Could not register the current window class.", windowName, MB_ICONERROR);
		return NULL;
	}
	return CreateWindowW(windowClass->lpszClassName, windowName, WS_OVERLAPPEDWINDOW, CW_DEFAULT, CW_DEFAULT, COLUMNS * 100, ROWS * 100, NULL, NULL, hInstance, NULL);
}

LRESULT CALLBACK WindowGUI_windowProcedure(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_CREATE:
		WindowGUI_window_center(hwnd);
		WindowGUI_windowIcon_set(hwnd);
		WindowGUI_menus_append(hwnd);
		statusHandle = WindowGUI_statusBar_create(hwnd);
		textHandle = WindowGUI_textBox_create(hwnd);
		break;
	case WM_DESTROY:
		WindowGUI_destroy();
		PostQuitMessage(0);
		return 0;
	case WM_PAINT:
		deviceContext = BeginPaint(hwnd, &painter);
		FillRect(deviceContext, &painter.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
		EndPaint(hwnd, &painter);
		break;
	case WM_COMMAND:
		switch (wParam) {
		case MENUID_GAME_SOLVE:
			WindowGUI_ConnectFour_solve(hwnd);
			break;
		case MENUID_GAME_EXIT:
			DestroyWindow(hwnd);
			break;
		case MENUID_EDIT_VARIANT:
			WindowGUI_taskDialog_variants(hwnd, &WindowGUI_connectFour);
			break;
		case MENUID_SOLVER_STOP:
		{
			char lastSequence[256];
			DWORD threadStopper;
			GetExitCodeThread(threader, &threadStopper);
			if (threadStopper == STILL_ACTIVE) {
				TerminateThread(threader, EXIT_FAILURE);
				nodes = 0ull;
				GetWindowTextA(textHandle, lastSequence, 256);
				ConnectFour_reset(&WindowGUI_connectFour, false);
				ConnectFour_sequence(&WindowGUI_connectFour, lastSequence);
#ifdef __GNUC__
				TranspositionTable_destroy(&table);
				TranspositionTable_initialize(&table, TT_TABLESIZE);
#else
				TranspositionTable_reset(&table);
#endif
				SendMessageW(statusHandle, WM_SETTEXT, 0, (LPARAM)WINGUI_STOPPED_MESSAGE);
			}
		}
		break;
		case MENUID_HELP_DETAILS:
			WindowGUI_taskDialog_gameDetails(hwnd, &WindowGUI_connectFour);
			break;
		case MENUID_HELP_ABOUT:
			WindowGUI_taskDialog_applicationVersion(hwnd);
		}
		break;
	case WM_GETMINMAXINFO:
	{
		LPMINMAXINFO maxima = (LPMINMAXINFO)lParam;
		maxima->ptMinTrackSize.x = COLUMNS * 50;
		maxima->ptMinTrackSize.y = ROWS * 50;
	}
	break;
	case WM_SIZE:
		WindowGUI_statusBar_resize(hwnd);
		break;
	}
	return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK WindowGUI_text_subclassProcedure(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
	switch (uMsg) {
	case WM_CHAR:
		switch (wParam) {
		case VK_RETURN:
		{
			char textSequence[256];
			ConnectFour_reset(&WindowGUI_connectFour, false);
			GetWindowTextA(hwnd, textSequence, 256);
			ConnectFour_sequence(&WindowGUI_connectFour, textSequence);
			return 0;
		}
		}
		break;
	case WM_NCDESTROY:
		RemoveWindowSubclass(hwnd, WindowGUI_text_subclassProcedure, SUBCLASSID_TEXTBOX);
	}
	return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

BOOL WindowGUI_menus_append(HWND hwnd) {
	if (hwnd) {
		HMENU windowMenu, gameMenu, editMenu, solverMenu, bookMenu, helpMenu;
		WCHAR titleString[16] = L"", exitMenuString[32] = L"", helpString[32] = L"";
		windowMenu = CreateMenu();
		gameMenu = CreatePopupMenu();
		editMenu = CreatePopupMenu();
		solverMenu = CreatePopupMenu();
		bookMenu = CreatePopupMenu();
		helpMenu = CreatePopupMenu();
		GetWindowTextW(hwnd, titleString, _countof(titleString));
		wcscat_s(exitMenuString, _countof(exitMenuString), L"E&xit ");
		wcscat_s(exitMenuString, _countof(exitMenuString), titleString);
		wcscat_s(exitMenuString, _countof(exitMenuString), L"\tAlt+F4");
		wcscat_s(helpString, _countof(helpString), L"About &");
		wcscat_s(helpString, _countof(helpString), titleString);
		AppendMenuW(windowMenu, MF_POPUP, (UINT_PTR)gameMenu, L"&Game");
		AppendMenuW(windowMenu, MF_POPUP, (UINT_PTR)editMenu, L"&Edit");
		AppendMenuW(windowMenu, MF_POPUP, (UINT_PTR)solverMenu, L"&Solver");
		AppendMenuW(windowMenu, MF_POPUP, (UINT_PTR)bookMenu, L"&Book");
		AppendMenuW(windowMenu, MF_POPUP, (UINT_PTR)helpMenu, L"&Help");
		AppendMenuW(gameMenu, MF_STRING, MENUID_GAME_RESET, L"&Reset\tCtrl+N");
		AppendMenuW(gameMenu, MF_STRING, MENUID_GAME_SOLVE, L"&Solve\tSpace");
		AppendMenuW(gameMenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
		AppendMenuW(gameMenu, MF_STRING, MENUID_GAME_RANDOM, L"&Make Random Move\tR");
		AppendMenuW(gameMenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
		AppendMenuW(gameMenu, MF_STRING, MENUID_GAME_EXIT, exitMenuString);
		AppendMenuW(editMenu, MF_STRING, MENUID_EDIT_REGRESS, L"&Regress Move\tLeft");
		AppendMenuW(editMenu, MF_STRING, MENUID_EDIT_PROGRESS, L"&Progress Move\tRight");
		AppendMenuW(editMenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
		AppendMenuW(editMenu, MF_STRING, MENUID_EDIT_BEGINNING, L"&Go to Beginning\tShift+Left");
		AppendMenuW(editMenu, MF_STRING, MENUID_EDIT_END, L"Go to &End\tShift+Right");
		AppendMenuW(editMenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
		AppendMenuW(editMenu, MF_STRING, MENUID_EDIT_POSITION, L"&Board Position\tCtrl+W");
		AppendMenuW(editMenu, MF_STRING, MENUID_EDIT_SIZE, L"Board &Size\tCtrl+E");
		AppendMenuW(editMenu, MF_STRING, MENUID_EDIT_COLOR, L"Board &Colors\tCtrl+R");
		AppendMenuW(editMenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
		AppendMenuW(editMenu, MF_STRING, MENUID_EDIT_VARIANT, L"Select &Variant...");
		AppendMenuW(solverMenu, MF_STRING, MENUID_SOLVER_BEST, L"Obtain &Best Moves\tCtrl+B");
		AppendMenuW(solverMenu, MF_STRING, MENUID_SOLVER_DISPLAY, L"&Display Values\tCtrl+D");
		AppendMenuW(solverMenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
		AppendMenuW(solverMenu, MF_STRING, MENUID_SOLVER_STOP, L"&Stop Solving\tSpace");
		AppendMenuW(solverMenu, MF_STRING, MENUID_SOLVER_ALL, L"Solve &All\tShift+Space");
		AppendMenuW(solverMenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
		AppendMenuW(solverMenu, MF_STRING, MENUID_SOLVER_HASH, L"&Configure Hashtable Size...");
		AppendMenuW(bookMenu, MF_STRING, MENUID_BOOK_USE, L"&Use Opening Book\tCtrl+U");
		AppendMenuW(bookMenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
		AppendMenuW(bookMenu, MF_STRING, MENUID_BOOK_GENERATE, L"&Generate Opening Book...\tCtrl+G");
		AppendMenuW(bookMenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
		AppendMenuW(bookMenu, MF_STRING, MENUID_BOOK_ADD, L"&Add Solution to Book\t");
		AppendMenuW(bookMenu, MF_STRING, MENUID_BOOK_CLEAR, L"&Clear All Solutions\t");
		AppendMenuW(bookMenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
		AppendMenuW(bookMenu, MF_STRING, MENUID_BOOK_INFORMATION, L"Book &Information...");
		AppendMenuW(helpMenu, MF_STRING, MENUID_HELP_DETAILS, L"Game &Details");
		AppendMenuW(helpMenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
		AppendMenuW(helpMenu, MF_STRING, MENUID_HELP_ABOUT, helpString);
		SetMenu(hwnd, windowMenu);
		return TRUE;
	}
	return FALSE;
}

HWND WindowGUI_statusBar_create(HWND hwnd) {
	RECT statusRect;
	INT numParts;
	HWND statusBar = CreateWindowW(STATUSCLASSNAME, WINGUI_READY_MESSAGE, WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, hwnd, (HMENU)CONTROLID_STATUSBAR, GetModuleHandle(NULL), NULL);
	GetWindowRect(hwnd, &statusRect);
	numParts = statusRect.right - statusRect.left;
	SendMessageW(statusBar, SB_SETPARTS, (WPARAM)1, (LPARAM)&numParts);
	SendMessageW(statusBar, WM_SIZE, 0, 0);
	return statusBar;
}

HWND WindowGUI_textBox_create(HWND hwnd) {
	HFONT textFont = CreateFontW(16, FW_DONTCARE, FW_DONTCARE, FW_DONTCARE, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, L"Segoe UI");
	HWND textBox = CreateWindowW(L"edit", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL, 0, 0, 222, 20, hwnd, (HMENU)CONTROLID_TEXTBOX, NULL, NULL);
	SendMessageW(textBox, WM_SETFONT, (WPARAM)textFont, 1);
	SendMessageW(textBox, EM_LIMITTEXT, MOVESIZE, 0);
	SetWindowSubclass(textBox, WindowGUI_text_subclassProcedure, SUBCLASSID_TEXTBOX, (DWORD_PTR)NULL);
	return textBox;
}

BOOL WindowGUI_statusBar_resize(HWND hwnd) {
	if (hwnd) {
		RECT rectResizer;
		INT partsResizer;
		GetWindowRect(hwnd, &rectResizer);
		partsResizer = rectResizer.right;
		SendMessageW(statusHandle, SB_SETPARTS, (WPARAM)1, (LPARAM)&partsResizer);
		SendMessageW(statusHandle, WM_SIZE, 0, 0);
		return MoveWindow(statusHandle, 0, 0, 0, 0, TRUE);
	}
	return FALSE;
}

BOOL WindowGUI_taskDialog_applicationVersion(HWND hwnd) {
	if (hwnd) {
		INT taskSelector;
		WCHAR aboutString[16] = L"About ", titleHeader[64] = L"";
		GetWindowTextW(hwnd, titleHeader, _countof(titleHeader));
		GetWindowTextW(hwnd, aboutString, _countof(aboutString));
		wcscat_s(titleHeader, 64, L" version 0.1 alpha");
		TaskDialog(hwnd, NULL, aboutString, titleHeader, L"Copyright 2019 TheTrustedComputer.\nThis application is currently under development.\n\nConnect Four and related terms are trademarks of Hasbro, Inc.", TDCBF_CLOSE_BUTTON, NULL, &taskSelector);
		return TRUE;
	}
	return FALSE;
}

BOOL WindowGUI_taskDialog_gameDetails(HWND hwnd, ConnectFour *cf) {
	if (hwnd) {
		INT taskSelector;
		WCHAR gameCaption[16] = L"", gameMessage[128] = L"";
		GetWindowTextW(hwnd, gameCaption, _countof(gameCaption));
		TaskDialog(hwnd, NULL, gameCaption, L"Game Details", L"To be implemented.", TDCBF_OK_BUTTON, TD_INFORMATION_ICON, &taskSelector);
		return TRUE;
	}
	return FALSE;
}

BOOL WindowGUI_taskDialog_variants(HWND hwnd, ConnectFour *cf) {
	if (hwnd) {
		INT variantButtonSelector;
		HRESULT hresult;
		WCHAR variantTitle[16];
		TASKDIALOGCONFIG variantConfiguration = { 0 };
		TASKDIALOG_BUTTON variantButtons[] = { { 0, L"&Original\nDrop disks to align four of your disks in a vertical, horizontal, or diagonal manner." },
												{1, L"PopOut\nYou can remove your own disk at the bottom, making the others on top fall down."},
												{2, L"Power Up\nPlay any of the four marked disks to produce a more strategical and tactical game."},
												{3, L"Pop Ten\nPop your way out to be the first player to possess ten disks. "},
												{4, L"Five-In-A-Row\nInstead of aligning four in a row, you instead have to align five in a row!"} };
		GetWindowTextW(hwnd, variantTitle, _countof(variantTitle));
		variantConfiguration.cbSize = sizeof(TASKDIALOGCONFIG);
		variantConfiguration.hwndParent = hwnd;
		variantConfiguration.dwFlags = TDF_USE_COMMAND_LINKS;
		variantConfiguration.dwCommonButtons = TDCBF_CANCEL_BUTTON;
		variantConfiguration.pszWindowTitle = variantTitle;
		variantConfiguration.pszMainIcon = NULL;
		variantConfiguration.pszMainInstruction = L"Select a variant";
		variantConfiguration.pszContent = L"Having variants add enjoyment to any game, so why not do the same for Connect Four?";
		variantConfiguration.cButtons = _countof(variantButtons);
		variantConfiguration.pButtons = variantButtons;
		variantConfiguration.nDefaultButton = 0;
		hresult = TaskDialogIndirect(&variantConfiguration, &variantButtonSelector, NULL, NULL);
		return TRUE;
	}
	return FALSE;
}

BOOL WindowGUI_window_center(HWND hwnd) {
	if (hwnd) {
		RECT windowRect;
		INT centerX, centerY, centerWidth, centerHeight, screenX, screenY;
		GetWindowRect(hwnd, &windowRect);
		screenX = GetSystemMetrics(SM_CXSCREEN);
		screenY = GetSystemMetrics(SM_CYSCREEN);
		centerWidth = windowRect.right - windowRect.left;
		centerHeight = windowRect.bottom - windowRect.top;
		centerX = (screenX - centerWidth) / 2;
		centerY = (screenY - centerHeight) / 2;
		return MoveWindow(hwnd, centerX, centerY, centerWidth, centerHeight, FALSE);
	}
	return FALSE;
}

BOOL WindowGUI_windowIcon_set(HWND hwnd) {
	HICON icon;
	if ((icon = LoadIconW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDI_ICON1)))) {
		SendMessageW(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)icon);
		return TRUE;
	}
	return FALSE;
}

BOOL WindowGUI_initialize(UINT initVariant, LPWCH errorTitle) {
	WindowGUI_variant = initVariant;
	ConnectFour_setSizeAndVariant(7, 6, WindowGUI_variant);
	ConnectFour_setupBitmaps();
	ConnectFour_initialize(&WindowGUI_connectFour);
	TranspositionTable_initialize(&table, TT_TABLESIZE);
	if (!(moveOrder = malloc(sizeof(int) * COLUMNS))) {
		MessageBoxW(NULL, L"Could not allocate memory for the alpha-beta move sorter.", errorTitle, MB_ICONERROR);
		return FALSE;
	}
	AlphaBeta_getColumnMoveOrder();
	return TRUE;
}

VOID WindowGUI_destroy(VOID) {
	ConnectFour_destroy(&WindowGUI_connectFour);
	TranspositionTable_destroy(&table);
	free(moveOrder);
}

VOID WindowGUI_ConnectFour_solve(HWND hwnd) {
	DWORD threadExitCode;
	GetExitCodeThread(threader, &threadExitCode);
	if (threadExitCode != STILL_ACTIVE) {
		SendMessageW(statusHandle, WM_SETTEXT, 0, (LPARAM)WINGUI_SOLVING_MESSAGE);
		threader = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)WindowGUI_thread_solve, statusHandle, 0, NULL);
	}
}

DWORD WindowGUI_thread_solve(LPVOID lpParam) {
	double seconds, nodesPerSeconds;
	WCHAR solverResult[64] = { 0 }, solverTemp[5] = { 0 }, nodesResult[64] = { 0 };
	clock_t elasped = clock();
	switch (WindowGUI_variant) {
	case NORMAL_VARIANT:
		WindowGUI_result = AlphaBeta_normal_solve(&WindowGUI_connectFour);
		break;
	case POPOUT_VARIANT:
		WindowGUI_result = AlphaBeta_normal_solve(&WindowGUI_connectFour);
		break;
	case POWERUP_VARIANT:
		WindowGUI_result = AlphaBeta_powerup_solve(&WindowGUI_connectFour);
	}
	elasped = clock() - elasped;
	seconds = (double)elasped / CLOCKS_PER_SEC;
	nodesPerSeconds = (double)nodes / seconds;
	switch (WindowGUI_result.wdl) {
	case UNKNOWN_CHAR:
	case DRAW_CHAR:
		wcscat_s(solverResult, 6, L"DRAW ");
		break;
	default:
		if (WindowGUI_result.wdl == 'W' && !WindowGUI_result.dtc) {
			wcscat_s(solverResult, 5, L"WIN ");
		}
		else {
			swprintf_s(solverTemp, _countof(solverTemp), L"%c%d ", WindowGUI_result.wdl, WindowGUI_result.dtc);
			wcscat_s(solverResult, _countof(solverResult), solverTemp);
		}
	}
	swprintf_s(nodesResult, _countof(nodesResult), L"%llu %.0f %.3f", nodes, nodesPerSeconds, seconds);
	wcscat_s(solverResult, _countof(solverResult), nodesResult);
	SendMessageW(lpParam, WM_SETTEXT, 0, (LPARAM)solverResult);
	nodes = 0ull;
#ifdef __GNUC__
	TranspositionTable_destroy(&table);
	TranspositionTable_initialize(&table, TT_TABLESIZE);
#else
	TranspositionTable_reset(&table);
#endif
	return 0;
}