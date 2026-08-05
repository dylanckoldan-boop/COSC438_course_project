// cosc438_project_virtpet.cpp : Defines the entry point for the application.
//
#define NOMINMAX

#include <windows.h>
#include "framework.h"
#include "cosc438_project_virtpet.h"
#include <string>
#include "MiniGameMW.h"
#include "MiniGame2MW.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <random>
#include <limits>
#include <algorithm>
#include <thread>

#define MAX_LOADSTRING 100
#define CLIENT_W 420
#define CLIENT_H 360
#define DECAY_TIMER_ID 1
#define DECAY_INTERVAL 10000    // milliseconds between each decay tick (10 seconds)

// -----------------------------------------------------------------------------
// Pet state (global for simplicity in a Win32 single-file project)
// -----------------------------------------------------------------------------
static int g_hunger = 100; //0 - starving, 100 - full
static int g_happiness = 100; // 0 = miserable, 100 = esctatic
static int g_health = 100; // 0 = sick, 100 = healthy
HWND g_hWnd = nullptr; // main window handle, used for triggering repaints from dialogs and game threads

// -----------------------------------------------------------------------------
// Global Variables:
// -----------------------------------------------------------------------------
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name


// -----------------------------------------------------------------------------
// Forward declarations of functions included in this code module:
// -----------------------------------------------------------------------------
void DrawStatBar(HDC, int, int, int, int, int, COLORREF, const wchar_t*);
void DrawPet(HDC, int);
void DrawStats(HDC);
void Feed_RefreshLabel(HWND);

ATOM                MyRegisterClass(HINSTANCE);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    FeedDlgProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    Game1DlgProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    Game2DlgProc(HWND, UINT, WPARAM, LPARAM);

// Helper: draw a centered stat bar using GDI
void DrawStatBar(HDC hdc, int x, int y, int barW, int barH, int value, COLORREF fillColor, const wchar_t* label)

{
    // Label
	SetBkMode(hdc, TRANSPARENT);
	SetTextColor(hdc, RGB(50, 50, 50));
	TextOutW(hdc, x, y, label, (int)wcslen(label));

    int barX = x + 105;
	int filled = (int)((value / 100.0) * barW);

	// Draw background (empty bar)
    RECT rcBg = { barX, y + 1, barX + barW, y + barH - 1 };
	HBRUSH hBg = CreateSolidBrush(RGB(210, 210, 210));
    FillRect(hdc, &rcBg, hBg);
	DeleteObject(hBg);

	// Draw filled portion
    if (filled > 0)
    {
		RECT rcFill = { barX, y + 1, barX + filled, y + barH -1 };
		HBRUSH hFill = CreateSolidBrush(fillColor);
		FillRect(hdc, &rcFill, hFill);
		DeleteObject(hFill);
    }

	// Draw border
	HPEN hPen = CreatePen(PS_SOLID, 2, RGB(90, 90, 90));
	HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
	HBRUSH hOldBr = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, barX, y,  barX + barW, y + barH);
    SelectObject(hdc, hOldPen);
    SelectObject(hdc, hOldBr);
    DeleteObject(hPen);

	// Numeric value
	std::wstring val = std::to_wstring(value) + L"%";
	TextOutW(hdc, barX + barW + 6, y, val.c_str(), (int)val.size());
}

