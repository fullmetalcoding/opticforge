#include "PsfTextureRenderer.h"

#include <GL/glew.h>

#include <stdexcept>

namespace opticforge::renderer
{
    PsfTextureRenderer::~PsfTextureRenderer()
    {
        release();
    }

    void PsfTextureRenderer::release() noexcept
    {
        if (m_texture)
            glDeleteTextures(1, &m_texture);

        m_texture = 0;
        m_width = 0;
        m_height = 0;
    }

    void PsfTextureRenderer::render(
        const raytracer::TraceResult& result,
        const telescope::ObservationPlane& plane,
        const PsfRenderSettings& settings,
        const PsfColorFunction& color)
    {
        const auto image =
            rasterizePsf(result, plane, settings, color);

        GLint maxSize = 0;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxSize);

        if (
            image.width > maxSize ||
            image.height > maxSize)
        {
            throw std::runtime_error(
                "PSF texture exceeds GL_MAX_TEXTURE_SIZE");
        }

        // Preserve the texture binding and pixel-unpack state.
        // Framebuffer, viewport, and drawing state are untouched.
        GLint binding = 0;
        GLint pbo = 0;
        GLint alignment = 0;
        GLint rowLength = 0;
        GLint skipRows = 0;
        GLint skipPixels = 0;

        glGetIntegerv(
            GL_TEXTURE_BINDING_2D,
            &binding);

        glGetIntegerv(
            GL_PIXEL_UNPACK_BUFFER_BINDING,
            &pbo);

        glGetIntegerv(
            GL_UNPACK_ALIGNMENT,
            &alignment);

        glGetIntegerv(
            GL_UNPACK_ROW_LENGTH,
            &rowLength);

        glGetIntegerv(
            GL_UNPACK_SKIP_ROWS,
            &skipRows);

        glGetIntegerv(
            GL_UNPACK_SKIP_PIXELS,
            &skipPixels);

        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
        glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);

        if (!m_texture)
            glGenTextures(1, &m_texture);

        glBindTexture(GL_TEXTURE_2D, m_texture);

        // Avoid adding interpolation to single-pixel marks.
        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_MIN_FILTER,
            GL_NEAREST);

        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_MAG_FILTER,
            GL_NEAREST);

        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_WRAP_S,
            GL_CLAMP_TO_EDGE);

        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_WRAP_T,
            GL_CLAMP_TO_EDGE);

        if (
            m_width != image.width ||
            m_height != image.height)
        {
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RGBA8,
                image.width,
                image.height,
                0,
                GL_RGBA,
                GL_UNSIGNED_BYTE,
                image.rgba.data());
        }
        else
        {
            glTexSubImage2D(
                GL_TEXTURE_2D,
                0,
                0,
                0,
                image.width,
                image.height,
                GL_RGBA,
                GL_UNSIGNED_BYTE,
                image.rgba.data());
        }

        m_width = image.width;
        m_height = image.height;

        glBindTexture(
            GL_TEXTURE_2D,
            static_cast<GLuint>(binding));

        glBindBuffer(
            GL_PIXEL_UNPACK_BUFFER,
            static_cast<GLuint>(pbo));

        glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, rowLength);
        glPixelStorei(GL_UNPACK_SKIP_ROWS, skipRows);
        glPixelStorei(GL_UNPACK_SKIP_PIXELS, skipPixels);
    }
}