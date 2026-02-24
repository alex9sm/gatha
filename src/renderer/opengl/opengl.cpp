#include "opengl.hpp"
#include "../../core/log.hpp"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#define WGL_CONTEXT_MAJOR_VERSION_ARB     0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB     0x2092
#define WGL_CONTEXT_PROFILE_MASK_ARB      0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB  0x00000001
#define WGL_DRAW_TO_WINDOW_ARB            0x2001
#define WGL_SUPPORT_OPENGL_ARB            0x2010
#define WGL_DOUBLE_BUFFER_ARB             0x2011
#define WGL_PIXEL_TYPE_ARB                0x2013
#define WGL_TYPE_RGBA_ARB                 0x202B
#define WGL_COLOR_BITS_ARB                0x2014
#define WGL_DEPTH_BITS_ARB                0x2022
#define WGL_STENCIL_BITS_ARB              0x2023

namespace opengl {

    PFNGLCLEARPROC              glClear = nullptr;
    PFNGLCLEARCOLORPROC         glClearColor = nullptr;
    PFNGLVIEWPORTPROC           glViewport = nullptr;
    PFNGLCREATEBUFFERSPROC      glCreateBuffers = nullptr;
    PFNGLNAMEDBUFFERSTORAGEPROC glNamedBufferStorage = nullptr;
    PFNGLMAPNAMEDBUFFERRANGEPROC glMapNamedBufferRange = nullptr;
    PFNGLDELETEBUFFERSPROC      glDeleteBuffers = nullptr;
    PFNGLBINDBUFFERBASEPROC     glBindBufferBase = nullptr;
    PFNGLCREATEVERTEXARRAYSPROC       glCreateVertexArrays = nullptr;
    PFNGLDELETEVERTEXARRAYSPROC       glDeleteVertexArrays = nullptr;
    PFNGLBINDVERTEXARRAYPROC          glBindVertexArray = nullptr;
    PFNGLVERTEXARRAYVERTEXBUFFERPROC  glVertexArrayVertexBuffer = nullptr;
    PFNGLVERTEXARRAYATTRIBFORMATPROC  glVertexArrayAttribFormat = nullptr;
    PFNGLVERTEXARRAYATTRIBBINDINGPROC glVertexArrayAttribBinding = nullptr;
    PFNGLENABLEVERTEXARRAYATTRIBPROC  glEnableVertexArrayAttrib = nullptr;
    PFNGLVERTEXARRAYELEMENTBUFFERPROC glVertexArrayElementBuffer = nullptr;
    PFNGLCREATESHADERPROC       glCreateShader = nullptr;
    PFNGLSHADERSOURCEPROC       glShaderSource = nullptr;
    PFNGLCOMPILESHADERPROC      glCompileShader = nullptr;
    PFNGLGETSHADERIVPROC        glGetShaderiv = nullptr;
    PFNGLGETSHADERINFOLOGPROC   glGetShaderInfoLog = nullptr;
    PFNGLDELETESHADERPROC       glDeleteShader = nullptr;
    PFNGLCREATEPROGRAMPROC      glCreateProgram = nullptr;
    PFNGLATTACHSHADERPROC       glAttachShader = nullptr;
    PFNGLLINKPROGRAMPROC        glLinkProgram = nullptr;
    PFNGLGETPROGRAMIVPROC       glGetProgramiv = nullptr;
    PFNGLGETPROGRAMINFOLOGPROC  glGetProgramInfoLog = nullptr;
    PFNGLUSEPROGRAMPROC         glUseProgram = nullptr;
    PFNGLDELETEPROGRAMPROC      glDeleteProgram = nullptr;
    PFNGLDRAWARRAYSINSTANCEDPROC       glDrawArraysInstanced = nullptr;
    PFNGLDRAWELEMENTSINSTANCEDPROC     glDrawElementsInstanced = nullptr;
    PFNGLMULTIDRAWELEMENTSINDIRECTPROC glMultiDrawElementsIndirect = nullptr;
    PFNGLDRAWELEMENTSINDIRECTPROC      glDrawElementsIndirect = nullptr;
    PFNGLFENCESYNCPROC      glFenceSync = nullptr;
    PFNGLCLIENTWAITSYNCPROC glClientWaitSync = nullptr;
    PFNGLDELETESYNCPROC     glDeleteSync = nullptr;

