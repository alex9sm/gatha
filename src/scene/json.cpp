#include "json.hpp"
#include "../core/memory.hpp"
#include "../core/string.hpp"

namespace json {

	struct Parser {
		const char* text;
		usize       pos;
		usize       length;
	};

	static void skip_whitespace(Parser* p) {
		while (p->pos < p->length) {
			char c = p->text[p->pos];
			if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
				p->pos++;
			else
				break;
		}
	}

	static char peek(Parser* p) {
		skip_whitespace(p);
		if (p->pos >= p->length) return '\0';
		return p->text[p->pos];
	}

	static char advance(Parser* p) {
		if (p->pos >= p->length) return '\0';
		return p->text[p->pos++];
	}

	static bool expect(Parser* p, char c) {
		skip_whitespace(p);
		if (p->pos < p->length && p->text[p->pos] == c) {
			p->pos++;
			return true;
		}
		return false;
	}

	static Value* alloc_value(Type type) {
		Value* v = (Value*)memory::malloc(sizeof(Value));
		memory::set(v, 0, sizeof(Value));
		v->type = type;
		return v;
	}

	static Value* parse_value(Parser* p);

	static char* parse_string_raw(Parser* p) {
		if (!expect(p, '"')) return nullptr;

		usize start = p->pos;
		usize escaped_len = 0;

		// First pass: find end and compute unescaped length
		while (p->pos < p->length && p->text[p->pos] != '"') {
			if (p->text[p->pos] == '\\') {
				p->pos++;
				escaped_len++;
			}
			p->pos++;
			escaped_len++;
		}

		char* result = (char*)memory::malloc(escaped_len + 1);
		usize out = 0;
		usize src = start;

		while (src < p->pos) {
			if (p->text[src] == '\\' && src + 1 < p->pos) {
				src++;
				switch (p->text[src]) {
					case '"':  result[out++] = '"';  break;
					case '\\': result[out++] = '\\'; break;
					case '/':  result[out++] = '/';  break;
					case 'n':  result[out++] = '\n'; break;
					case 't':  result[out++] = '\t'; break;
					case 'r':  result[out++] = '\r'; break;
					case 'b':  result[out++] = '\b'; break;
					case 'f':  result[out++] = '\f'; break;
					default:   result[out++] = p->text[src]; break;
				}
			} else {
				result[out++] = p->text[src];
			}
			src++;
		}
		result[out] = '\0';

		p->pos++; // skip closing "
		return result;
	}

	static Value* parse_string(Parser* p) {
		char* s = parse_string_raw(p);
		if (!s) return nullptr;
		Value* v = alloc_value(STRING);
		v->string = s;
		return v;
	}

	static Value* parse_number(Parser* p) {
		skip_whitespace(p);
		usize start = p->pos;

		if (p->pos < p->length && p->text[p->pos] == '-') p->pos++;
		while (p->pos < p->length && p->text[p->pos] >= '0' && p->text[p->pos] <= '9') p->pos++;
		if (p->pos < p->length && p->text[p->pos] == '.') {
			p->pos++;
			while (p->pos < p->length && p->text[p->pos] >= '0' && p->text[p->pos] <= '9') p->pos++;
		}
		if (p->pos < p->length && (p->text[p->pos] == 'e' || p->text[p->pos] == 'E')) {
			p->pos++;
			if (p->pos < p->length && (p->text[p->pos] == '+' || p->text[p->pos] == '-')) p->pos++;
			while (p->pos < p->length && p->text[p->pos] >= '0' && p->text[p->pos] <= '9') p->pos++;
		}

		// Manual string-to-double since we don't have strtod
		f64 result = 0.0;
		f64 sign = 1.0;
		usize i = start;

		if (p->text[i] == '-') { sign = -1.0; i++; }
		while (i < p->pos && p->text[i] >= '0' && p->text[i] <= '9') {
			result = result * 10.0 + (p->text[i] - '0');
			i++;
		}
		if (i < p->pos && p->text[i] == '.') {
			i++;
			f64 frac = 0.1;
			while (i < p->pos && p->text[i] >= '0' && p->text[i] <= '9') {
				result += (p->text[i] - '0') * frac;
				frac *= 0.1;
				i++;
			}
		}
		if (i < p->pos && (p->text[i] == 'e' || p->text[i] == 'E')) {
			i++;
			f64 exp_sign = 1.0;
			if (i < p->pos && p->text[i] == '-') { exp_sign = -1.0; i++; }
			else if (i < p->pos && p->text[i] == '+') { i++; }
			f64 exp = 0.0;
			while (i < p->pos && p->text[i] >= '0' && p->text[i] <= '9') {
				exp = exp * 10.0 + (p->text[i] - '0');
				i++;
			}
			f64 power = 1.0;
			for (int e = 0; e < (int)exp; e++) power *= 10.0;
			if (exp_sign < 0) result /= power;
			else result *= power;
		}
		result *= sign;

		Value* v = alloc_value(NUMBER);
		v->number = result;
		return v;
	}

