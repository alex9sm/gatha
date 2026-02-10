#pragma once

#include "framework.h"
#include "Resource.h"

namespace platform {
	namespace internal {
	
		bool init(HINSTANCE hInstance, int nCmdShow);
		void process_messages();
		bool is_running();
		void shutdown();
		HWND get_hwnd();
	
	}
}




//int RunApp(HINSTANCE hInstance, int nCmdShow);