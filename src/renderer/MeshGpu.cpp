// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "MeshGpu.h"

#include <utility>

namespace opticforge
{

    MeshGpu::~MeshGpu()
    {
        destroy();
    }

    MeshGpu::MeshGpu(
        MeshGpu&& other) noexcept
        : m_vao(other.m_vao),
        m_vbo(other.m_vbo),
        m_ebo(other.m_ebo),
        m_indexCount(other.m_indexCount)
    {
        other.m_vao = 0;
        other.m_vbo = 0;
        other.m_ebo = 0;
        other.m_indexCount = 0;
    }

    MeshGpu& MeshGpu::operator=(
        MeshGpu&& other) noexcept
    {
        if (this == &other)
            return *this;

        // Release any resources currently owned by this object.
        destroy();

        // Take ownership of the other object's resources.
        m_vao = other.m_vao;
        m_vbo = other.m_vbo;
        m_ebo = other.m_ebo;
        m_indexCount = other.m_indexCount;

        // Prevent the moved-from object from deleting resources
        // that now belong to this object.
        other.m_vao = 0;
        other.m_vbo = 0;
        other.m_ebo = 0;
        other.m_indexCount = 0;

        return *this;
    }

    void MeshGpu::destroy()
    {
        if (m_ebo != 0)
        {
            glDeleteBuffers(
                1,
                &m_ebo);

            m_ebo = 0;
        }

        if (m_vbo != 0)
        {
            glDeleteBuffers(
                1,
                &m_vbo);

            m_vbo = 0;
        }

        if (m_vao != 0)
        {
            glDeleteVertexArrays(
                1,
                &m_vao);

            m_vao = 0;
        }

        m_indexCount = 0;
    }

    void MeshGpu::bind() const
    {
        glBindVertexArray(m_vao);
    }

    void MeshGpu::unbind()
    {
        glBindVertexArray(0);
    }

    GLuint MeshGpu::vao() const
    {
        return m_vao;
    }

    GLuint MeshGpu::vbo() const
    {
        return m_vbo;
    }

    GLuint MeshGpu::ebo() const
    {
        return m_ebo;
    }

    GLsizei MeshGpu::indexCount() const
    {
        return m_indexCount;
    }

    bool MeshGpu::valid() const
    {
        return m_vao != 0;
    }

} // namespace opticforge