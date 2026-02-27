#include "win32.hpp"
#include "../../platform/platform.hpp"
#include "../../core/types.hpp"
#include "../../core/string.hpp"
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

    HWND viewport_hwnd = nullptr;
    HWND left_panel_hwnd = nullptr;
    HWND right_panel_hwnd = nullptr;
    bool editor_mode = true;

    constexpr int LEFT_PANEL_WIDTH = 200;
    constexpr int RIGHT_PANEL_WIDTH = 250;

    constexpr int PANEL_ID_LEFT = 0;
    constexpr int PANEL_ID_RIGHT = 1;

    char fps_text[64] = "FPS: ---";
    char frametime_text[64] = "Frame: --- ms";

    HMENU menu_bar = nullptr;

    constexpr int IDM_FILE_SAVE    = 40001;
    constexpr int IDM_FILE_SAVE_AS = 40002;
    constexpr int IDM_FILE_LOAD    = 40003;

    // Callback for menu actions — set by app layer
    using MenuCallback = void (*)(int action);
    MenuCallback menu_callback = nullptr;

    const arr::Array<file::FileEntry>* asset_entries = nullptr;
    int asset_scroll_offset = 0;
    constexpr int ASSET_ITEM_HEIGHT = 18;
    constexpr int ASSET_INDENT = 12;

    using AssetCallback = void (*)(const char* path);
    AssetCallback asset_callback = nullptr;

    int asset_hovered_index = -1;
    int asset_selected_index = -1;
    bool asset_tracking_mouse = false;

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
    case VK_F5:      return platform::KEY_F5;
    default:         return platform::KEY_COUNT; // ignore
    }
}

static void sample_mouse() {
    HWND target = viewport_hwnd ? viewport_hwnd : hWnd;
    if (!mouse_captured || !target) {
        mouse_dx = 0.0f;
        mouse_dy = 0.0f;
        return;
    }

    RECT client;
    GetClientRect(target, &client);
    POINT centre = {
        (client.right - client.left) / 2,
        (client.bottom - client.top) / 2
    };

    POINT cursor;
    GetCursorPos(&cursor);
    ScreenToClient(target, &cursor);

    mouse_dx = (f32)(cursor.x - centre.x);
    mouse_dy = (f32)(cursor.y - centre.y);
    POINT screen_centre = centre;
    ClientToScreen(target, &screen_centre);
    SetCursorPos(screen_centre.x, screen_centre.y);
}

static bool register_window_class(HINSTANCE hInst);
static bool create_main_window(HINSTANCE hInst, int nCmdShow);
static LRESULT CALLBACK window_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK viewport_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK panel_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK about_dialog_proc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
static void editor_layout();

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

    void* get_native_window_handle() {
        HWND target = viewport_hwnd ? viewport_hwnd : hWnd;
        return static_cast<void*>(target);
    }

    void get_paint_field_size(u32* width, u32* height) {
        HWND target = viewport_hwnd ? viewport_hwnd : hWnd;
        RECT rect;
        GetClientRect(target, &rect);
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
            HWND target = viewport_hwnd ? viewport_hwnd : hWnd;
            RECT client;
            GetClientRect(target, &client);
            POINT centre = {
                (client.right - client.left) / 2,
                (client.bottom - client.top) / 2
            };
            ClientToScreen(target, &centre);
            SetCursorPos(centre.x, centre.y);
        }
        else {
            mouse_dx = 0.0f;
            mouse_dy = 0.0f;
        }
    }

    bool is_mouse_captured() {
        return mouse_captured;
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
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(hInst, MAKEINTRESOURCE(IDI_SMALL));
    return RegisterClassExW(&wcex) != 0;
}

static bool create_main_window(HINSTANCE hInst, int nCmdShow) {
    hWnd = CreateWindowW(
        szWindowClass, szTitle,
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
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

    case WM_COMMAND:
        if (HIWORD(wParam) == 0) {
            int id = LOWORD(wParam);
            if (menu_callback && (id == IDM_FILE_SAVE || id == IDM_FILE_SAVE_AS || id == IDM_FILE_LOAD)) {
                menu_callback(id);
            }
        }
        return 0;

    case WM_SIZE:
        if (viewport_hwnd) editor_layout();
        return 0;

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

static LRESULT CALLBACK viewport_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_LBUTTONDOWN:
        if (!mouse_captured) {
            platform::set_mouse_captured(true);
        }
        return 0;

    default:
        return DefWindowProcA(hwnd, message, wParam, lParam);
    }
}

