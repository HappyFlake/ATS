#include <windows.h>
#include <string>
#include <fstream>
#include <TlHelp32.h>

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

#define ID_BUTTON 1
#define ID_TOGGLE 2 
#define ID_EDIT1  3
#define ID_EDIT2  4
#define ID_EDIT3  5
#define ID_EDIT4  6

// Default placeholders   
const char placeholder1[] = "Play";       
const char placeholder2[] = "Gambling";   
const char placeholder3[] = "Log out"; 

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
void LaunchExecutable(HWND hwnd, const char* relativePath, const char* processName) {
    char fullPath[MAX_PATH];

    // Convert relative path to full path
    if (GetFullPathName(relativePath, MAX_PATH, fullPath, NULL) == 0) {
        MessageBox(hwnd, "Failed to get full path!", "Error", MB_OK);
        return;
    }

    // Try to close the existing process
    CloseExistingProcess(processName);

    // Use CreateProcess to launch the executable
    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi;

    if (CreateProcess(fullPath, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    } else {
        MessageBox(hwnd, "Failed to open the application!", "Error", MB_OK);
    }
}


void LaunchMessageAPP(HWND hwnd) {   // Fixed: this function should be outside LaunchExecutable
    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    
    // Write where Message.exe and current_user.txt are
    if (!SetCurrentDirectory("Message")) {
        MessageBox(hwnd, "Failed to set working directory to message!", "Error", MB_OK);
        return;
    }

    const char* messageAppPath = "Message\\Message.exe";  // Missing semicolon here!

    if (CreateProcess(messageAppPath, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    } else {
        MessageBox(hwnd, "Failed to open the application!", "Error", MB_OK);
    }
}

void UpdateCoins(const std::string& username, int newCoins) {
    std::ofstream file("current_user.txt");
    if (file.is_open()) {
        file << username << ":" << newCoins;
        file.close();
    }
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int nCmdShow) {
    const char CLASS_NAME[] = "SampleWindow";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = CreateSolidBrush(RGB(32, 44, 68)); 

    RegisterClass(&wc);

    // Get screen width & height
    RECT rect;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);
    int screenWidth = rect.right;
    int screenHeight = rect.bottom;

    // Window width & height
    int windowWidth = 960;
    int windowHeight = 540;

    // Calculate where the center is for the window
    int xPos = (screenWidth - windowWidth) / 2;
    int yPos = (screenHeight - windowHeight) / 2;

    HWND hwnd = CreateWindowEx(0, CLASS_NAME, "Menu", 
        WS_POPUP | WS_SYSMENU,
        xPos, yPos, windowWidth, windowHeight,
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

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND hButton, hEdit1, hEdit2, hEdit3, hEdit4;
    static std::string g_username;
    static int g_coins;

    switch (msg) {
        case WM_CREATE: {
            // Read current user data once at creation time.
            auto userData = GetCurrentUserWithCoins();
            g_username = userData.first;
            g_coins = userData.second;

            // Plat
            hEdit2 = CreateWindowEx(0, "Button", placeholder1, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                380, 375, 200, 20, hwnd, (HMENU)ID_EDIT1, NULL, NULL);

            // Gambling
            hEdit3 = CreateWindowEx(0, "Button", placeholder2, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                380, 400, 200, 20, hwnd, (HMENU)ID_EDIT2, NULL, NULL);

            // Log out
            hEdit4 = CreateWindowEx(0, "Button", placeholder3, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                430, 425, 100, 20, hwnd, (HMENU)ID_EDIT3, NULL, NULL);
            return 0;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            SetTextColor(hdc, RGB(255, 255, 255));
            SetBkMode(hdc, TRANSPARENT);

            std::string message = "Would " + g_username + " please pick an option?\n\n You got: " + std::to_string(g_coins) + "$";

            SIZE textSize;
            GetTextExtentPoint32(hdc, message.c_str(), message.length(), &textSize);

            int x = (960 - textSize.cx) / 2;
            int y = 50;
            TextOut(hdc, x, y, message.c_str(), message.length());

            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_COMMAND:
            if (LOWORD(wParam) == ID_EDIT1) {
                LaunchExecutable(hwnd, "Play\\Play.exe", "Play.exe");
                DestroyWindow(hwnd);
            } 
            else if (LOWORD(wParam) == ID_EDIT2) {
                LaunchExecutable(hwnd, "Gambling\\Gambling.exe", "Gambling.exe");
                DestroyWindow(hwnd);
            } else if (LOWORD(wParam) == ID_EDIT3) {
                LaunchExecutable(hwnd, "Menu.exe", "Menu.exe");
                DestroyWindow(hwnd);
            } 
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}