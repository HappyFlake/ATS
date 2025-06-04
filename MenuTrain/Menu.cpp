#include <windows.h>
#include <string>
#include <cstdio>
#include <fstream>
#include <gdiplus.h>
#pragma comment (lib,"Gdiplus.lib")
using namespace Gdiplus;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK MsgBoxProc(HWND, UINT, WPARAM, LPARAM);


// Random Key Generator
std::string GenerateRandomKey(int length = 16) {
    const char charset[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    const int charsetSize = sizeof(charset) - 1;

    std::string key;
    for (int i = 0; i < length; ++i) {
        key += charset[rand() % charsetSize];
        if ((i + 1) % 4 == 0 && i + 1 < length) {
            key += ' '; // Insert space every 4 characters
        }
    }
    return key;
}

// Glues The Keys Together When Copying!
struct Keys {
    std::string pubkey;
    std::string privkey;
};

const COLORREF RGBBlue = RGB(32, 44, 68);
static ULONG_PTR g_gdiplusToken = 0;

#define ID_BUTTON 1
#define ID_EDIT1  2
#define ID_EDIT2  3
#define ID_TOGGLE 4
#define ID_BUTTON2  5

const char placeholder1[] = "Public Key";
const char placeholder2[] = "Private Key";
const char placeholder3[] = "Exit";

bool isSignUpMode = true; // Toggle Login or signup.

void SaveCurrentUser(const char* Publickey, int coins = 0) {
    std::ofstream file("current_user.txt");
    if (file.is_open()) {
        file << Publickey << ":" << coins;
        file.close();
    }
}


//Check if username is taken in Users.txt
bool DoesUsernameExist(const char* Publickey) {
    std::ifstream file("users.txt");
    std::string line;
    std::string userInput = std::string(Publickey) + ":";

    if (file.is_open()) {
        while (std::getline(file, line)) {
            if (line.find(userInput) == 0) {
                file.close();
                return true;
            }
        }
        file.close();
    }
    return false;
}

// Check PublicKey & PrivateKey if they correct.
int ValidateUser(const char* Publickey, const char* Privatekey) {
    std::ifstream file("users.txt");
    std::string line, userPass = std::string(Publickey) + ":" + Privatekey;

    bool userExists = false;
    if (file.is_open()) {
        while (std::getline(file, line)) {
            if (line.find(Publickey) == 0) {
                userExists = true;
                if (line == userPass) {
                    file.close();
                    return 1; //Valid credentials
                }
            }
        }
        file.close();
    }
    return userExists ? -1 : 0; // -1: Wrong Privatekey, 0: Noneexistent publickey.
}

// Add new user credentials
bool SaveUser(const char* Publickey, const char* Privatekey) {
    if (DoesUsernameExist(Publickey)) {
        return false;
    }

    std::ofstream file("users.txt", std::ios::app);
    if (file.is_open()) {
        file << Publickey << ":" << Privatekey << "\n";
        file.close();
        return true;
    }
    return false;
}

std::pair<std::string, int> GetCurrentUserWithCoins() {
    std::ifstream file("current_user.txt");
    std::string line;
    if (file.is_open()) {
        std::getline(file, line);
        file.close();
    }

    size_t colonPos = line.find(':');
    if (colonPos == std::string::npos) {
        return { "Guest", 0 }; // Default fallback if file is corrupted
    }

    std::string username = line.substr(0, colonPos);
    int coins = std::stoi(line.substr(colonPos + 1));
    return { username, coins };
}


void LaunchMessageApp(HWND hwnd) {
    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi;

    // Change this to the absolute path to Message.exe
    const char* messageAppPath = "Message\\Message.exe";

    if (CreateProcess(messageAppPath, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}




int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int nCmdShow) {
    // Generate Key Popup
    WNDCLASS msgboxClass = {};
    msgboxClass.lpfnWndProc = MsgBoxProc;
    msgboxClass.hInstance = hInstance;
    msgboxClass.lpszClassName = "CustomMsgBox";
    msgboxClass.hbrBackground = CreateSolidBrush(RGB(32, 44, 68)); // Light background

    RegisterClass(&msgboxClass);
    const char CLASS_NAME[] = "Sample Window";


    HBRUSH hBlueBrush = CreateSolidBrush(RGBBlue);

    // Main Window
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = hBlueBrush;

    // Displays the IMAGE!!!
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, NULL);
    
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

    HWND hwnd = CreateWindowEx(0, CLASS_NAME, "Login", 
        WS_POPUP | WS_SYSMENU,
        xPos, yPos, windowWidth, windowHeight,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) return 0;

    srand(static_cast<unsigned int>(time(NULL)));

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg = {};
    while(GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

// Generate Key Popup!
LRESULT CALLBACK MsgBoxProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static Keys* keys = nullptr;

    switch (msg) {
        case WM_CREATE: {
            CREATESTRUCT* pcs = (CREATESTRUCT*)lParam;
            keys = (Keys*)pcs->lpCreateParams;

            std::string message = 
                "DO NOT SAVE THE KEYS ON YOUR SYSTEM OR ANY OTHER DEVICE\n"
                "WRITE THEM DOWN ON A PAPER AND STORE IT SECURELY\n\n"
                "Your Publickey is: " + keys->pubkey + "\nYour Privatekey is: " + keys->privkey;

            CreateWindow("Static",
                         message.c_str(),
                         WS_CHILD | WS_VISIBLE | SS_CENTER,
                         0, 0, 480, 270,
                         hwnd, NULL, NULL, NULL);

            CreateWindow("Button", "OK",
                         WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                         190, 100, 100, 30,
                         hwnd, (HMENU)1, NULL, NULL);
            return 0;
        }
        case WM_COMMAND:
            if (LOWORD(wParam) == 1) {
                DestroyWindow(hwnd);
            }
            return 0;

        case WM_DESTROY:
            if (keys) {
                delete keys;
                keys = nullptr;
            }
            return 0;

        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND hButton, hButton2, hEdit1, hEdit2, hToggle;
    static bool isPlaceholder1 = true, isPlaceholder2 = true;

    switch (msg) {
        case WM_CREATE:
            hEdit1 = NULL;
            hEdit2 = NULL;

            hButton = CreateWindowEx(0, "Button", "Generate", WS_CHILD | WS_VISIBLE,
                380, 375, 200, 20, hwnd, (HMENU)ID_BUTTON, NULL, NULL);

            hToggle = CreateWindowEx(0, "Button", "Already have Keys?", WS_CHILD | WS_VISIBLE,
                380, 400, 200, 20, hwnd, (HMENU)ID_TOGGLE, NULL, NULL);

            hButton2 = CreateWindowEx(0, "Button", "Exit", WS_CHILD | WS_VISIBLE | WS_BORDER,
                              430, 425, 100, 20, hwnd, (HMENU)ID_BUTTON2, NULL, NULL);

            SetWindowText(hButton, "Generate");
            SetWindowText(hToggle, "Already have Keys?");

            // In sign-up mode, do not show the login fields
            if (hEdit1) {
                DestroyWindow(hEdit1);
                hEdit1 = NULL;
            }

            if (hEdit2) {
                DestroyWindow(hEdit2);
                hEdit2 = NULL;
            }
        return 0;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_BUTTON:
                    if (isSignUpMode) {
                        std::string pubKey = GenerateRandomKey();
                        std::string privKey = GenerateRandomKey();

                    // Save to users.txt
                    if (SaveUser(pubKey.c_str(), privKey.c_str())) {
                        SaveCurrentUser(pubKey.c_str(), 0); // Save as current user (starting with 0 coins)

                    std::string clipboardText = "Publickey: " + pubKey + "\r\nPrivatekey: " + privKey;

                    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, clipboardText.size() + 1);
                    if (hMem) {
                        memcpy(GlobalLock(hMem), clipboardText.c_str(), clipboardText.size() + 1);
                        GlobalUnlock(hMem);
                        OpenClipboard(hwnd);
                        SetClipboardData(CF_TEXT, hMem);
                        CloseClipboard();
                    }

                    SetWindowText(hButton, "Copied");

                    // Get screen width & height
                    RECT rect;
                    SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);
                    int msgscreenWidth = rect.right;
                    int msgscreenHeight = rect.bottom;

                    // Window width & height
                    int msgwindowWidth = 480;
                    int msgwindowHeight = 150;

                    // Calculate where the center is for the window
                    int msgxPos = (msgscreenWidth - msgwindowWidth) / 2;
                    int msgyPos = (msgscreenHeight - msgwindowHeight) / 2;
                    // Prepare keys for the popup
                    Keys* keys = new Keys{ pubKey, privKey };

                    // Create popup and pass keys pointer
                    HWND msgHwnd = CreateWindowEx(
                        WS_EX_DLGMODALFRAME, "CustomMsgBox", "IMPORTANT",
                        WS_POPUP | WS_SYSMENU,
                        msgxPos, msgyPos, msgwindowWidth, msgwindowHeight,
                        hwnd, NULL, GetModuleHandle(NULL), keys);

                    ShowWindow(msgHwnd, SW_SHOW);
                    UpdateWindow(msgHwnd);

                    // Automatically switch to login mode
                    isSignUpMode = false;
                    SetWindowText(hButton, "Login");
                    SetWindowText(hToggle, "Back to Generate");

                    // Create the login input fields
                    if (!hEdit1) {
                        hEdit1 = CreateWindow("Edit", placeholder1, WS_CHILD | WS_VISIBLE | WS_BORDER,
                                                380, 325, 200, 20, hwnd, (HMENU)ID_EDIT1, NULL, NULL);
                        isPlaceholder1 = true;
                    }

                    if (!hEdit2) {
                        hEdit2 = CreateWindow("Edit", placeholder2, WS_CHILD | WS_VISIBLE | WS_BORDER,
                                                380, 350, 200, 20, hwnd, (HMENU)ID_EDIT2, NULL, NULL);
                        isPlaceholder2 = true;
                    }
                }
            } else {
                // Login logic
                char pubInput[100], privInput[100];
                GetWindowText(hEdit1, pubInput, sizeof(pubInput));
                GetWindowText(hEdit2, privInput, sizeof(privInput));

                if (strcmp(pubInput, placeholder1) == 0 || strcmp(privInput, placeholder2) == 0 ||
                    strlen(pubInput) == 0 || strlen(privInput) == 0) {
                    MessageBox(hwnd, "Please enter both keys.", "Missing Info", MB_OK);
                    return 0;
                }

                int result = ValidateUser(pubInput, privInput);
                if (result == 1) {
                    SaveCurrentUser(pubInput); // Valid login
                    LaunchMessageApp(hwnd); // Open external app
                    DestroyWindow(hwnd);
                } else if (result == -1) {
                    MessageBox(hwnd, "Invalid private key.", "Login Failed", MB_OK | MB_ICONERROR);
                } else {
                    MessageBox(hwnd, "Public key not found.", "Login Failed", MB_OK | MB_ICONERROR);
                }
            }
            return 0;

                case ID_TOGGLE:
                    isSignUpMode = !isSignUpMode;

                    if (!isSignUpMode) {
                        SetWindowText(hButton, "Login");
                        SetWindowText(hToggle, "Back to Generate");

                        if (!hEdit1) {
                            hEdit1 = CreateWindow("Edit", placeholder1, WS_CHILD | WS_VISIBLE | WS_BORDER,
                                                    380, 325, 200, 20, hwnd, (HMENU)ID_EDIT1, NULL, NULL);
                            isPlaceholder1 = true;
                        }

                        if (!hEdit2) {
                            hEdit2 = CreateWindow("Edit", placeholder2, WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                                                    380, 350, 200, 20, hwnd, (HMENU)ID_EDIT2, NULL, NULL);
                            isPlaceholder2 = true;
                        }
                    } else {
                        SetWindowText(hButton, "Generate");
                        SetWindowText(hToggle, "Already have Keys?");

                        if (hEdit1) {
                            DestroyWindow(hEdit1);
                            hEdit1 = NULL;
                            isPlaceholder1 = true;
                        }

                        if (hEdit2) {
                            DestroyWindow(hEdit2);
                            hEdit2 = NULL;
                            isPlaceholder2 = true;
                        }

                        

                    InvalidateRect(hwnd, NULL, TRUE);
                    return 0;
            }

            // Clears input bar when clicked/ in use!
            if (HIWORD(wParam) == EN_SETFOCUS) {
                HWND focused = (HWND)lParam;
                if (focused == hEdit1 && isPlaceholder1) {
                    SetWindowText(hEdit1, "");
                    isPlaceholder1 = false;
                } else if (focused == hEdit2 && isPlaceholder2) {
                    SetWindowText(hEdit2, "");
                    isPlaceholder2 = false;
                }
            } else if (HIWORD(wParam) == EN_KILLFOCUS) {
                char text[100];
                HWND unfocused = (HWND)lParam;
                if (unfocused == hEdit1 && GetWindowTextLength(hEdit1) == 0) {
                    SetWindowText(hEdit1, placeholder1);
                    isPlaceholder1 = true;
                } else if (unfocused == hEdit2 && GetWindowTextLength(hEdit2) == 0) {
                    SetWindowText(hEdit2, placeholder2);
                    isPlaceholder2 = true;
                }
        }

        return 0;
        

        case ID_BUTTON2:
            DestroyWindow(hwnd);
            return 0;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            {
                Graphics graphics(hdc);
                Image image(L"Symbool.png"); // Ensure the image is in the correct path
                graphics.DrawImage(&image, 350, 20, 256, 128);
            }
            
            SetTextColor(hdc, RGB(255, 255, 255));
            SetBkMode(hdc, TRANSPARENT);
            TextOut(hdc, 430, 130, "VirtualGrass V3", 16);
            
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_DESTROY:
            GdiplusShutdown(g_gdiplusToken);
            PostQuitMessage(0);
            return 0;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
