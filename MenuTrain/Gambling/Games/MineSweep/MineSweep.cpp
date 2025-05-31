#include <windows.h>
#include <string>
#include <fstream>
#include <vector>
#include <ctime>
#include <sstream>
#include <algorithm>
#include <TlHelp32.h>
#include <random>

#define ID_BUTTONBET 1
#define ID_BUTTONEXIT 5
#define ID_EDIT1  2
#define ID_EDIT2  3
#define ID_WITHDRAW 4
#define GRID_SIZE 5
#define TILE_SIZE 50

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

std::string username = "Guest";

const char placeholder1[] = "Amount";
const char placeholder2[] = "Mines";

int grid[GRID_SIZE][GRID_SIZE];
bool revealed[GRID_SIZE][GRID_SIZE];
bool mines[GRID_SIZE][GRID_SIZE];

int betAmount = 0;
int totalMines = 0;
bool gameActive = false;
int revealedTiles = 0;

int gridOffsetX = 0;
int gridOffsetY = 0;

int balance = 0; 

std::pair<std::string, int> GetCurrentUserWithCoins() {
    std::ifstream file("current_user.txt");
    std::string line;
    if (file.is_open()) {
        std::getline(file, line);
        file.close();
    }

    size_t colonPos = line.find(':');
    if (colonPos == std::string::npos) {
        return { "Guest", 0 }; //Default if it gets corrupted
    }

    std::string username = line.substr(0, colonPos);
    int coins = std::stoi(line.substr(colonPos + 1));
    return { username, coins };
}
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

void ResetGame() {
    memset(revealed, 0, sizeof(revealed));
    memset(mines, 0, sizeof(mines));
    revealedTiles = 0;
    gameActive = false;
}

void PlaceMines(int mineCount) {
    std::srand((unsigned int)std::time(nullptr));
    while (mineCount > 0) {
        int x = std::rand() % GRID_SIZE;
        int y = std::rand() % GRID_SIZE;
        if (!mines[y][x]) {
            mines[y][x] = true;
            mineCount--;
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
}

void DrawGrid(HDC hdc) {
    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            RECT rect = {
                gridOffsetX + x * TILE_SIZE,
                gridOffsetY + y * TILE_SIZE,
                gridOffsetX + (x + 1) * TILE_SIZE,
                gridOffsetY + (y + 1) * TILE_SIZE
            };

            HBRUSH brush = CreateSolidBrush(RGB(32, 44, 68));  // Default: black tile
            if (revealed[y][x]) {
                if (mines[y][x]) {
                    brush = CreateSolidBrush(RGB(255, 0, 0));  // Red if mine was clicked
                } else {
                    brush = CreateSolidBrush(RGB(0, 255, 0));  // Green for safe tiles
                }
            }

            FillRect(hdc, &rect, brush);
            FrameRect(hdc, &rect, CreateSolidBrush(RGB(255, 255, 255)));  // White outline
            SetBkMode(hdc, TRANSPARENT);

            DeleteObject(brush);
        }
    }
}


void HandleTileClick(HWND hwnd, int x, int y) {
    if (!gameActive || revealed[y][x]) return;

    revealed[y][x] = true;

    if (mines[y][x]) {
        InvalidateRect(hwnd, NULL, TRUE);  // Force immediate redraw to show red tile
        UpdateWindow(hwnd);  // Make sure the redraw happens before the MessageBox
        SetWindowText(GetDlgItem(hwnd, ID_BUTTONBET), "BET");
        MessageBox(hwnd, "You hit a mine! You lost your bet.", "Game Over", MB_OK);
        balance -= betAmount;
        ResetGame();
        
    } else {
        revealedTiles++;
        if (revealedTiles == (GRID_SIZE * GRID_SIZE - totalMines)) {
            int baseProfit = betAmount * revealedTiles / (GRID_SIZE * GRID_SIZE - totalMines);
            int greenBonus = 0;

            for (int yy = 0; yy < GRID_SIZE; yy++) {
                for (int xx = 0; xx < GRID_SIZE; xx++) {
                    if (revealed[yy][xx] && !mines[yy][xx]) {
                        greenBonus += (betAmount / 4);  // +25% per safe tile
                    }
                }
            }

            int profit = baseProfit + greenBonus;

            int currentBalance = LoadCoins(username);
            currentBalance += profit;
            SaveCoins(username, currentBalance);

            std::ostringstream withdrawMsg;
            withdrawMsg << "You revealed all safe tiles!\nAutomatic withdraw: " << profit << " coins!";
            MessageBox(hwnd, withdrawMsg.str().c_str(), "Auto Withdraw", MB_OK);

            ResetGame();
            SetWindowText(GetDlgItem(hwnd, ID_BUTTONBET), "BET");
        }
    }
    InvalidateRect(hwnd, NULL, TRUE);  // Normal redraw
}



