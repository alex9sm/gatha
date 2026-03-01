#pragma once

#include "../core/types.hpp"

namespace json {

	enum Type : u8 { OBJECT, ARRAY, STRING, NUMBER, BOOL, NIL };

	struct Value {
		Type type;
		union {
			struct { char** keys; Value** values; u32 count; u32 capacity; } object;
			struct { Value** items; u32 count; u32 capacity; } array;
			char*  string;
			f64    number;
			bool   boolean;
		};
	};

	Value* parse(const char* text, usize length);
	void   destroy(Value* v);

	Value*      get(const Value* obj, const char* key);
	u32         length(const Value* arr);
	Value*      at(const Value* arr, u32 index);

	f64         as_number(const Value* v, f64 fallback = 0.0);
	const char* as_string(const Value* v, const char* fallback = "");
	bool        as_bool(const Value* v, bool fallback = false);

}
