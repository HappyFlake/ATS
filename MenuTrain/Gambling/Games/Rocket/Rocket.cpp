#include <windows.h>
#include <string>
#include <fstream>
#include <ctime>
#include <sstream>
#include <algorithm>
#include <random>
#include <TlHelp32.h>

#define ID_BUTTONBET 1
#define ID_BUTTONEXIT 5
#define ID_EDIT1  2
#define ID_WITHDRAW 4
#define ID_MODETOGGLE 10



LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

const char placeholder1[] = "Amount";

HWND hButtonBet, hButtonExit, hEdit1, hButtonWithdraw;
std::string username = "Guest";
bool gameRunning = false;
double multiplier = 1.0;

RECT fillRect;
RECT progressRect;

bool autoMode = false;            // Default to manual mode
double cashoutTarget = 2.0;       // Example default value (you can change it)



int balance = 10;      // User's coins
double currentBet = 0; // Current round bet (reset every round)

int gridOffsetX = 0;
int gridOffsetY = 0;

UINT_PTR gameTimer = 0;

std::random_device rd;
std::mt19937 rng(rd());
std::uniform_real_distribution<> crashChance(0.0005, 0.005);

// Scrapped idea
void ToggleMode(HWND hwnd) {
    autoMode = !autoMode;
    std::ostringstream msg;
    msg << "Auto mode: " << (autoMode ? "ON" : "OFF");
    MessageBox(hwnd, msg.str().c_str(), "Mode Toggled", MB_OK);
}
// Function to close an already running process by name
void CloseExistingProcess(const char* processName) {
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return;

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnap, &pe)) {
        do {
            if (_stricmp(pe.szExeFile, processName) == 0) {
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                if (hProcess) {
                    TerminateProcess(hProcess, 0);
                    CloseHandle(hProcess);
                }
            }
        } while (Process32Next(hSnap, &pe));
    }

    CloseHandle(hSnap);
}