// -----------------------------------------------------------------------------
// DrawPet - simple GDI "ASCII-style" pet drawn with lines and shapes.
//           The pet's expression changes based on overall wellness.
// -----------------------------------------------------------------------------
void DrawPet(HDC hdc, int clientW)
{
	int wellness = (g_hunger + g_happiness + g_health) / 3;

    //Center the pet horizontally in the top portion of the window
	int cx = clientW / 2;
    int topY = 18;

    // --- Choose pen/brush ---
    HPEN hPen = CreatePen(PS_SOLID, 3, RGB(60, 60, 60));
    HBRUSH hBody = CreateSolidBrush(RGB(255, 215, 100));   //warm yellow body
	HBRUSH hFace = CreateSolidBrush(RGB(255, 238, 170));   //lighter face
	HBRUSH hDark = CreateSolidBrush(RGB(40, 40, 40));       //dark eyes

    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBody);

    // --- Body (rounded rectangle approximated as ellipse) ---
    Ellipse(hdc, cx - 56, topY + 42, cx + 56, topY + 132);

    // --- Head ---
    SelectObject(hdc, hFace);
	Ellipse(hdc, cx - 42, topY, cx + 42, topY + 72);

    // --- Ears (two small filled ellipses above the head) ---
	SelectObject(hdc, hBody);
	Ellipse(hdc, cx - 48, topY - 20, cx - 22, topY + 12);   // Left ear
	Ellipse(hdc, cx + 22, topY - 20, cx + 48, topY + 12);   // Right ear

    // --- Arms/paws ---
	Ellipse(hdc, cx - 82, topY + 66, cx - 54, topY + 92);   // Left arm
	Ellipse(hdc, cx + 54, topY + 66, cx + 82, topY + 92); // Right arm

    // Tail
    {
        HPEN hTail = CreatePen(PS_SOLID, 3, RGB(195, 155, 55));
		SelectObject(hdc, hTail);
		SelectObject(hdc, GetStockObject(NULL_BRUSH));
		MoveToEx(hdc, cx + 53, topY + 96, nullptr);
		LineTo(hdc, cx + 76, topY + 80);
		LineTo(hdc, cx + 90, topY + 62);
		LineTo(hdc, cx + 82, topY + 48);
        SelectObject(hdc, hPen);
		DeleteObject(hTail);
    }

    // Eyes
	SelectObject(hdc, hDark);
    if (wellness >= 50)
    {
        // Open happy eyes
        Ellipse(hdc, cx - 24, topY + 18, cx - 11, topY + 31);
        Ellipse(hdc, cx + 11, topY + 18, cx + 24, topY + 31);
    }
    else
    {
        // Tired flat lines
		HPEN hSad = CreatePen(PS_SOLID, 3, RGB(40, 40, 40));
		SelectObject(hdc, hSad);
		SelectObject(hdc, GetStockObject(NULL_BRUSH));
		MoveToEx(hdc, cx - 24, topY + 25, nullptr); LineTo(hdc, cx - 11, topY + 25);
		MoveToEx(hdc, cx + 11, topY + 25, nullptr); LineTo(hdc, cx + 24, topY + 25);
		SelectObject(hdc, hPen);
		DeleteObject(hSad);
		SelectObject(hdc, hDark);
    }

    // Nose
	SelectObject(hdc, hDark);
	Ellipse(hdc, cx - 5, topY + 36, cx + 5, topY + 44);

    // Mouth
    {
		HPEN hMouth = CreatePen(PS_SOLID, 2, RGB(180, 70, 70));
		SelectObject(hdc, hMouth);
		SelectObject(hdc, GetStockObject(NULL_BRUSH));
        if (wellness >= 70)
            Arc(hdc, cx - 18, topY + 42, cx + 18, topY + 64, cx - 18, topY + 52, cx + 18, topY + 52);
        else if (wellness >= 40)
        {
			MoveToEx(hdc, cx - 13, topY + 53, nullptr);
			LineTo(hdc, cx + 13, topY + 53);
        }
        else
            Arc(hdc, cx - 18, topY + 46, cx + 18, topY + 68, cx + 18, topY + 57, cx - 18, topY + 57);
			SelectObject(hdc, hPen);
			DeleteObject(hMouth);
    }

    // --- Status label below the pet ---
    const wchar_t* mood =
    wellness >= 75 ? L"( ^..^ ) Your pet is very happy!":
    wellness >= 50 ? L"( -..- ) Your pet is doing okay.":
    wellness >= 25 ? L"( >..< ) Your pet needs attention!":
	                 L"( T..T ) Your pet is miserable!";

	HFONT hFont = CreateFontW(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
		DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
	HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
	SetBkMode(hdc, TRANSPARENT);
	SetTextColor(hdc, RGB(70, 70, 160));

    SIZE sz;
	int len = (int)wcslen(mood);
	GetTextExtentPoint32W(hdc, mood, len, &sz);
	TextOutW(hdc, cx - sz.cx / 2, topY + 140, mood, len);

	SelectObject(hdc, hOldFont);
    DeleteObject(hFont);

    // --- Cleanup ---
	SelectObject(hdc, hOldPen);
	SelectObject(hdc, hOldBrush);
    DeleteObject(hPen);
	DeleteObject(hBody);
	DeleteObject(hFace);
	DeleteObject(hDark);
}

