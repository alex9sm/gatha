#include "win32.hpp"
#include "../../platform/platform.hpp"
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
    LARGE_INTEGER perf_freq = {};
    LARGE_INTEGER frame_start = {};
    f32           delta_time = 0.0f;   
    bool key_state[platform::KEY_COUNT] = {};
    f32  mouse_dx = 0.0f;
    f32  mouse_dy = 0.0f;
    bool mouse_captured = false;

}

static platform::Key vk_to_key(WPARAM vk) {
    switch (vk) {
    case 'W':        return platform::KEY_W;
    case 'A':        return platform::KEY_A;
    case 'S':        return platform::KEY_S;
    case 'D':        return platform::KEY_D;
    case VK_CONTROL: return platform::KEY_CTRL;
    case VK_SPACE:   return platform::KEY_SPACE;
    case VK_SHIFT:   return platform::KEY_SHIFT;
    case VK_ESCAPE:  return platform::KEY_ESCAPE;
    default:         return platform::KEY_COUNT; // ignore
    }
}

static void sample_mouse() {
    if (!mouse_captured || !hWnd) {
        mouse_dx = 0.0f;
        mouse_dy = 0.0f;
        return;
    }

    RECT client;
    GetClientRect(hWnd, &client);
    POINT centre = {
        (client.right - client.left) / 2,
        (client.bottom - client.top) / 2
    };

    POINT cursor;
    GetCursorPos(&cursor);
    ScreenToClient(hWnd, &cursor);

    mouse_dx = (f32)(cursor.x - centre.x);
    mouse_dy = (f32)(cursor.y - centre.y);
    POINT screen_centre = centre;
    ClientToScreen(hWnd, &screen_centre);
    SetCursorPos(screen_centre.x, screen_centre.y);
}

static bool register_window_class(HINSTANCE hInst);
static bool create_main_window(HINSTANCE hInst, int nCmdShow);
static LRESULT CALLBACK window_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK about_dialog_proc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

namespace platform {
    namespace internal {

        bool init(HINSTANCE hInst, int nCmdShow) {
            ::hInstance = hInst;

            LoadStringW(hInst, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
            LoadStringW(hInst, IDC_GATHA, szWindowClass, MAX_LOADSTRING);

            if (!register_window_class(hInst)) return false;
            if (!create_main_window(hInst, nCmdShow)) return false;

            hAccelTable = LoadAccelerators(hInst, MAKEINTRESOURCE(IDC_GATHA));
            QueryPerformanceFrequency(&perf_freq);
            QueryPerformanceCounter(&frame_start);

            running = true;
            return true;
        }

        void process_messages() {
            LARGE_INTEGER now;
            QueryPerformanceCounter(&now);

            delta_time = (f32)(now.QuadPart - frame_start.QuadPart)
                / (f32)perf_freq.QuadPart;

            if (delta_time > 0.1f) delta_time = 0.1f;

            frame_start = now;

            sample_mouse();

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
            if (::hInstance) {
                UnregisterClass(szWindowClass, ::hInstance);
            }
            running = false;
        }

        HWND get_hwnd() { return hWnd; }

    }

    int run() {
        if (!init()) {
            internal::shutdown();
            return -1;
        }

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

    bool is_running() { return running; }

    void* get_native_window_handle() { return static_cast<void*>(hWnd); }

    void get_paint_field_size(u32* width, u32* height) {
        RECT rect;
        GetClientRect(hWnd, &rect);
        *width = static_cast<u32>(rect.right - rect.left);
        *height = static_cast<u32>(rect.bottom - rect.top);
    }

    f32 get_delta_time() { return delta_time; }

    bool is_key_down(Key key) {
        if (key >= KEY_COUNT) return false;
        return key_state[key];
    }

    void get_mouse_delta(f32* out_dx, f32* out_dy) {
        *out_dx = mouse_dx;
        *out_dy = mouse_dy;
    }

    void set_mouse_captured(bool captured) {
        if (mouse_captured == captured) return;
        mouse_captured = captured;
        ShowCursor(!captured);

        if (captured) {
            RECT client;
            GetClientRect(hWnd, &client);
            POINT centre = {
                (client.right - client.left) / 2,
                (client.bottom - client.top) / 2
            };
            ClientToScreen(hWnd, &centre);
            SetCursorPos(centre.x, centre.y);
        }
        else {
            mouse_dx = 0.0f;
            mouse_dy = 0.0f;
        }
    }

}

int APIENTRY wWinMain(
    _In_     HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_     LPWSTR    lpCmdLine,
    _In_     int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    if (!platform::internal::init(hInstance, nCmdShow)) return -1;
    return platform::run();
}

static bool register_window_class(HINSTANCE hInst) {
    WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wcex.lpfnWndProc = window_proc;
    wcex.hInstance = hInst;
    wcex.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_GATHA));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_GATHA);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(hInst, MAKEINTRESOURCE(IDI_SMALL));
    return RegisterClassExW(&wcex) != 0;
}

static bool create_main_window(HINSTANCE hInst, int nCmdShow) {
    hWnd = CreateWindowW(
        szWindowClass, szTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0,
        nullptr, nullptr, hInst, nullptr
    );
    if (!hWnd) return false;
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    return true;
}

static LRESULT CALLBACK window_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN: {
        platform::Key k = vk_to_key(wParam);
        if (k < platform::KEY_COUNT) key_state[k] = true;
        return 0;
    }

    case WM_KEYUP:
    case WM_SYSKEYUP: {
        platform::Key k = vk_to_key(wParam);
        if (k < platform::KEY_COUNT) key_state[k] = false;
        return 0;
    }
    case WM_KILLFOCUS: {
        for (int i = 0; i < platform::KEY_COUNT; ++i) key_state[i] = false;
        mouse_dx = 0.0f;
        mouse_dy = 0.0f;
        if (mouse_captured) {
            platform::set_mouse_captured(false);
        }
        return 0;
    }

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