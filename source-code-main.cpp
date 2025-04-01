#include <windows.h>
#include <thread>
#include <cstdlib>
#include <ctime>
#include <cmath>

int pointSize = 10;               // Initial size of the point
const int minPointSize = 5;       // Minimum size of the point
const int maxPointSize = 25;      // Maximum size of the point
const int borderOffset = 150;      // Minimum distance from the screen edges
const int minDelaySeconds = 10;   // Minimum delay before showing the point
const int maxDelaySeconds = 30;  // Maximum delay before showing the point

struct Point {
    int x, y;
};

Point currentPoint = {0, 0};
COLORREF currentColor = RGB(0, 0, 0);

HBITMAP backBuffer = nullptr;  // Backbuffer for smooth rendering
HDC backBufferDC = nullptr;

// Function to initialize the backbuffer
void InitializeBackBuffer(HDC hdc, int width, int height) {
    if (backBuffer) {
        DeleteObject(backBuffer);
        DeleteDC(backBufferDC);
    }

    backBuffer = CreateCompatibleBitmap(hdc, width, height);
    backBufferDC = CreateCompatibleDC(hdc);
    SelectObject(backBufferDC, backBuffer);
}

// Function to draw the point with color inversion
void DrawPoint(HDC hdc, Point point, COLORREF bgColor) {
    // Calculate the inverted color
    COLORREF pointColor = RGB(255 - GetRValue(bgColor), 255 - GetGValue(bgColor), 255 - GetBValue(bgColor));
    HBRUSH brush = CreateSolidBrush(pointColor);
    HBRUSH oldBrush = (HBRUSH)SelectObject(backBufferDC, brush);

    PatBlt(backBufferDC, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), BLACKNESS);
    Ellipse(backBufferDC, point.x, point.y, point.x + pointSize, point.y + pointSize);

    SelectObject(backBufferDC, oldBrush);
    DeleteObject(brush);

    BitBlt(hdc, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), backBufferDC, 0, 0, SRCCOPY);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int GenerateBiasedRandomPosition(int min, int max, int midpoint, double biasStrength) {
    double randValue = (double)std::rand() / RAND_MAX; // Random value between 0 and 1
    double weighted = pow(randValue, biasStrength);   // Apply bias strength
    double biased = randValue < 0.5 ? 1.0 - weighted : weighted; // Mirror bias for both sides
    return min + (int)((max - min) * biased);
}

void UpdateColor(HDC hdc, Point point) {
    // Get the background color of the screen at the current point
    HDC screenDC = GetDC(nullptr);
    COLORREF bgColor = GetPixel(screenDC, point.x + pointSize / 2, point.y + pointSize / 2);
    ReleaseDC(nullptr, screenDC);

    // Check if the color needs to be updated
    if (bgColor != currentColor) {
        currentColor = bgColor;
        DrawPoint(hdc, point, bgColor);
    }
}

void MovePoint(HDC hdc, RECT screenRect, HWND hwnd) {
    std::srand(std::time(nullptr)); // Seed for random direction

    while (true) {
        // Wait for a random delay before showing the point
        int delay = minDelaySeconds + std::rand() % (maxDelaySeconds - minDelaySeconds + 1);
        std::this_thread::sleep_for(std::chrono::seconds(delay));

        // Generate a biased random position
        currentPoint.x = GenerateBiasedRandomPosition(borderOffset, screenRect.right - borderOffset - maxPointSize, screenRect.right / 2, 2.0);
        currentPoint.y = GenerateBiasedRandomPosition(borderOffset, screenRect.bottom - borderOffset - maxPointSize, screenRect.bottom / 2, 2.0);

        // Draw the new point and update its color periodically
        DrawPoint(hdc, currentPoint, currentColor);

        for (int i = 0; i < delay; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Faster update interval
            UpdateColor(hdc, currentPoint);
        }

        // Randomly change the size of the point
        pointSize += (std::rand() % 3 - 1); // -1, 0, or 1
        if (pointSize < minPointSize) pointSize = minPointSize;
        if (pointSize > maxPointSize) pointSize = maxPointSize;
    }
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const char CLASS_NAME[] = "FloatingPointWindow";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT, // Extended styles
        CLASS_NAME,                // Window class
        "Floating Point",         // Window title
        WS_POPUP,                  // Window style (no border, no title bar)
        0, 0,                      // Position
        GetSystemMetrics(SM_CXSCREEN), // Full screen width
        GetSystemMetrics(SM_CYSCREEN), // Full screen height
        nullptr, nullptr, hInstance, nullptr);

    if (!hwnd) {
        return 0;
    }

    // Make the window transparent
    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);

    ShowWindow(hwnd, nCmdShow);

    HDC hdc = GetDC(hwnd);
    RECT screenRect = {0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)};

    InitializeBackBuffer(hdc, screenRect.right, screenRect.bottom);

    std::thread animationThread(MovePoint, hdc, screenRect, hwnd);
    animationThread.detach();

    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
