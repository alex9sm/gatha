#include "mesh.hpp"

namespace opengl {

    Mesh mesh_create(const Vertex* vertices, u32 vertex_count, const u32* indices, u32 index_count) {
        Mesh mesh = {};
        mesh.index_count = index_count;
        glCreateVertexArrays(1, &mesh.vao);
        glCreateBuffers(1, &mesh.vbo);
        glCreateBuffers(1, &mesh.ibo);
        glNamedBufferStorage(mesh.vbo, vertex_count * sizeof(Vertex), vertices, 0);
        glNamedBufferStorage(mesh.ibo, index_count * sizeof(u32), indices, 0);
        glVertexArrayVertexBuffer(mesh.vao, 0, mesh.vbo, 0, sizeof(Vertex));
        glVertexArrayElementBuffer(mesh.vao, mesh.ibo);
        glVertexArrayAttribFormat(mesh.vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(mesh.vao, 0, 0);
        glEnableVertexArrayAttrib(mesh.vao, 0);
        glVertexArrayAttribFormat(mesh.vao, 1, 3, GL_FLOAT, GL_FALSE, 12);
        glVertexArrayAttribBinding(mesh.vao, 1, 0);
        glEnableVertexArrayAttrib(mesh.vao, 1);
        glVertexArrayAttribFormat(mesh.vao, 2, 2, GL_FLOAT, GL_FALSE, 24);
        glVertexArrayAttribBinding(mesh.vao, 2, 0);
        glEnableVertexArrayAttrib(mesh.vao, 2);
        glVertexArrayAttribFormat(mesh.vao, 3, 4, GL_FLOAT, GL_FALSE, 32);
        glVertexArrayAttribBinding(mesh.vao, 3, 0);
        glEnableVertexArrayAttrib(mesh.vao, 3);

        return mesh;
    }

    void mesh_destroy(Mesh* mesh) {
        glDeleteVertexArrays(1, &mesh->vao);
        glDeleteBuffers(1, &mesh->vbo);
        glDeleteBuffers(1, &mesh->ibo);
        *mesh = {};
    }

    void mesh_draw(const Mesh& mesh) {
        glBindVertexArray(mesh.vao);
        glDrawElementsInstanced(GL_TRIANGLES, mesh.index_count, GL_UNSIGNED_INT, nullptr, 1);
    }

}