// -----------------------------------------------------------------------------
// DrawStats - hunger / happiness / health bars below the pet
// -----------------------------------------------------------------------------
void DrawStats(HDC hdc)
{
    int x = 30;
    int y = 210;
    int barW = 180;
    int barH = 16;
    int gap = 28;

    HFONT hFont = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    HFONT hOld = (HFONT)SelectObject(hdc, hFont);

    DrawStatBar(hdc, x, y, barW, barH, g_hunger, RGB(255, 140, 40), L"Hunger:     ");
    DrawStatBar(hdc, x, y + gap, barW, barH, g_happiness, RGB(80, 180, 80), L"Happiness:");
    DrawStatBar(hdc, x, y + gap * 2, barW, barH, g_health, RGB(60, 140, 220), L"Health:    ");

    SelectObject(hdc, hOld);
    DeleteObject(hFont);
}

// =============================================================================
// wWinMain
// =============================================================================

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    //LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    //LoadStringW(hInstance, IDC_COSC438PROJECTVIRTPET, szWindowClass, MAX_LOADSTRING);
    //MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING) || szTitle[0] == L'\0') wcscpy_s(szTitle, MAX_LOADSTRING, L"Virtual Pet");

    if (!LoadStringW(hInstance, IDC_COSC438PROJECTVIRTPET, szWindowClass, MAX_LOADSTRING) || szWindowClass[0] == L'\0') wcscpy_s(szWindowClass, MAX_LOADSTRING, L"COSC438PROJECTVIRTPET");

    MyRegisterClass(hInstance);

    if (!InitInstance(hInstance, nCmdShow))
        return FALSE;

    HACCEL hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_COSC438PROJECTVIRTPET));
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccel, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}


// =============================================================================
//  FUNCTION: MyRegisterClass()
// =============================================================================
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex = {};

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.hInstance      = hInstance;
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_COSC438PROJECTVIRTPET);
    wcex.lpszClassName  = szWindowClass;
	//wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_COSC438PROJECTVIRTPET));
    //wcex.hIconSm        = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

// =============================================================================
//   FUNCTION: InitInstance(HINSTANCE, int)
// =============================================================================
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   // Start with desired client sizea and expand to windwo size
   RECT rc = { 0, 0, CLIENT_W, CLIENT_H };
   DWORD style = WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX; // fixed-size window
   AdjustWindowRect(&rc, style, TRUE); //True = has window

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, style,
	   CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
	   return FALSE;

	   ShowWindow(hWnd, nCmdShow);
       UpdateWindow(hWnd);
       return TRUE;
 }

// =============================================================================
// Save / Load pet state
// =============================================================================
static const wchar_t* SAVE_FILE = L"virtpet_save.dat";

static void SavePet()
{
    FILE* f = nullptr;
    _wfopen_s(&f, SAVE_FILE, L"wb");    // wb = write binary
    if (!f) return;

    fwrite(&g_hunger, sizeof(int), 1, f);
    fwrite(&g_happiness, sizeof(int), 1, f);
    fwrite(&g_health, sizeof(int), 1, f);

    fclose(f);
}

