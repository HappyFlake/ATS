#include <windows.h>
#include <string>
#include <fstream>
#include <TlHelp32.h>

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

#define ID_BUTTON 1
#define ID_TOGGLE 2 
#define ID_EDIT1  3
#define ID_EDIT2  4

// Default placeholders   
const char placeholder1[] = "Tree Puzzle"; // Tree Puzzle, 25 x 25.       
const char placeholder2[] = "Back";

std::string GetLastLoggedInUser() {
    std::ifstream file("current_user.txt");
    std::string username;
    if (file.is_open()) {
        std::getline(file, username);
        file.close();
    }
    return username.empty() ? "Guest" : username;
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

    // Closing open window
    CloseExistingProcess(processName);

    // Open a new window
    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi;

    if (CreateProcess(fullPath, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    } else {
        MessageBox(hwnd, "Failed to open the application!", "Error", MB_OK);
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
    static std::string username = GetLastLoggedInUser();

    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            SetTextColor(hdc, RGB(255, 255, 255));
            SetBkMode(hdc, TRANSPARENT);

            SIZE textSize;
            std::string message = "Pick a Game " + username + "!";
            GetTextExtentPoint32(hdc, message.c_str(), message.length(), &textSize);

            int x = (960 - textSize.cx) / 2;  
            int y = 50;  
            TextOut(hdc, x, y, message.c_str(), message.length());

            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_CREATE:
            // Tree
            hEdit3 = CreateWindowEx(0, "Button", placeholder1, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                380, 400, 200, 20, hwnd, (HMENU)ID_EDIT1, NULL, NULL);

            // Back
            hEdit4 = CreateWindowEx(0, "Button", placeholder2, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                430, 425, 100, 20, hwnd, (HMENU)ID_EDIT2, NULL, NULL);
            return 0;

        case WM_COMMAND:
            if (LOWORD(wParam) == ID_EDIT1) {
                LaunchExecutable(hwnd, "Play\\Games\\Tree\\Tree.exe", "Tree.exe");
                DestroyWindow(hwnd);
            } else if (LOWORD(wParam) == ID_EDIT2) {
                LaunchExecutable(hwnd, "Message\\Message.exe", "Message.exe");
                DestroyWindow(hwnd);
            } 
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}
