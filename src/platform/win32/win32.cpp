#include "win32.hpp"
#include "../../core/types.hpp"
#include "../../app/gatha.hpp"

namespace {

    constexpr int MAX_LOADSTRING = 100;
    HINSTANCE hInstance = nullptr;
    HWND      hWnd = nullptr;
    HACCEL    hAccelTable = nullptr;
    bool      running = false;
    WCHAR     szTitle[MAX_LOADSTRING];
    WCHAR     szWindowClass[MAX_LOADSTRING];

}

static bool register_window_class(HINSTANCE hInstance);
static bool create_main_window(HINSTANCE hInstance, int nCmdShow);
static LRESULT CALLBACK window_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK about_dialog_proc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

namespace platform {

    namespace internal {

        bool init(HINSTANCE hInstance, int nCmdShow) {

            hInstance = hInstance;
            LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
            LoadStringW(hInstance, IDC_GATHA, szWindowClass, MAX_LOADSTRING);

            if (!register_window_class(hInstance)) {
                return false;
            }

            if (!create_main_window(hInstance, nCmdShow)) {
                return false;
            }

            hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_GATHA));

            running = true;
            return true;

        }

        void process_messages() {

            MSG msg;

            while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
                if (msg.message == WM_QUIT) {
                    running = false;
                    return;

                }

                if (!TranslateAccelerator(hWnd, hAccelTable, &msg)) {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);

                }
            }
        }

        void shutdown() {

            if (hWnd) {
                DestroyWindow(hWnd);
                hWnd = nullptr;
            }

            if (hInstance) {
                UnregisterClass(szWindowClass, hInstance);
            }

            running = false;

        }

        HWND get_hwnd() {
            return hWnd;
        }

    }

    int run() {

        if (!init()) {
            internal::shutdown();
            return -1;
        }

        // main loop
        while (running) {

            internal::process_messages();
            if (!running) break;
            update();
            render();
        }

        shutdown();
        internal::shutdown();

        return 0;
    }

    bool is_running() {
        return running;
    }

    void* get_native_window_handle() {
        return static_cast<void*>(hWnd);
    }

    void get_paint_field_size(u32* width, u32* height) {
        RECT rect;
        GetClientRect(hWnd, &rect);
        *width = static_cast<u32>(rect.right - rect.left);
        *height = static_cast<u32>(rect.bottom - rect.top);
    }

}

int APIENTRY wWinMain(

    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    if (!platform::internal::init(hInstance, nCmdShow)) {
        return -1;
    }

    return platform::run();

}

static bool register_window_class(HINSTANCE hInstance) {

    WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = window_proc;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GATHA));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_GATHA);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex) != 0;
}

static bool create_main_window(HINSTANCE hInstance, int nCmdShow) {

    hWnd = CreateWindowW(
        szWindowClass,
        szTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0,
        CW_USEDEFAULT, 0,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (!hWnd) {
        return false;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return true;

}

static LRESULT CALLBACK window_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {

    switch (message) {
    /*
    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        switch (wmId) {
        case IDM_ABOUT:
            DialogBox(hInstance, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, about_dialog_proc);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
    }*/

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        (void)hdc;
        EndPaint(hWnd, &ps);
        break;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

static INT_PTR CALLBACK about_dialog_proc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    UNREFERENCED_PARAMETER(lParam);

    switch (message) {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }

    return (INT_PTR)FALSE;
}