static void LoadPet()
{
    FILE* f = nullptr;
    _wfopen_s(&f, SAVE_FILE, L"rb");    // rb = read binary
    if (!f) return; // no save file yet - just keep defaults

    int h, hap, hel;
    if (fread(&h, sizeof(int), 1, f) == 1 &&
        fread(&hap, sizeof(int), 1, f) == 1 &&
        fread(&hel, sizeof(int), 1, f) == 1)
    {
        // Clamp on load just in case the file is corrupt/edited
        g_hunger = std::clamp(h, 0, 100);
        g_happiness = std::clamp(hap, 0, 100);
        g_health = std::clamp(hel, 0, 100);
    }

    fclose(f);
}
// =============================================================================
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
// =============================================================================
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        g_hWnd = hWnd;
        LoadPet();      // restore state if save exists
        SetTimer(hWnd, DECAY_TIMER_ID, DECAY_INTERVAL, nullptr);
        break;

    case WM_COMMAND:
        {
            // Parse the menu selections:
            switch (LOWORD(wParam))
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
                // --- Feed ---
            case IDM_FEED:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_FEED), hWnd, FeedDlgProc);
                InvalidateRect(hWnd, nullptr, TRUE);  // repaint pet after feeding
                break;

               // --- Game 1 ---
            case IDM_GAME1:
            {
                HWND capturedHwnd = hWnd;
                std::thread gameThread([capturedHwnd]() {
                    AllocConsole();
                    FILE* f;
                    freopen_s(&f, "CONOUT$", "w", stdout);
                    freopen_s(&f, "CONIN$", "r", stdin);

                    // --- Style the console game ---
                    HWND hConsole = GetConsoleWindow();
                    SetWindowText(hConsole, L"Reaction Test");
                    MoveWindow(hConsole, 400, 200, 600, 400, TRUE); //position and size

                    // Set conosole colors (yellow text on dark blue)
                    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
					SetConsoleTextAttribute(hOut, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY); // bright yellow

                    // Set console font size
                    CONSOLE_FONT_INFOEX fontInfo = { sizeof(CONSOLE_FONT_INFOEX) };
                    GetCurrentConsoleFontEx(hOut, FALSE, &fontInfo);
                    fontInfo.dwFontSize.Y = 24;
                    wcscpy_s(fontInfo.FaceName, L"Consolas");
                    SetCurrentConsoleFontEx(hOut, FALSE, &fontInfo);

                    // --- RUn the game ---
                    int happinessGained = MiniGameMW::playReactionTest();

                    // Post result back to main thread safely
                    g_happiness = std::min(100, g_happiness + happinessGained);

                    // --- Pause so player can read the result, then auto close ---
                    SetConsoleTextAttribute(hOut, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
                    std::cout << "\n  Closing in 3 seconds...";
                    Sleep(3000);

                    FreeConsole();

                    // Trigger repaint from the game thread
                    InvalidateRect(capturedHwnd, NULL, TRUE);
                });
                gameThread.detach();
                break;
            }

                // --- Game 2 ---
           /* case IDM_GAME2:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_GAME2), hWnd, Game2DlgProc);
                InvalidateRect(hWnd, nullptr, TRUE);
                break;*/

                //---------------------------------------------------------------
                    //this is where i am adding the game 2 in the switch case but it will look
                    //exectly the same as the game 1, where we go into the command line, but instead
                    //it will use a different minigame call, so a different set of methods get performed
                //---------------------------------------------------------------
            case IDM_GAME2:
            {
                HWND capturedHwnd = hWnd;
                std::thread gameThread([capturedHwnd]() {
                    AllocConsole();
                    FILE* f;
                    freopen_s(&f, "CONOUT$", "w", stdout);
                    freopen_s(&f, "CONIN$", "r", stdin);

                    // --- Style the console game ---
                    HWND hConsole = GetConsoleWindow();
                    SetWindowText(hConsole, L"Question Game");
                    MoveWindow(hConsole, 400, 200, 600, 400, TRUE); //position and size

                    // Set conosole colors (yellow text on dark blue)
                    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
                    SetConsoleTextAttribute(hOut, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY); // bright yellow

                    // Set console font size
                    CONSOLE_FONT_INFOEX fontInfo = { sizeof(CONSOLE_FONT_INFOEX) };
                    GetCurrentConsoleFontEx(hOut, FALSE, &fontInfo);
                    fontInfo.dwFontSize.Y = 24;
                    wcscpy_s(fontInfo.FaceName, L"Consolas");
                    SetCurrentConsoleFontEx(hOut, FALSE, &fontInfo);

                    // --- RUn the game ---
                    int happinessGained = MiniGame2MW::playQuestionGame();

                    // Post result back to main thread safely
                    g_happiness = std::min(100, g_happiness + happinessGained);

                    // --- Pause so player can read the result, then auto close ---
                    SetConsoleTextAttribute(hOut, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
                    std::cout << "\n  Closing in 3 seconds...";
                    Sleep(3000);

                    FreeConsole();

                    // Trigger repaint from the game thread
                    InvalidateRect(capturedHwnd, NULL, TRUE);
                    });
                gameThread.detach();
                break;
            }

            case IDM_SAVE:
                SavePet();
                MessageBoxW(hWnd, L"Game saved!", L"Save", MB_OK | MB_ICONINFORMATION);
                break;

            case IDM_LOAD:
                LoadPet();
                InvalidateRect(hWnd, nullptr, TRUE);
                MessageBoxW(hWnd, L"Pet loaded!", L"Load", MB_OK | MB_ICONINFORMATION);
                break;

            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;

            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
    }
        break;

    case WM_TIMER:
        if (wParam == DECAY_TIMER_ID)
        {
            // Decay rates - tweak these values to adjust how fast each stat drops
            g_hunger = std::max(0, g_hunger - 5);
            g_happiness = std::max(0, g_happiness - 3);

            // Health decays faster if hunger is critically low
            if (g_hunger < 20)
                g_health = std::max(0, g_health - 5);
            else
                g_health = std::max(0, g_health - 1);

            InvalidateRect(hWnd, nullptr, TRUE);    // trigger repaint to show update
        }
        break;
        // ----------------------------------------------------------------------------------

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            // TODO: Add any drawing code here...
			HDC hdc = BeginPaint(hWnd, &ps);

            RECT rc;
			GetClientRect(hWnd, &rc);

            // Fill background
			HBRUSH hBg = CreateSolidBrush(RGB(245, 245, 255));
			FillRect(hdc, &rc, hBg);
			DeleteObject(hBg);

            // Draw pet and stats into the memory DC
			DrawPet(hdc, rc.right);
			DrawStats(hdc);

			EndPaint(hWnd, &ps);
            break;
        }

		// ----------------------------------------------------------------------------------

    case WM_DESTROY:
        SavePet();      // auto-save on close
        KillTimer(hWnd, DECAY_TIMER_ID);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// =============================================================================