static int asset_hit_test(HWND hwnd, int mouse_y) {
    if (!asset_entries || asset_entries->count == 0) return -1;
    RECT rc;
    GetClientRect(hwnd, &rc);
    int mid_y = (rc.bottom - rc.top) / 2;
    int list_top = mid_y + 28;
    if (mouse_y < list_top) return -1;
    int index = (mouse_y - list_top) / ASSET_ITEM_HEIGHT + asset_scroll_offset;
    if (index < 0 || index >= (int)asset_entries->count) return -1;
    if (!asset_entries->data[index].is_file) return -1;
    return index;
}

static LRESULT CALLBACK panel_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rc;
        GetClientRect(hwnd, &rc);

        HBRUSH bg = CreateSolidBrush(RGB(45, 45, 45));
        FillRect(hdc, &rc, bg);
        DeleteObject(bg);

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(200, 200, 200));

        HFONT font = CreateFontA(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
        HFONT old_font = (HFONT)SelectObject(hdc, font);

        int panel_id = (int)GetWindowLongPtrA(hwnd, GWLP_USERDATA);

        if (panel_id == PANEL_ID_LEFT) {
            RECT section = { 8, 8, rc.right - 8, 24 };
            DrawTextA(hdc, "Entities", -1, &section, DT_LEFT | DT_SINGLELINE);

            int mid_y = (rc.bottom - rc.top) / 2;
            HPEN pen = CreatePen(PS_SOLID, 1, RGB(80, 80, 80));
            HPEN old_pen = (HPEN)SelectObject(hdc, pen);
            MoveToEx(hdc, 4, mid_y, nullptr);
            LineTo(hdc, rc.right - 4, mid_y);
            SelectObject(hdc, old_pen);
            DeleteObject(pen);

            RECT assets_section = { 8, mid_y + 8, rc.right - 8, mid_y + 24 };
            DrawTextA(hdc, "Assets", -1, &assets_section, DT_LEFT | DT_SINGLELINE);

            if (asset_entries && asset_entries->count > 0) {
                int list_top = mid_y + 28;
                int list_bottom = rc.bottom;
                int visible_count = (list_bottom - list_top) / ASSET_ITEM_HEIGHT;

                for (int i = 0; i < visible_count && (asset_scroll_offset + i) < (int)asset_entries->count; i++) {
                    int abs_index = asset_scroll_offset + i;
                    const file::FileEntry& e = asset_entries->data[abs_index];
                    int y = list_top + i * ASSET_ITEM_HEIGHT;
                    int x = 8 + (int)e.depth * ASSET_INDENT;

                    if (e.is_file) {
                        RECT row_rect = { 0, y, rc.right, y + ASSET_ITEM_HEIGHT };
                        if (abs_index == asset_selected_index) {
                            HBRUSH sel = CreateSolidBrush(RGB(60, 80, 120));
                            FillRect(hdc, &row_rect, sel);
                            DeleteObject(sel);
                        } else if (abs_index == asset_hovered_index) {
                            HBRUSH hov = CreateSolidBrush(RGB(60, 60, 60));
                            FillRect(hdc, &row_rect, hov);
                            DeleteObject(hov);
                        }
                        SetTextColor(hdc, RGB(180, 180, 180));
                    } else {
                        SetTextColor(hdc, RGB(220, 200, 120));
                    }

                    RECT item_rect = { x, y, rc.right - 4, y + ASSET_ITEM_HEIGHT };
                    DrawTextA(hdc, e.name, -1, &item_rect, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
                }
                SetTextColor(hdc, RGB(200, 200, 200));
            }
        } else if (panel_id == PANEL_ID_RIGHT) {
            RECT fps_rect = { 8, 8, rc.right - 8, 24 };
            DrawTextA(hdc, fps_text, -1, &fps_rect, DT_LEFT | DT_SINGLELINE);

            RECT ft_rect = { 8, 26, rc.right - 8, 42 };
            DrawTextA(hdc, frametime_text, -1, &ft_rect, DT_LEFT | DT_SINGLELINE);

            HPEN pen = CreatePen(PS_SOLID, 1, RGB(80, 80, 80));
            HPEN old_pen = (HPEN)SelectObject(hdc, pen);
            MoveToEx(hdc, 4, 50, nullptr);
            LineTo(hdc, rc.right - 4, 50);
            SelectObject(hdc, old_pen);
            DeleteObject(pen);

            RECT prop_rect = { 8, 58, rc.right - 8, 74 };
            DrawTextA(hdc, "Properties", -1, &prop_rect, DT_LEFT | DT_SINGLELINE);
        }

        SelectObject(hdc, old_font);
        DeleteObject(font);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_MOUSEMOVE: {
        int panel_id = (int)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
        if (panel_id == PANEL_ID_LEFT) {
            if (!asset_tracking_mouse) {
                TRACKMOUSEEVENT tme = {};
                tme.cbSize = sizeof(tme);
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = hwnd;
                TrackMouseEvent(&tme);
                asset_tracking_mouse = true;
            }
            int mouse_y = (int)(short)HIWORD(lParam);
            int new_hover = asset_hit_test(hwnd, mouse_y);
            if (new_hover != asset_hovered_index) {
                asset_hovered_index = new_hover;
                InvalidateRect(hwnd, nullptr, FALSE);
            }
        }
        return 0;
    }

    case WM_MOUSELEAVE: {
        asset_tracking_mouse = false;
        if (asset_hovered_index != -1) {
            asset_hovered_index = -1;
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;
    }

    case WM_LBUTTONDOWN: {
        int panel_id = (int)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
        if (panel_id == PANEL_ID_LEFT) {
            int mouse_y = (int)(short)HIWORD(lParam);
            int index = asset_hit_test(hwnd, mouse_y);
            if (index != asset_selected_index) {
                asset_selected_index = index;
                InvalidateRect(hwnd, nullptr, FALSE);
            }
        }
        return 0;
    }

    case WM_LBUTTONDBLCLK: {
        int panel_id = (int)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
        if (panel_id == PANEL_ID_LEFT && asset_callback) {
            int mouse_y = (int)(short)HIWORD(lParam);
            int index = asset_hit_test(hwnd, mouse_y);
            if (index >= 0) {
                const file::FileEntry& e = asset_entries->data[index];
                if (str::ends_with(e.name, ".gltf")) {
                    asset_callback(e.path);
                }
            }
        }
        return 0;
    }

    case WM_MOUSEWHEEL: {
        int panel_id = (int)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
        if (panel_id == PANEL_ID_LEFT && asset_entries && asset_entries->count > 0) {
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            int scroll_lines = delta / 120;
            asset_scroll_offset -= scroll_lines * 3;
            if (asset_scroll_offset < 0) asset_scroll_offset = 0;
            int max_offset = (int)asset_entries->count - 1;
            if (max_offset < 0) max_offset = 0;
            if (asset_scroll_offset > max_offset) asset_scroll_offset = max_offset;
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;
    }

    case WM_ERASEBKGND:
        return 1;

    default:
        return DefWindowProcA(hwnd, message, wParam, lParam);
    }
}

static void editor_layout() {
    RECT rc;
    GetClientRect(hWnd, &rc);
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;

    if (editor_mode) {
        int vp_x = LEFT_PANEL_WIDTH;
        int vp_w = w - LEFT_PANEL_WIDTH - RIGHT_PANEL_WIDTH;
        if (vp_w < 1) vp_w = 1;

        MoveWindow(left_panel_hwnd, 0, 0, LEFT_PANEL_WIDTH, h, TRUE);
        MoveWindow(right_panel_hwnd, w - RIGHT_PANEL_WIDTH, 0, RIGHT_PANEL_WIDTH, h, TRUE);
        MoveWindow(viewport_hwnd, vp_x, 0, vp_w, h, TRUE);

        ShowWindow(left_panel_hwnd, SW_SHOW);
        ShowWindow(right_panel_hwnd, SW_SHOW);
    } else {
        ShowWindow(left_panel_hwnd, SW_HIDE);
        ShowWindow(right_panel_hwnd, SW_HIDE);
        MoveWindow(viewport_hwnd, 0, 0, w, h, TRUE);
    }
}

namespace platform {

    void editor_init() {
        WNDCLASSEXA panel_class = {};
        panel_class.cbSize = sizeof(WNDCLASSEXA);
        panel_class.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        panel_class.lpfnWndProc = panel_proc;
        panel_class.hInstance = hInstance;
        panel_class.hCursor = LoadCursor(nullptr, IDC_ARROW);
        panel_class.lpszClassName = "GathaPanel";
        RegisterClassExA(&panel_class);

        WNDCLASSEXA vp_class = {};
        vp_class.cbSize = sizeof(WNDCLASSEXA);
        vp_class.style = CS_OWNDC;
        vp_class.lpfnWndProc = viewport_proc;
        vp_class.hInstance = hInstance;
        vp_class.hCursor = LoadCursor(nullptr, IDC_ARROW);
        vp_class.lpszClassName = "GathaViewport";
        RegisterClassExA(&vp_class);

        viewport_hwnd = CreateWindowExA(0, "GathaViewport", nullptr,
            WS_CHILD | WS_VISIBLE, 0, 0, 100, 100,
            hWnd, nullptr, hInstance, nullptr);

        left_panel_hwnd = CreateWindowExA(0, "GathaPanel", nullptr,
            WS_CHILD | WS_VISIBLE, 0, 0, LEFT_PANEL_WIDTH, 100,
            hWnd, nullptr, hInstance, nullptr);
        SetWindowLongPtrA(left_panel_hwnd, GWLP_USERDATA, PANEL_ID_LEFT);

        right_panel_hwnd = CreateWindowExA(0, "GathaPanel", nullptr,
            WS_CHILD | WS_VISIBLE, 0, 0, RIGHT_PANEL_WIDTH, 100,
            hWnd, nullptr, hInstance, nullptr);
        SetWindowLongPtrA(right_panel_hwnd, GWLP_USERDATA, PANEL_ID_RIGHT);

        // Menu bar
        menu_bar = CreateMenu();
        HMENU file_menu = CreatePopupMenu();
        AppendMenuA(file_menu, MF_STRING, IDM_FILE_SAVE, "Save\tCtrl+S");
        AppendMenuA(file_menu, MF_STRING, IDM_FILE_SAVE_AS, "Save As...");
        AppendMenuA(file_menu, MF_STRING, IDM_FILE_LOAD, "Load...");
        AppendMenuA(menu_bar, MF_POPUP, (UINT_PTR)file_menu, "File");
        SetMenu(hWnd, menu_bar);

        editor_layout();
    }

    void editor_toggle() {
        editor_mode = !editor_mode;
        if (editor_mode) {
            SetMenu(hWnd, menu_bar);
        } else {
            SetMenu(hWnd, nullptr);
        }
        editor_layout();
    }

    bool is_editor_mode() {
        return editor_mode;
    }

    void editor_set_menu_callback(void (*callback)(int)) {
        menu_callback = callback;
    }

    bool editor_open_file_dialog(char* out_path, u32 max_len) {
        char file[512] = {};
        OPENFILENAMEA ofn = {};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hWnd;
        ofn.lpstrFilter = "Scene Files (*.json)\0*.json\0All Files (*.*)\0*.*\0";
        ofn.lpstrFile = file;
        ofn.nMaxFile = sizeof(file);
        ofn.lpstrInitialDir = "assets\\scenes";
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
        if (!GetOpenFileNameA(&ofn)) return false;
        str::copy(out_path, file, max_len);
        return true;
    }

    bool editor_save_file_dialog(char* out_path, u32 max_len) {
        char file[512] = {};
        OPENFILENAMEA ofn = {};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hWnd;
        ofn.lpstrFilter = "Scene Files (*.json)\0*.json\0All Files (*.*)\0*.*\0";
        ofn.lpstrFile = file;
        ofn.nMaxFile = sizeof(file);
        ofn.lpstrInitialDir = "assets\\scenes";
        ofn.lpstrDefExt = "json";
        ofn.Flags = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
        if (!GetSaveFileNameA(&ofn)) return false;
        str::copy(out_path, file, max_len);
        return true;
    }

    void editor_set_asset_callback(void (*callback)(const char* path)) {
        asset_callback = callback;
    }

    void editor_set_asset_entries(const arr::Array<file::FileEntry>* entries) {
        asset_entries = entries;
        asset_scroll_offset = 0;
        if (left_panel_hwnd && editor_mode) {
            InvalidateRect(left_panel_hwnd, nullptr, FALSE);
        }
    }

    void editor_set_fps(f32 fps, f32 frame_time_ms) {
        u32 fps_whole = (u32)fps;
        u32 ft_whole = (u32)frame_time_ms;
        u32 ft_frac = (u32)((frame_time_ms - (f32)ft_whole) * 100.0f);
        str::format(fps_text, sizeof(fps_text), "FPS: %u", fps_whole);
        str::format(frametime_text, sizeof(frametime_text), "Frame: %u.%02u ms", ft_whole, ft_frac);
        if (right_panel_hwnd && editor_mode) {
            InvalidateRect(right_panel_hwnd, nullptr, FALSE);
        }
    }

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