// Function to open .exe file
void LaunchExecutable(HWND hwnd, const char* exePath, const char* processName) {
    CloseExistingProcess(processName); // Close existing instance if running

    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi;

    if (CreateProcess(exePath, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}

void LoadUsernameAndCoins() {
    std::ifstream file("current_user.txt");
    std::string line;
    if (std::getline(file, line)) {
        size_t colon = line.find(':');
        if (colon != std::string::npos) {
            username = line.substr(0, colon);
        }
    }
}

void SaveCoins(const std::string& user, int coins) {
    std::ofstream file("current_user.txt");
    if (file.is_open()) {
        file << user << ":" << coins;
        file.close();
    }
}

int LoadCoins(const std::string& user) {
    std::ifstream file("current_user.txt");
    std::string line;
    if (file.is_open() && std::getline(file, line)) {
        file.close();
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            return std::stoi(line.substr(colonPos + 1));
        }
    }
    return 0;
}

void UpdateBetWithdrawButton() {
    if (gameRunning) {
        SetWindowText(hButtonBet, "WITHDRAW");
    } else {
        SetWindowText(hButtonBet, "BET");
    }
}

void StartGame(HWND hwnd) {
    if (gameRunning) return;
    multiplier = 1.0;

    char betText[100] = "";
    GetWindowText(hEdit1, betText, sizeof(betText));

    if (strcmp(betText, "Amount") == 0 || strlen(betText) == 0) {
        MessageBox(hwnd, "Please enter a valid number!", "Error", MB_OK);
        return;
    }

    currentBet = atof(betText);

    if (currentBet <= 0 || currentBet > balance) {
        MessageBox(hwnd, "Invalid bet amount!", "Error", MB_OK);
        return;
    }

    balance -= currentBet;
    SaveCoins(username, balance);
    InvalidateRect(hwnd, NULL, TRUE);
    gameRunning = true;
    gameTimer = SetTimer(hwnd, 1, 100, NULL);

    UpdateBetWithdrawButton();
}

void EndGame(HWND hwnd, bool crashed) {
    KillTimer(hwnd, 1);
    gameRunning = false;
    int reward = std::max(50 - 10, 10);
    int currentCoins = LoadCoins(username) + reward;



    SaveCoins(username, currentCoins);
    std::ostringstream message;

    if (!crashed) {
        double winnings = currentBet * multiplier;
        balance += (int)winnings;
        std::ostringstream msg;
        msg << "You cashed out at " << multiplier << "x!\nWinnings: $" << winnings;
        MessageBox(hwnd, msg.str().c_str(), "Cashed Out!", MB_OK);
    } else {
        MessageBox(hwnd, "The rocket crashed! You lost the bet.", "Crash!", MB_OK);
    }
    
    SaveCoins(username, balance);
    InvalidateRect(hwnd, NULL, TRUE);  // Redraw balance display

    UpdateBetWithdrawButton();  // Restore to "BET"
}

void UpdateGame(HWND hwnd) {
    multiplier += 0.1;
    fillRect.right = progressRect.left + (LONG)((multiplier / 10.0) * (progressRect.right - progressRect.left));

    if (fillRect.top > progressRect.top) {
        fillRect.top = progressRect.top;
    }

    InvalidateRect(hwnd, &progressRect, TRUE);

    if (autoMode && multiplier >= cashoutTarget) {
        EndGame(hwnd, false);
    } else if ((rand() % 1000) < (crashChance(rng) * 1000)) {
        EndGame(hwnd, true);
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, PSTR, int nCmdShow) {
    const char CLASS_NAME[] = "Rocket Game";

    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = CreateSolidBrush(RGB(32, 44, 68));

    RegisterClass(&wc);

    RECT workArea;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);

    int windowWidth = 960, windowHeight = 540;
    int xPos = (workArea.right - windowWidth) / 2;
    int yPos = (workArea.bottom - windowHeight) / 2;

    HWND hwnd = CreateWindowEx(0, CLASS_NAME, "Rocket",
        WS_POPUP | WS_SYSMENU, xPos, yPos, windowWidth, windowHeight,
        NULL, NULL, hInstance, NULL);

    if (!hwnd) return 0;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static bool isPlaceholder1 = true;

    switch (msg) {
        case WM_CREATE:
            LoadUsernameAndCoins();
            balance = LoadCoins(username);

            hEdit1 = CreateWindow("Edit", placeholder1, WS_CHILD | WS_VISIBLE | WS_BORDER,
                                380, 410, 200, 20, hwnd, (HMENU)ID_EDIT1, NULL, NULL);

            hButtonBet = CreateWindow("Button", "BET", WS_CHILD | WS_VISIBLE,
                                    380, 435, 200, 20, hwnd, (HMENU)ID_BUTTONBET, NULL, NULL);
            
            hButtonExit = CreateWindow("Button", "Exit", WS_CHILD | WS_VISIBLE | WS_BORDER,
                                    430, 485, 100, 20, hwnd, (HMENU)ID_BUTTONEXIT, NULL, NULL);
            return 0;
        
            case WM_COMMAND:
                switch (LOWORD(wParam)) {
                        if (HIWORD(wParam) == EN_SETFOCUS || HIWORD(wParam) == EN_KILLFOCUS) {
                            HWND focused = (HWND)lParam;
                        if (focused == hEdit1 && isPlaceholder1 && HIWORD(wParam) == EN_SETFOCUS) {
                            SetWindowText(hEdit1, "");
                            isPlaceholder1 = false;
                        } else if (focused == hEdit1 && HIWORD(wParam) == EN_KILLFOCUS && GetWindowTextLength(hEdit1) == 0) {
                            SetWindowText(hEdit1, placeholder1);
                            isPlaceholder1 = true;
                        }
                    }
                    case ID_BUTTONBET:
                        if (gameRunning)
                            EndGame(hwnd, false);
                        else
                            StartGame(hwnd);
                    break;
                        
                        
        
                    case ID_BUTTONEXIT:
                        LaunchExecutable(hwnd, "Gambling\\Gambling.exe", "Gambling.exe");
                        DestroyWindow(hwnd);
                        break;
        
                    case ID_MODETOGGLE:
                        ToggleMode(hwnd);
                        break;
                }
                return 0;

                    case WM_TIMER:
                        if (wParam == 1) UpdateGame(hwnd);
                        return 0;
        
                    case WM_CTLCOLOREDIT:
                        
                    
                    case WM_PAINT: {
                        PAINTSTRUCT ps;
                        HDC hdc = BeginPaint(hwnd, &ps);

                        RECT progressRect = { 380, 150, 580, 350 };
                        FillRect(hdc, &progressRect, CreateSolidBrush(RGB(32, 44, 68)));
                        FrameRect(hdc, &progressRect, CreateSolidBrush(RGB(255, 255, 255)));
            
                        HBRUSH fillBrush = CreateSolidBrush(RGB(255, 255, 255));
                        RECT tempFillRect = progressRect;
                        tempFillRect.right = progressRect.left + (LONG)((multiplier / 10.0) * (progressRect.right - progressRect.left));
                        FillRect(hdc, &tempFillRect, fillBrush);
                        DeleteObject(fillBrush);
            
                        std::ostringstream multiplierText;
                        multiplierText.precision(1);
                        multiplierText << std::fixed << multiplier << "x";
            
                        SetBkMode(hdc, TRANSPARENT);
                        SetTextColor(hdc, RGB(255, 0, 0));
                        RECT textRect = progressRect;
                        DrawText(hdc, multiplierText.str().c_str(), -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            
                        std::ostringstream balanceText;
                        balanceText << balance << "$";
            
            
                        std::string message = "Rocket Game";
                        SIZE textSize;
                        GetTextExtentPoint32(hdc, message.c_str(), message.length(), &textSize);
                        int x = (960 - textSize.cx) / 2;
                        int y = 50;
                        TextOut(hdc, x, 385, balanceText.str().c_str(), balanceText.str().length());
                        TextOut(hdc, x, y, message.c_str(), message.length());
            
                        EndPaint(hwnd, &ps);
                        return 0;
                    }
        

            case WM_DESTROY:
                PostQuitMessage(0);
                return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}