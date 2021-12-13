#ifndef WINDOWGUI_H
#define WINDOWGUI_H

#ifdef _WIN32

#define WINGUI_READY_MESSAGE L"Ready."
#define WINGUI_SOLVING_MESSAGE L"Solving..."
#define WINGUI_STOPPED_MESSAGE L"Stopped."

#ifdef __GNUC__ // These commctrl.h definitions below are required for MinGW-w64 (gcc 8.1) to compile. They are not required if compiling from Visual Studio as they are already defined.

#define TDCBF_OK_BUTTON 1
#define TDCBF_CANCEL_BUTTON 8
#define TDCBF_CLOSE_BUTTON 32
#define TD_INFORMATION_ICON MAKEINTRESOURCEW(-3)

enum _TASKDIALOG_FLAGS {
	TDF_ENABLE_HYPERLINKS = 0x0001,
	TDF_USE_HICON_MAIN = 0x0002,
	TDF_USE_HICON_FOOTER = 0x0004,
	TDF_ALLOW_DIALOG_CANCELLATION = 0x0008,
	TDF_USE_COMMAND_LINKS = 0x0010,
	TDF_USE_COMMAND_LINKS_NO_ICON = 0x0020,
	TDF_EXPAND_FOOTER_AREA = 0x0040,
	TDF_EXPANDED_BY_DEFAULT = 0x0080,
	TDF_VERIFICATION_FLAG_CHECKED = 0x0100,
	TDF_SHOW_PROGRESS_BAR = 0x0200,
	TDF_SHOW_MARQUEE_PROGRESS_BAR = 0x0400,
	TDF_CALLBACK_TIMER = 0x0800,
	TDF_POSITION_RELATIVE_TO_WINDOW = 0x1000,
	TDF_RTL_LAYOUT = 0x2000,
	TDF_NO_DEFAULT_RADIO_BUTTON = 0x4000,
	TDF_CAN_BE_MINIMIZED = 0x8000,
#if (NTDDI_VERSION >= NTDDI_WIN8)
	TDF_NO_SET_FOREGROUND = 0x00010000,
#endif
	TDF_SIZE_TO_CONTENT = 0x01000000
};

typedef int TASKDIALOG_FLAGS;
typedef int TASKDIALOG_COMMON_BUTTON_FLAGS;
typedef HRESULT(CALLBACK* PFTASKDIALOGCALLBACK)(_In_ HWND hwnd, _In_ UINT msg, _In_ WPARAM wParam, _In_ LPARAM lParam, _In_ LONG_PTR lpRefData);

typedef struct _TASKDIALOG_BUTTON {
	int     nButtonID;
	PCWSTR  pszButtonText;
} TASKDIALOG_BUTTON;

typedef struct _TASKDIALOGCONFIG {
	UINT                           cbSize;
	HWND                           hwndParent;
	HINSTANCE                      hInstance;
	TASKDIALOG_FLAGS               dwFlags;
	TASKDIALOG_COMMON_BUTTON_FLAGS dwCommonButtons;
	PCWSTR                         pszWindowTitle;
	union {
		HICON  hMainIcon;
		PCWSTR pszMainIcon;
	} DUMMYUNIONNAME;
	PCWSTR                         pszMainInstruction;
	PCWSTR                         pszContent;
	UINT                           cButtons;
	const TASKDIALOG_BUTTON*	   pButtons;
	int                            nDefaultButton;
	UINT                           cRadioButtons;
	const TASKDIALOG_BUTTON*	   pRadioButtons;
	int                            nDefaultRadioButton;
	PCWSTR                         pszVerificationText;
	PCWSTR                         pszExpandedInformation;
	PCWSTR                         pszExpandedControlText;
	PCWSTR                         pszCollapsedControlText;
	union {
		HICON  hFooterIcon;
		PCWSTR pszFooterIcon;
	} DUMMYUNIONNAME2;
	PCWSTR                         pszFooter;
	PFTASKDIALOGCALLBACK           pfCallback;
	LONG_PTR                       lpCallbackData;
	UINT                           cxWidth;
} TASKDIALOGCONFIG;

WINCOMMCTRLAPI HRESULT WINAPI TaskDialogIndirect(_In_ const TASKDIALOGCONFIG* pTaskConfig, _Out_opt_ int* pnButton, _Out_opt_ int* pnRadioButton, _Out_opt_ BOOL* pfVerificationFlagChecked);
WINCOMMCTRLAPI HRESULT WINAPI TaskDialog(_In_opt_ HWND hwndOwner, _In_opt_ HINSTANCE hInstance, _In_opt_ PCWSTR pszWindowTitle, _In_opt_ PCWSTR pszMainInstruction, _In_opt_ PCWSTR pszContent, TASKDIALOG_COMMON_BUTTON_FLAGS dwCommonButtons, _In_opt_ PCWSTR pszIcon, _Out_opt_ int* pnButton);

#endif

enum MenuID {
	MENUID_GAME_RESET, MENUID_GAME_SOLVE, MENUID_GAME_RANDOM, MENUID_GAME_EXIT,
	MENUID_EDIT_REGRESS, MENUID_EDIT_PROGRESS, MENUID_EDIT_BEGINNING, MENUID_EDIT_END, MENUID_EDIT_POSITION, MENUID_EDIT_SIZE, MENUID_EDIT_COLOR, MENUID_EDIT_VARIANT,
	MENUID_SOLVER_BEST, MENUID_SOLVER_DISPLAY, MENUID_SOLVER_STOP, MENUID_SOLVER_ALL, MENUID_SOLVER_HASH,
	MENUID_BOOK_USE, MENUID_BOOK_GENERATE, MENUID_BOOK_ADD, MENUID_BOOK_CLEAR, MENUID_BOOK_INFORMATION,
	MENUID_HELP_DETAILS, MENUID_HELP_ABOUT
};

enum ControlID {
	CONTROLID_STATUSBAR = 2, CONTROLID_TEXTBOX
};

enum SubclassID {
	SUBCLASSID_TEXTBOX
};

ConnectFour WindowGUI_connectFour;
Result WindowGUI_result, *WindowGUI_results;
UINT WindowGUI_variant;
HANDLE threader;
HWND mainHandle, statusHandle, textHandle;

PAINTSTRUCT painter;
HDC deviceContext;

HWND WindowGUI_createWindow(WNDCLASSW*, HINSTANCE, LPWSTR);
LRESULT CALLBACK WindowGUI_windowProcedure(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WindowGUI_text_subclassProcedure(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);

BOOL WindowGUI_menus_append(HWND);
HWND WindowGUI_statusBar_create(HWND);
HWND WindowGUI_textBox_create(HWND);
BOOL WindowGUI_statusBar_resize(HWND);
BOOL WindowGUI_window_center(HWND);
BOOL WindowGUI_windowIcon_set(HWND);

BOOL WindowGUI_taskDialog_applicationVersion(HWND);
BOOL WindowGUI_taskDialog_gameDetails(HWND, ConnectFour*);
BOOL WindowGUI_taskDialog_variants(HWND, ConnectFour*);

BOOL WindowGUI_initialize(UINT, LPWCH);
VOID WindowGUI_destroy(VOID);
VOID WindowGUI_ConnectFour_solve(HWND);
DWORD WindowGUI_thread_solve(LPVOID);

#endif

#endif