// Message handler for about box.
// =============================================================================
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

// =============================================================================
// Feed dialog helpers
// =============================================================================

//Updates the hunger label inside IDD_FEED with the current g_hunger value.
static void Feed_RefreshLabel(HWND hDlg)
{
    std::wstring s = L"Hunger: " + std::to_wstring(g_hunger) + L" / 100";
	SetDlgItemTextW(hDlg, IDC_FEED_HUNGER_LABEL, s.c_str());
}

// =============================================================================
// Feed dialog
//  - Shows current hunger level
//  - "Feed" button raises hunger by 20 (capped at 100)
//  - Happiness and health nudge slightly
// =============================================================================
INT_PTR CALLBACK FeedDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
    case WM_INITDIALOG:
        Feed_RefreshLabel(hDlg);
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_BTN_FEED:
        {
            if (g_hunger >= 100)
            {
                MessageBoxW(hDlg, L"Your pet is already full!", L"Feed", MB_OK | MB_ICONINFORMATION);
            }
            else
            {
                g_hunger = std::min(100, g_hunger + 20);
                g_health = std::min(100, g_health + 5);
                Feed_RefreshLabel(hDlg);
                MessageBoxW(hDlg, L"Yum! Your pet enjoyed the meal.", L"Feed", MB_OK | MB_ICONINFORMATION);
            }
            break;
        }
        case IDOK:
        case IDCANCEL:
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