    PFNGLENABLEPROC              glEnable = nullptr;
    PFNGLGETUNIFORMLOCATIONPROC  glGetUniformLocation = nullptr;
    PFNGLUNIFORMMATRIX4FVPROC    glUniformMatrix4fv = nullptr;

	namespace {

		HMODULE opengl_dll = nullptr;
		HDC device_context = nullptr;
		HGLRC gl_context = nullptr;

		using PFNWGLCHOOSEPIXELFORMATARBPROC = int (*)(HDC hdc, const int* piAttribIList, const float* pfAttribFList, unsigned int nMaxFormats, int* piFormats, unsigned int* nNumFormats);
		using PFNWGLCREATECONTEXTATTRIBSARBPROC = HGLRC(*)(HDC hDC, HGLRC hShareContext, const int* attribList);
		using PFNWGLSWAPINTERVALEXTPROC = int (*)(int interval);

		PFNWGLCHOOSEPIXELFORMATARBPROC    wglChoosePixelFormatARB = nullptr;
		PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = nullptr;
		PFNWGLSWAPINTERVALEXTPROC         wglSwapIntervalEXT = nullptr;

	}

    static void* get_gl_proc(const char* name) {
        void* proc = (void*)wglGetProcAddress(name);
        if (!proc || proc == (void*)0x1 || proc == (void*)0x2 || proc == (void*)0x3 || proc == (void*)-1) {
            proc = (void*)GetProcAddress(opengl_dll, name);
        }
        return proc;
    }

    static bool load_gl_functions() {

        glClear = (PFNGLCLEARPROC)GetProcAddress(opengl_dll, "glClear");
        glClearColor = (PFNGLCLEARCOLORPROC)GetProcAddress(opengl_dll, "glClearColor");
        glViewport = (PFNGLVIEWPORTPROC)GetProcAddress(opengl_dll, "glViewport");

        glCreateBuffers = (PFNGLCREATEBUFFERSPROC)get_gl_proc("glCreateBuffers");
        glNamedBufferStorage = (PFNGLNAMEDBUFFERSTORAGEPROC)get_gl_proc("glNamedBufferStorage");
        glMapNamedBufferRange = (PFNGLMAPNAMEDBUFFERRANGEPROC)get_gl_proc("glMapNamedBufferRange");
        glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)get_gl_proc("glDeleteBuffers");
        glBindBufferBase = (PFNGLBINDBUFFERBASEPROC)get_gl_proc("glBindBufferBase");