// Inside FinishPuzzle:
void FinishPuzzle(HWND hwnd) {
    int currentBalance = LoadCoins(username) - betAmount;
    SaveCoins(username, currentBalance);

    int revealedSafeTiles = revealedTiles;
    int baseProfit = betAmount * revealedSafeTiles / (GRID_SIZE * GRID_SIZE - totalMines);
    int greenBonus = revealedSafeTiles * (betAmount / 4);
    int profit = baseProfit + greenBonus;

    currentBalance = LoadCoins(username) + profit;
    SaveCoins(username, currentBalance);

    std::ostringstream message;
    message << "Congratulations " << username << "!\n"
            << "You earned " << profit << " coins!";
    MessageBox(hwnd, message.str().c_str(), "Puzzle Solved!", MB_OK);
    PostQuitMessage(0);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, PSTR, int nCmdShow) {
    const char CLASS_NAME[] = "MineSweep";

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

    gridOffsetX = (windowWidth - (GRID_SIZE * TILE_SIZE)) / 2;
    gridOffsetY = (windowHeight - (GRID_SIZE * TILE_SIZE)) / 2;

    HWND hwnd = CreateWindowEx(0, CLASS_NAME, "Minesweep",
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
    static HWND hButtonBet, hEditAmount, hEditMines, hButtonExit;
    static bool isPlaceholder1 = true, isPlaceholder2 = true;

    switch (msg) {
        case WM_CREATE:
            username = GetLastLoggedInUser();
            hEditAmount = CreateWindow("Edit", placeholder1, WS_CHILD | WS_VISIBLE | WS_BORDER,
                                        277.5, 435, 200, 20, hwnd, (HMENU)ID_EDIT1, NULL, NULL);
            hEditMines = CreateWindow("Edit", placeholder2, WS_CHILD | WS_VISIBLE | WS_BORDER,
                                        482.5, 435, 200, 20, hwnd, (HMENU)ID_EDIT2, NULL, NULL);
            hButtonBet = CreateWindow("Button", "BET", WS_CHILD | WS_VISIBLE,
                                       277.5, 460, 405, 20, hwnd, (HMENU)ID_BUTTONBET, NULL, NULL);
            hButtonExit = CreateWindow("Button", "EXIT", WS_CHILD | WS_VISIBLE | WS_BORDER,
                                        430, 485, 100, 20, hwnd, (HMENU)ID_BUTTONEXIT, NULL, NULL);
            ResetGame();
            return 0;

        case WM_COMMAND:
            if (LOWORD(wParam) == ID_BUTTONBET) {
                if (gameActive) {
                    // WITHDRAW clicked
                    int baseProfit = betAmount * revealedTiles / (GRID_SIZE * GRID_SIZE - totalMines);

                    int greenBonus = 0;
                    for (int y = 0; y < GRID_SIZE; y++) {
                        for (int x = 0; x < GRID_SIZE; x++) {
                            if (revealed[y][x] && !mines[y][x]) {
                                greenBonus += (betAmount / 4);  // +25% per green tile
                            }
                        }
                    }

                    int profit = baseProfit + greenBonus;

                    int currentBalance = LoadCoins(username);
                    currentBalance += profit;
                    SaveCoins(username, currentBalance);

                    std::ostringstream withdrawMsg;
                    withdrawMsg << "You withdrew: " << profit << " coins!";
                    MessageBox(hwnd, withdrawMsg.str().c_str(), "Withdraw", MB_OK);

                    // Reset game state
                    ResetGame();
                    SetWindowText(hButtonBet, "BET");
                    InvalidateRect(hwnd, NULL, TRUE);
                } else {
                    // BET clicked
                    char amountStr[100], minesStr[100];
                    GetWindowText(hEditAmount, amountStr, sizeof(amountStr));
                    GetWindowText(hEditMines, minesStr, sizeof(minesStr));

                    betAmount = std::stoi(amountStr);
                    totalMines = std::stoi(minesStr);

                    if (betAmount <= 0 || totalMines < 0 || totalMines >= GRID_SIZE * GRID_SIZE) {
                        MessageBox(hwnd, "Invalid bet amount or mines count.", "Error", MB_OK);
                        break;
                    }

                    int currentBalance = LoadCoins(username);
                    if (betAmount > currentBalance) {
                        MessageBox(hwnd, "Not enough balance.", "Error", MB_OK);
                        break;
                    }

                    // Deduct the bet amount immediately
                    currentBalance -= betAmount;
                    SaveCoins(username, currentBalance);

                    ResetGame();
                    PlaceMines(totalMines);
                    gameActive = true;

                    // Change button text to WITHDRAW
                    SetWindowText(hButtonBet, "WITHDRAW");

                    InvalidateRect(hwnd, NULL, TRUE);
                }
            }
            else if (LOWORD(wParam) == ID_BUTTONEXIT) {
                LaunchExecutable(hwnd, "Gambling\\Gambling.exe", "Gambling.exe");
                DestroyWindow(hwnd);
            }

            if (HIWORD(wParam) == EN_SETFOCUS) {
                HWND focused = (HWND)lParam;
                if (focused == hEditAmount && isPlaceholder1) {
                    SetWindowText(hEditAmount, "");
                    isPlaceholder1 = false;
                } else if (focused == hEditMines && isPlaceholder2) {
                    SetWindowText(hEditMines, "");
                    isPlaceholder2 = false;
                }
            } else if (HIWORD(wParam) == EN_KILLFOCUS) {
                char text[100];
                HWND unfocused = (HWND)lParam;

                if (unfocused == hEditAmount && GetWindowTextLength(hEditAmount) == 0) {
                    SetWindowText(hEditAmount, placeholder1);
                    isPlaceholder1 = true;
                } else if (unfocused == hEditMines && GetWindowTextLength(hEditMines) == 0) {
                    SetWindowText(hEditMines, placeholder2);
                    isPlaceholder2 = true;
                }
            }
            break;

        case WM_LBUTTONDOWN: {
            if (!gameActive) break;
            int clickX = LOWORD(lParam);
            int clickY = HIWORD(lParam);
            
            clickX -= gridOffsetX;
            clickY -= gridOffsetY;
            
            if (clickX < 0 || clickY < 0) break;
            
            int x = clickX / TILE_SIZE;
            int y = clickY / TILE_SIZE;
            
            if (x >= 0 && x < GRID_SIZE && y >= 0 && y < GRID_SIZE) {
                HandleTileClick(hwnd, x, y);
            }
        
        break;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            DrawGrid(hdc);

            int coins = LoadCoins(username);

            SetTextColor(hdc, RGB(255, 255, 255));
            SetBkMode(hdc, TRANSPARENT);

            SIZE textSize;
            std::string balanceText = std::to_string(coins) + "$";
            GetTextExtentPoint32(hdc, balanceText.c_str(), balanceText.length(), &textSize);

            std::string message = "MineSweep";
            GetTextExtentPoint32(hdc, message.c_str(), message.length(), &textSize);

            int x = (960 - textSize.cx) / 2;
            int y = 50;

            TextOut(hdc, x, 410, balanceText.c_str(), balanceText.length());
            TextOut(hdc, x, y, message.c_str(), message.length());

            EndPaint(hwnd, &ps);
            break;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}