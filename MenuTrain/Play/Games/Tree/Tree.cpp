#include <windows.h>
#include <string>
#include <fstream>
#include <vector>
#include <ctime>
#include <sstream>
#include <algorithm>
#include <TlHelp32.h>

#define GRID_SIZE 5
#define TILE_SIZE 50
#define ID_TIMER 1
#define ID_BUTTONEXIT 5

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
HWND hButtonExit;
std::string username = "Guest";
bool timerStarted = false;
DWORD startTime = 0;

int grid[GRID_SIZE][GRID_SIZE];
POINT emptyTile = { GRID_SIZE - 1, GRID_SIZE - 1 };  // Bottom-right starts empty

int gridOffsetX = 0;
int gridOffsetY = 0;

std::string GetLastLoggedInUser() {
    std::ifstream file("current_user.txt");
    std::string line;
    if (file.is_open()) {
        std::getline(file, line);
        file.close();
    }

    size_t colonPos = line.find(':');
    if (colonPos != std::string::npos) {
        return line.substr(0, colonPos);
    }
    return "Guest";
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
    } else {
        MessageBox(hwnd, "Failed to open the application!", "Error", MB_OK);
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
    if (file.is_open()) {
        std::getline(file, line);
        file.close();
    }

    size_t colonPos = line.find(':');
    if (colonPos != std::string::npos) {
        return std::stoi(line.substr(colonPos + 1));
    }
    return 0;
}

void InitGrid() {
    int number = 1;
    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            grid[y][x] = number++;
        }
    }

    grid[GRID_SIZE-1][GRID_SIZE-1] = 0;  // Empty space

}



void ShuffleGrid() {
    srand(static_cast<unsigned>(time(nullptr)));
    for (int i = 0; i < 1000; ++i) {
        int dir = rand() % 4;
        int dx = 0, dy = 0;
        switch (dir) {
            case 0: dx = 1; break;
            case 1: dx = -1; break;
            case 2: dy = 1; break;
            case 3: dy = -1; break;
        }

        int newX = emptyTile.x + dx;
        int newY = emptyTile.y + dy;
        if (newX >= 0 && newX < GRID_SIZE && newY >= 0 && newY < GRID_SIZE) {
            std::swap(grid[emptyTile.y][emptyTile.x], grid[newY][newX]);
            emptyTile.x = newX;
            emptyTile.y = newY;
        }
    }
}

bool IsSolved() {
    int number = 1;
    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            if (y == GRID_SIZE - 1 && x == GRID_SIZE - 1) return grid[y][x] == 0;
            if (grid[y][x] != number++) return false;
        }
    }
    return true;
}

void DrawGrid(HDC hdc) {
    HBRUSH redBrush = CreateSolidBrush(RGB(32, 44, 68));
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, redBrush);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255, 255, 255));

    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            RECT rect = {
                gridOffsetX + x * TILE_SIZE, 
                gridOffsetY + y * TILE_SIZE, 
                gridOffsetX + (x + 1) * TILE_SIZE, 
                gridOffsetY + (y + 1) * TILE_SIZE
            };
            FillRect(hdc, &rect, redBrush);
            DrawEdge(hdc, &rect, BDR_RAISEDINNER, BF_RECT);

            if (grid[y][x] != 0) {
                std::string number = std::to_string(grid[y][x]);
                DrawText(hdc, number.c_str(), -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            }
        }
    }

    SelectObject(hdc, oldBrush);
    DeleteObject(redBrush);
}

void StartTimer(HWND hwnd) {
    if (!timerStarted) {
        timerStarted = true;
        startTime = GetTickCount();
        SetTimer(hwnd, ID_TIMER, 1000, NULL);
    }
}

void OnTileClick(HWND hwnd, int x, int y) {
    x = (x - gridOffsetX) / TILE_SIZE;
    y = (y - gridOffsetY) / TILE_SIZE;

    if (x < 0 || x >= GRID_SIZE || y < 0 || y >= GRID_SIZE) return;

    int dx = abs(x - emptyTile.x);
    int dy = abs(y - emptyTile.y);

    if ((dx == 1 && dy == 0) || (dx == 0 && dy == 1)) {
        std::swap(grid[y][x], grid[emptyTile.y][emptyTile.x]);
        emptyTile.x = x;
        emptyTile.y = y;
        StartTimer(hwnd);
    }
}

void FinishPuzzle(HWND hwnd) {
    KillTimer(hwnd, ID_TIMER);
    DWORD timeTaken = (GetTickCount() - startTime) / 1000;
    int reward = std::max(50 - static_cast<int>(timeTaken), 10);
    int currentCoins = LoadCoins(username) + reward;



    SaveCoins(username, currentCoins);
    std::ostringstream message;

    message << "Congratulations " << username << "!\n"
            << "Time Taken: " << timeTaken << " seconds\n"
            << "You earned " << reward << " coins!";
    MessageBox(hwnd, message.str().c_str(), "Puzzle Solved!", MB_OK);
    LaunchExecutable(hwnd, "Play\\Play.exe", "Play.exe");
    PostQuitMessage(0);
}

void DrawTimer(HDC hdc) {
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255, 255, 255));
    DWORD elapsed = (GetTickCount() - startTime) / 1000;
    std::ostringstream timerText;
    timerText << "Time: " << elapsed << " sec";
    TextOut(hdc, 480, 10, timerText.str().c_str(), timerText.str().size());
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    
    switch (msg) {
        case WM_CREATE:
            username = GetLastLoggedInUser();
            InitGrid();
            ShuffleGrid();

            hButtonExit = CreateWindow("Button", "Exit", WS_CHILD | WS_VISIBLE | WS_BORDER,
                430, 485, 100, 20, hwnd, (HMENU)ID_BUTTONEXIT, NULL, NULL);
            return 0;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            DrawGrid(hdc);
            if (timerStarted) {
                DrawTimer(hdc);
            }
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_COMMAND:
            if (LOWORD(wParam) == ID_BUTTONEXIT) {
                LaunchExecutable(hwnd, "Play\\Play.exe", "Play.exe");
                DestroyWindow(hwnd);
            } 

        case WM_TIMER:
            InvalidateRect(hwnd, NULL, TRUE);
            return 0;

        case WM_LBUTTONDOWN:
            OnTileClick(hwnd, LOWORD(lParam), HIWORD(lParam));
            InvalidateRect(hwnd, NULL, TRUE);
            if (IsSolved()) FinishPuzzle(hwnd);
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, PSTR, int nCmdShow) {
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "PuzzleGame";
    wc.hbrBackground = CreateSolidBrush(RGB(32, 44, 68));

    RegisterClass(&wc);

    int windowWidth = 960;
    int windowHeight = 540;

    gridOffsetX = (windowWidth - (GRID_SIZE * TILE_SIZE)) / 2;
    gridOffsetY = (windowHeight - (GRID_SIZE * TILE_SIZE)) / 2;

    HWND hwnd = CreateWindow(
        "PuzzleGame", "Puzzle Game", WS_POPUP | WS_SYSMENU,
        (GetSystemMetrics(SM_CXSCREEN) - windowWidth) / 2,
        (GetSystemMetrics(SM_CYSCREEN) - windowHeight) / 2,
        windowWidth, windowHeight,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) return 0;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}