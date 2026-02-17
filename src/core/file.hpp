#pragma once

#include "types.hpp"

namespace file {

	bool get_size(const char* path, u64* file_size);
	u64 read_file(const char* path, void* buffer, u64 buffer_size);
	bool exists(const char* path);

}