// ---- helpers --------------------------------------------------------------------

// Flush any characters already sitting in stdin so we don't get a false "instant" read when the Go prompt appears.

static void flushStdin() {
	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

// ---- single round --------------------------------------------------------------

// Returns reaction time in milliseconds, or -1 on a false start.

int MiniGameMW::runRound(HANDLE hOut) {
    SetConsoleTextAttribute(hOut, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    std::cout << "\n Get ready... press ENTER the moment you see GO!\n";
    std::cout << "  (Don't press anything yet)\n";

    //Random delay: 1500 - 4000 ms
    std::mt19937 rng(static_cast<unsigned>(
        std::chrono::steady_clock::now().time_since_epoch().count()));
    std::uniform_int_distribution<int> dist(1500, 4000);
    int delayMs = dist(rng);

    // Sleep in small chunks so we can detact a premature keypress.
    // On most terminals, stdin is line-buffered, so we can't detect a keystroke mid-sleep without platform-specific code.
    // Instead we rely on the simple cross-platform approach: flush stdin right before showing GO, and if the very first read
    // returns instanntly ( <50 ms), treat it as a false start cause by the user holding Enter.

    std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));

    // ---- Show GO ----

    SetConsoleTextAttribute(hOut, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    std::cout << "\n *** GO! ***\n";
    std::cout.flush();

    auto t0 = std::chrono::steady_clock::now();

    // Block until Enter

    std::string dummy;
    std::getline(std::cin, dummy);

    auto t1 = std::chrono::steady_clock::now();
    int elapsed = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count());

    // False-start heuristic: reaction < 50ms is humanly impossible

    if (elapsed < 50) {
        return -1;
    }

    return elapsed;
}

// ---- rating label -----------------------------------------------------------------

std::string MiniGameMW::getRating(int ms) {
    if (ms < 200) return "LEGENDARY";
    if (ms < 300) return "Great";
    if (ms < 450) return "Good";
    if (ms < 600) return "Okay";
    return "Slow";
}

// ---- score -> happiness ------------------------------------------------------------

// Best-of-3 best time maps to 0-40 happiness points.

int MiniGameMW::scoreToHappiness(int bestMs) {
    if (bestMs < 200) return 40;
    if (bestMs < 300) return 30;
    if (bestMs < 450) return 20;
    if (bestMs < 600) return 10;
    return 5;
}

// =============================================================================
// I copied anothoer section of the code that was from the minigame2 clone
// =============================================================================

int MiniGame2MW::runRound2(HANDLE hOut) {
    SetConsoleTextAttribute(hOut, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    std::cout << "\n Get ready... press ENTER the moment you see GO!\n";
    std::cout << "  (Don't press anything yet)\n";

    //Random delay: 1500 - 4000 ms
    std::mt19937 rng(static_cast<unsigned>(
        std::chrono::steady_clock::now().time_since_epoch().count()));
    std::uniform_int_distribution<int> dist(1500, 4000);
    int delayMs = dist(rng);

    // Sleep in small chunks so we can detact a premature keypress.
    // On most terminals, stdin is line-buffered, so we can't detect a keystroke mid-sleep without platform-specific code.
    // Instead we rely on the simple cross-platform approach: flush stdin right before showing GO, and if the very first read
    // returns instanntly ( <50 ms), treat it as a false start cause by the user holding Enter.

    std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));

    // ---- Show GO ----

    SetConsoleTextAttribute(hOut, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    std::cout << "\n *** GO! ***\n";
    std::cout.flush();

    auto t0 = std::chrono::steady_clock::now();

    // Block until Enter

    std::string dummy;
    std::getline(std::cin, dummy);

    auto t1 = std::chrono::steady_clock::now();
    int elapsed = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count());

    // False-start heuristic: reaction < 50ms is humanly impossible

    if (elapsed < 50) {
        return -1;
    }

    return elapsed;
}

// ---- rating label -----------------------------------------------------------------

