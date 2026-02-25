#include "shader.hpp"
#include "../../core/file.hpp"
#include "../../core/memory.hpp"
#include "../../core/log.hpp"
#include "../../core/string.hpp"
#include "../../core/array.hpp"

namespace opengl {
	
	namespace {
	
		struct ShaderEntry {
			char name[64];
			GLuint program;
		};
		arr::Array<ShaderEntry> registry;
		
	}

    static GLuint compile_shader(GLenum type, const char* path) {
        u64 file_size = 0;
        if (!file::get_size(path, &file_size)) {
            logger::error("shader: could not find '%s'", path);
            return 0;
        }

        char* source = static_cast<char*>(memory::malloc(file_size + 1));
        if (!source) {
            logger::error("shader: out of memory reading '%s'", path);
            return 0;
        }

        u64 bytes_read = file::read_file(path, source, file_size);
        source[bytes_read] = '\0';

        if (bytes_read != file_size) {
            logger::error("shader: partial read on '%s' (%llu of %llu bytes)", path, bytes_read, file_size);
            memory::free(source);
            return 0;
        }

        GLuint shader = glCreateShader(type);
        const GLchar* src = source;
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);

        memory::free(source);

        GLint status = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

        if (!status) {
            char info[1024];
            glGetShaderInfoLog(shader, sizeof(info), nullptr, info);
            logger::error("shader: compile error in '%s':\n%s", path, info);
            glDeleteShader(shader);
            return 0;
        }

        return shader;
    }

    GLuint shader_create(const char* vert_path, const char* frag_path) {
        GLuint vert = compile_shader(GL_VERTEX_SHADER, vert_path);
        if (!vert) return 0;

        GLuint frag = compile_shader(GL_FRAGMENT_SHADER, frag_path);
        if (!frag) {
            glDeleteShader(vert);
            return 0;
        }

        GLuint program = glCreateProgram();
        glAttachShader(program, vert);
        glAttachShader(program, frag);
        glLinkProgram(program);

        glDeleteShader(vert);
        glDeleteShader(frag);

        GLint status = 0;
        glGetProgramiv(program, GL_LINK_STATUS, &status);

        if (!status) {
            char info[1024];
            glGetProgramInfoLog(program, sizeof(info), nullptr, info);
            logger::error("shader: link error (%s + %s):\n%s", vert_path, frag_path, info);
            glDeleteProgram(program);
            return 0;
        }

        return program;
    }

    void shader_destroy(GLuint program) {
        if (program) glDeleteProgram(program);
    }

    static bool on_frag_found(const char* filename, void* userdata) {
        const char* folder = static_cast<const char*>(userdata);
        usize name_len = str::length(filename) - 5;

        if (name_len == 0 || name_len >= 64) {
            logger::warn("shader_load: skipping %s", filename);
            return true;
        }

        char name[64] = {};
        memory::copy(name, filename, name_len);

        char vert_path[256];
        char frag_path[256];
        str::format(vert_path, sizeof(vert_path), "%s/%s.vert", folder, name);
        str::format(frag_path, sizeof(frag_path), "%s/%s.frag", folder, name);

        GLuint program = shader_create(vert_path, frag_path);
        if (!program) return true;

        ShaderEntry entry = {};
        str::copy(entry.name, name, sizeof(entry.name));
        entry.program = program;
        arr::array_push(&registry, entry);

        return true;
    }

    bool shader_load(const char* folder) {
        file::file_visit(folder, ".frag", on_frag_found, (void*)folder);
        return true;
    }

    GLuint shader_get(const char* name) {
        for (usize i = 0; i < registry.count; i++) {
            if (str::equal(registry.data[i].name, name)) {
                return registry.data[i].program;
            }
        }
        logger::warn("shader_get: %s not found", name);
        return 0;
    }

    void shader_unload() {
        for (usize i = 0; i < registry.count; i++) {
            glDeleteProgram(registry.data[i].program);
        }
        arr::array_destroy(&registry);
        logger::info("shader: shutdown");
    }
}