	static void obj_grow(Value* obj) {
		u32 new_cap = obj->object.capacity == 0 ? 8 : obj->object.capacity * 2;
		obj->object.keys = (char**)memory::realloc(obj->object.keys, new_cap * sizeof(char*));
		obj->object.values = (Value**)memory::realloc(obj->object.values, new_cap * sizeof(Value*));
		obj->object.capacity = new_cap;
	}

	static void arr_grow(Value* arr) {
		u32 new_cap = arr->array.capacity == 0 ? 8 : arr->array.capacity * 2;
		arr->array.items = (Value**)memory::realloc(arr->array.items, new_cap * sizeof(Value*));
		arr->array.capacity = new_cap;
	}

	static Value* parse_object(Parser* p) {
		if (!expect(p, '{')) return nullptr;
		Value* obj = alloc_value(OBJECT);

		if (peek(p) == '}') { p->pos++; return obj; }

		for (;;) {
			char* key = parse_string_raw(p);
			if (!key) break;
			if (!expect(p, ':')) { memory::free(key); break; }
			Value* val = parse_value(p);
			if (!val) { memory::free(key); break; }

			if (obj->object.count >= obj->object.capacity) obj_grow(obj);
			obj->object.keys[obj->object.count] = key;
			obj->object.values[obj->object.count] = val;
			obj->object.count++;

			if (!expect(p, ',')) break;
		}

		expect(p, '}');
		return obj;
	}

	static Value* parse_array(Parser* p) {
		if (!expect(p, '[')) return nullptr;
		Value* arr = alloc_value(ARRAY);

		if (peek(p) == ']') { p->pos++; return arr; }

		for (;;) {
			Value* val = parse_value(p);
			if (!val) break;

			if (arr->array.count >= arr->array.capacity) arr_grow(arr);
			arr->array.items[arr->array.count++] = val;

			if (!expect(p, ',')) break;
		}

		expect(p, ']');
		return arr;
	}

	static bool match_literal(Parser* p, const char* lit) {
		usize len = str::length(lit);
		if (p->pos + len > p->length) return false;
		for (usize i = 0; i < len; i++) {
			if (p->text[p->pos + i] != lit[i]) return false;
		}
		p->pos += len;
		return true;
	}

	static Value* parse_value(Parser* p) {
		char c = peek(p);
		if (c == '"') return parse_string(p);
		if (c == '{') return parse_object(p);
		if (c == '[') return parse_array(p);
		if (c == 't') { if (match_literal(p, "true"))  { Value* v = alloc_value(BOOL); v->boolean = true;  return v; } }
		if (c == 'f') { if (match_literal(p, "false")) { Value* v = alloc_value(BOOL); v->boolean = false; return v; } }
		if (c == 'n') { if (match_literal(p, "null"))   return alloc_value(NIL); }
		if (c == '-' || (c >= '0' && c <= '9')) return parse_number(p);
		return nullptr;
	}

	Value* parse(const char* text, usize length) {
		Parser p = { text, 0, length };
		return parse_value(&p);
	}

	void destroy(Value* v) {
		if (!v) return;
		switch (v->type) {
			case OBJECT:
				for (u32 i = 0; i < v->object.count; i++) {
					memory::free(v->object.keys[i]);
					destroy(v->object.values[i]);
				}
				memory::free(v->object.keys);
				memory::free(v->object.values);
				break;
			case ARRAY:
				for (u32 i = 0; i < v->array.count; i++) {
					destroy(v->array.items[i]);
				}
				memory::free(v->array.items);
				break;
			case STRING:
				memory::free(v->string);
				break;
			default: break;
		}
		memory::free(v);
	}

	Value* get(const Value* obj, const char* key) {
		if (!obj || obj->type != OBJECT) return nullptr;
		for (u32 i = 0; i < obj->object.count; i++) {
			if (str::equal(obj->object.keys[i], key)) return obj->object.values[i];
		}
		return nullptr;
	}

	u32 length(const Value* arr) {
		if (!arr || arr->type != ARRAY) return 0;
		return arr->array.count;
	}

	Value* at(const Value* arr, u32 index) {
		if (!arr || arr->type != ARRAY || index >= arr->array.count) return nullptr;
		return arr->array.items[index];
	}

	f64 as_number(const Value* v, f64 fallback) {
		if (!v || v->type != NUMBER) return fallback;
		return v->number;
	}

	const char* as_string(const Value* v, const char* fallback) {
		if (!v || v->type != STRING) return fallback;
		return v->string;
	}

	bool as_bool(const Value* v, bool fallback) {
		if (!v || v->type != BOOL) return fallback;
		return v->boolean;
	}

}