std::string MiniGame2MW::getRating2(int ms) {
    if (ms < 200) return "LEGENDARY";
    if (ms < 300) return "Great";
    if (ms < 450) return "Good";
    if (ms < 600) return "Okay";
    return "Slow";
}

// ---- score -> happiness ------------------------------------------------------------

// Best-of-3 best time maps to 0-40 happiness points.

int MiniGame2MW::scoreToHappiness2(int bestMs) {
    if (bestMs < 200) return 40;
    if (bestMs < 300) return 30;
    if (bestMs < 450) return 20;
    if (bestMs < 600) return 10;
    return 5;
}

// ---- public entry point ------------------------------------------------------------

int MiniGameMW::playReactionTest() {

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

    SetConsoleTextAttribute(hOut, FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    std::cout << "\n  ╔══════════════════════════════╗\n";
    std::cout << "  ║     REACTION TEST  ⚡         ║\n";
    std::cout << "  ║  Best of 3 rounds             ║\n";
    std::cout << "  ╚══════════════════════════════╝\n";

    const int TOTAL_ROUNDS = 3;
    int bestMs = std::numeric_limits<int>::max();
    int round = 1;

    while (round <= TOTAL_ROUNDS) {
        SetConsoleTextAttribute(hOut, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        std::cout << "\n --- Round " << round << " of " << TOTAL_ROUNDS << " ---";

        int result = runRound(hOut);

        if (result == -1) {
            // False start -> restart this round (don't increment)
            SetConsoleTextAttribute(hOut, FOREGROUND_RED | FOREGROUND_INTENSITY);
            std::cout << "\n ^ Too early! You pressed before GO appeared.\n";
            std::cout << "    Resarting this round...\n";
            // Brief pause so the message is readable
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            continue;
        }

        SetConsoleTextAttribute(hOut, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        std::string rating = getRating(result);
        std::cout << "\n  Your time : " << result << " ms [" << rating << "]\n";

        if (result < bestMs) {
            bestMs = result;
        }

        ++round;
    }

    // --- results ---

    SetConsoleTextAttribute(hOut, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    std::cout << "\n  ══════════════════════════════\n";

    SetConsoleTextAttribute(hOut, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    std::cout << "  Best time  : " << bestMs << " ms\n";
    std::cout << "  Rating     : " << getRating(bestMs) << "\n";

    SetConsoleTextAttribute(hOut, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    int happiness = scoreToHappiness(bestMs);
    std::cout << "  Happiness  : +" << happiness << "\n";

    SetConsoleTextAttribute(hOut, FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    std::cout << "  ══════════════════════════════\n";

    return happiness;

}

//-----------------------------------------------------------
// This comment block is in reference to the below method which is the second mini game.
// I intended it to look as a carbon copy of the initial reaction game, since the game will act 
// similarly to that one, and i can use as much code reuse as possible.  I know a variable could be
// passed to the same method you created, although, i didn't want to change the initial code in case
// we ran into a polymorphic issue with function calls.
//-----------------------------------------------------------

int MiniGame2MW::playQuestionGame() {

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

    SetConsoleTextAttribute(hOut, FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    std::cout << "\n";
    std::cout << "Why Shouldn't I Ask Questions?\n";
    std::cout << "Give me an answer!  And NOT just:  Because!\n";
    std::cout << "\n";

    const int TOTAL_ROUNDS = 3;
    int bestMs = std::numeric_limits<int>::max();
    int round = 1;
    std::string answer = "";

    while (answer != "Because!" && answer != "because!") {
        if (answer == "") {
                std::cout << "\nAnswer the question: Why shouldn't I ask questions?\n";
                std::cin >> answer;
        }
        else if (answer == "Because!" || answer == "because!") {
				break;
		}
        else if (answer != "Because!" && answer != "because!") {
                std::cin >> answer;
        }
    } 
    // --- results ---


    SetConsoleTextAttribute(hOut, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    int happiness = scoreToHappiness2(bestMs);
    std::cout << "  Happiness  : +" << happiness << "\n";

    return happiness;
}
