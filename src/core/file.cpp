#include "file.hpp"
#include "log.hpp"
#include "string.hpp"

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
		return true;
	}

	u64 read_file(const char* path, void* buffer, u64 buffer_size) {
		HANDLE h = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

		if (h == INVALID_HANDLE_VALUE) {
			logger::error("file read could not open: %s Win32 error %lu", path, GetLastError());
			return 0;
		}

		DWORD to_read = static_cast<DWORD>(buffer_size);
		DWORD bytes_read = 0;

		BOOL ok = ReadFile(h, buffer, to_read, &bytes_read, nullptr);
		CloseHandle(h);

		if (!ok) {
			logger::error("file read could not read: %s Win32 error %lu", path, GetLastError());
			return 0;
		}
		return static_cast<u64>(bytes_read);
	}

	bool write_file(const char* path, const void* data, u64 size) {
		HANDLE h = CreateFileA(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (h == INVALID_HANDLE_VALUE) {
			logger::error("file write could not create: %s Win32 error %lu", path, GetLastError());
			return false;
		}
		DWORD written = 0;
		BOOL ok = WriteFile(h, data, static_cast<DWORD>(size), &written, nullptr);
		CloseHandle(h);
		if (!ok || written != (DWORD)size) {
			logger::error("file write failed: %s Win32 error %lu", path, GetLastError());
			return false;
		}
		return true;
	}

	bool exists(const char* path) {
		DWORD attr = GetFileAttributesA(path);
		return (attr != INVALID_FILE_ATTRIBUTES) && !(attr & FILE_ATTRIBUTE_DIRECTORY);
	}

	void file_visit(const char* folder, const char* extension, file_visit_fn callback, void* userdata) {
		char pattern[256];
		str::copy(pattern, folder, sizeof(pattern));
		str::concat(pattern, "/*", sizeof(pattern));
		str::concat(pattern, extension, sizeof(pattern));

		WIN32_FIND_DATAA find_data;
		HANDLE h = FindFirstFileA(pattern, &find_data);

		if (h == INVALID_HANDLE_VALUE) return;

		do {
			if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
			if (!callback(find_data.cFileName, userdata)) break;

		} while (FindNextFileA(h, &find_data));

		FindClose(h);
	}

}