        glCreateVertexArrays = (PFNGLCREATEVERTEXARRAYSPROC)get_gl_proc("glCreateVertexArrays");
        glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC)get_gl_proc("glDeleteVertexArrays");
        glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)get_gl_proc("glBindVertexArray");
        glVertexArrayVertexBuffer = (PFNGLVERTEXARRAYVERTEXBUFFERPROC)get_gl_proc("glVertexArrayVertexBuffer");
        glVertexArrayAttribFormat = (PFNGLVERTEXARRAYATTRIBFORMATPROC)get_gl_proc("glVertexArrayAttribFormat");
        glVertexArrayAttribBinding = (PFNGLVERTEXARRAYATTRIBBINDINGPROC)get_gl_proc("glVertexArrayAttribBinding");
        glEnableVertexArrayAttrib = (PFNGLENABLEVERTEXARRAYATTRIBPROC)get_gl_proc("glEnableVertexArrayAttrib");
        glVertexArrayElementBuffer = (PFNGLVERTEXARRAYELEMENTBUFFERPROC)get_gl_proc("glVertexArrayElementBuffer");

        glCreateShader = (PFNGLCREATESHADERPROC)get_gl_proc("glCreateShader");
        glShaderSource = (PFNGLSHADERSOURCEPROC)get_gl_proc("glShaderSource");
        glCompileShader = (PFNGLCOMPILESHADERPROC)get_gl_proc("glCompileShader");
        glGetShaderiv = (PFNGLGETSHADERIVPROC)get_gl_proc("glGetShaderiv");
        glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)get_gl_proc("glGetShaderInfoLog");
        glDeleteShader = (PFNGLDELETESHADERPROC)get_gl_proc("glDeleteShader");
        glCreateProgram = (PFNGLCREATEPROGRAMPROC)get_gl_proc("glCreateProgram");
        glAttachShader = (PFNGLATTACHSHADERPROC)get_gl_proc("glAttachShader");
        glLinkProgram = (PFNGLLINKPROGRAMPROC)get_gl_proc("glLinkProgram");
        glGetProgramiv = (PFNGLGETPROGRAMIVPROC)get_gl_proc("glGetProgramiv");
        glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)get_gl_proc("glGetProgramInfoLog");
        glUseProgram = (PFNGLUSEPROGRAMPROC)get_gl_proc("glUseProgram");
        glDeleteProgram = (PFNGLDELETEPROGRAMPROC)get_gl_proc("glDeleteProgram");

        glDrawArraysInstanced = (PFNGLDRAWARRAYSINSTANCEDPROC)get_gl_proc("glDrawArraysInstanced");
        glDrawElementsInstanced = (PFNGLDRAWELEMENTSINSTANCEDPROC)get_gl_proc("glDrawElementsInstanced");
        glMultiDrawElementsIndirect = (PFNGLMULTIDRAWELEMENTSINDIRECTPROC)get_gl_proc("glMultiDrawElementsIndirect");
        glDrawElementsIndirect = (PFNGLDRAWELEMENTSINDIRECTPROC)get_gl_proc("glDrawElementsIndirect");

        glFenceSync = (PFNGLFENCESYNCPROC)get_gl_proc("glFenceSync");
        glClientWaitSync = (PFNGLCLIENTWAITSYNCPROC)get_gl_proc("glClientWaitSync");
        glDeleteSync = (PFNGLDELETESYNCPROC)get_gl_proc("glDeleteSync");

        glEnable = (PFNGLENABLEPROC)GetProcAddress(opengl_dll, "glEnable");
        glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)get_gl_proc("glGetUniformLocation");
        glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)get_gl_proc("glUniformMatrix4fv");

        return true;
    }

	bool init(void* hwnd, u32 w, u32 h) {
		HWND window = static_cast<HWND>(hwnd);
		opengl_dll = LoadLibraryA("opengl32.dll");
		device_context = GetDC(window);

        PIXELFORMATDESCRIPTOR pfd = {};
        pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
        pfd.nVersion = 1;
        pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
        pfd.iPixelType = PFD_TYPE_RGBA;
        pfd.cColorBits = 32;
        pfd.cDepthBits = 24;
        pfd.cStencilBits = 8;
        int dummy_pixel_format = ChoosePixelFormat(device_context, &pfd);
        SetPixelFormat(device_context, dummy_pixel_format, &pfd);
        HGLRC dummy_context = wglCreateContext(device_context);
        wglMakeCurrent(device_context, dummy_context);

		wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
		wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
		wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");

        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(dummy_context);

		int pixel_attr[] = {
			WGL_DRAW_TO_WINDOW_ARB, 1,
			WGL_SUPPORT_OPENGL_ARB, 1,
			WGL_DOUBLE_BUFFER_ARB,  1,
			WGL_PIXEL_TYPE_ARB,     WGL_TYPE_RGBA_ARB,
			WGL_COLOR_BITS_ARB,     32,
			WGL_DEPTH_BITS_ARB,     24,
			WGL_STENCIL_BITS_ARB,   8,
			0
		};

		int pixel_format;
		u32 num_formats;
		wglChoosePixelFormatARB(device_context, pixel_attr, nullptr, 1, &pixel_format, &num_formats);

		int context_attr[] = {
			WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
			WGL_CONTEXT_MINOR_VERSION_ARB, 5,
			WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
			0
		};

		gl_context = wglCreateContextAttribsARB(device_context, nullptr, context_attr);
		wglMakeCurrent(device_context, gl_context);
        if (!load_gl_functions()) {
            return false;
        }
        wglSwapIntervalEXT(0);

        glViewport(0, 0, static_cast<GLsizei>(w), static_cast<GLsizei>(h));
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);

        log::info("opengl context initialized");
        return true;
	}

    void shutdown() {
        if (gl_context) {
            wglMakeCurrent(nullptr, nullptr);
            wglDeleteContext(gl_context);
            gl_context = nullptr;
        }

        if (device_context) {
            device_context = nullptr;
        }

        if (opengl_dll) {
            FreeLibrary(opengl_dll);
            opengl_dll = nullptr;
        }
    }

    void swap_buffers() {
        SwapBuffers(device_context);
    }

    void set_viewport(u32 w, u32 h) {
        glViewport(0, 0, w, h);
    }

}