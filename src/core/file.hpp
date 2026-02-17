#pragma once

#include "types.hpp"

namespace file {

	bool get_size(const char* path, u64* file_size);
	u64 read_file(const char* path, void* buffer, u64 buffer_size);
	bool exists(const char* path);
	using file_visit_fn = bool(*)(const char* filename, void* userdata);
	void file_visit(const char* folder, const char* extension, file_visit_fn callback, void* userdata);

}