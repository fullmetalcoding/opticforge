#pragma once
#include <GL/glew.h>
namespace opticforge {
    class MeshGpu
    {
    public:
        MeshGpu() = default;

        ~MeshGpu();

        MeshGpu(
            const MeshGpu&) = delete;

        MeshGpu& operator=(
            const MeshGpu&) = delete;

        MeshGpu(
            MeshGpu&& other) noexcept;

        MeshGpu& operator=(
            MeshGpu&& other) noexcept;

        void destroy();
        void bind() const;
        void unbind();
        GLuint vao() const;
        GLuint vbo() const;
        GLuint ebo() const;
        GLsizei indexCount() const; 
        bool valid() const;

    private:
        friend class RenderSystem;
        GLuint m_vao = 0;
        GLuint m_vbo = 0;
        GLuint m_ebo = 0;

        GLsizei m_indexCount = 0;
    };
}