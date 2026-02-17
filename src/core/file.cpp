#include "file.hpp"
#include "log.hpp"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace file {

	bool get_size(const char* path, u64* file_size) {
		HANDLE h = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

		if (h == INVALID_HANDLE_VALUE) {
			return false;
		}

		LARGE_INTEGER size;
		if (!GetFileSizeEx(h, &size)) {
			CloseHandle(h);
			return false;
		}

		*file_size = static_cast<u64>(size.QuadPart);
		CloseHandle(h);
		return false;
	}

	u64 read_file(const char* path, void* buffer, u64 buffer_size) {
		HANDLE h = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

		if (h == INVALID_HANDLE_VALUE) {
			log::error("file read could not open: %s Win32 error %lu", path, GetLastError());
			return 0;
		}

		DWORD to_read = static_cast<DWORD>(buffer_size);
		DWORD bytes_read = 0;

		BOOL ok = ReadFile(h, buffer, to_read, &bytes_read, nullptr);
		CloseHandle(h);

		if (!ok) {
			log::error("file read could not read: % s Win32 error % lu", path, GetLastError());
			return 0;
		}
		return static_cast<u64>(bytes_read);
	}

	bool exists(const char* path) {
		DWORD attr = GetFileAttributesA(path);
		return (attr != INVALID_FILE_ATTRIBUTES) && !(attr & FILE_ATTRIBUTE_DIRECTORY);
	}

}