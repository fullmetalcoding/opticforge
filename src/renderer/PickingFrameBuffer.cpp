#include "PickingFramebuffer.h"

#include <stdexcept>

namespace opticforge
{
    PickingFramebuffer::~PickingFramebuffer()
    {
        destroy();
    }

    void PickingFramebuffer::ensureSize(
        int width,
        int height)
    {
        if (width <= 0 || height <= 0)
            return;

        if (
            m_fbo != 0 &&
            width == m_width &&
            height == m_height)
        {
            return;
        }

        destroy();
        create(width, height);
    }

    void PickingFramebuffer::create(
        int width,
        int height)
    {
        m_width = width;
        m_height = height;

        glGenFramebuffers(
            1,
            &m_fbo);

        glBindFramebuffer(
            GL_FRAMEBUFFER,
            m_fbo);

        //
        // PrimitiveId is uint64_t.
        //
        // Store:
        //   R = low 32 bits
        //   G = high 32 bits
        //
        glGenTextures(
            1,
            &m_idTexture);

        glBindTexture(
            GL_TEXTURE_2D,
            m_idTexture);

        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RG32UI,
            width,
            height,
            0,
            GL_RG_INTEGER,
            GL_UNSIGNED_INT,
            nullptr);

        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_MIN_FILTER,
            GL_NEAREST);

        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_MAG_FILTER,
            GL_NEAREST);

        glFramebufferTexture2D(
            GL_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D,
            m_idTexture,
            0);

        glGenRenderbuffers(
            1,
            &m_depthRenderbuffer);

        glBindRenderbuffer(
            GL_RENDERBUFFER,
            m_depthRenderbuffer);

        glRenderbufferStorage(
            GL_RENDERBUFFER,
            GL_DEPTH_COMPONENT24,
            width,
            height);

        glFramebufferRenderbuffer(
            GL_FRAMEBUFFER,
            GL_DEPTH_ATTACHMENT,
            GL_RENDERBUFFER,
            m_depthRenderbuffer);

        const GLenum drawBuffers[] =
        {
            GL_COLOR_ATTACHMENT0
        };

        glDrawBuffers(
            1,
            drawBuffers);

        const GLenum status =
            glCheckFramebufferStatus(
                GL_FRAMEBUFFER);

        glBindFramebuffer(
            GL_FRAMEBUFFER,
            0);

        if (status != GL_FRAMEBUFFER_COMPLETE)
        {
            destroy();

            throw std::runtime_error(
                "Picking framebuffer is incomplete.");
        }
    }

    void PickingFramebuffer::bindForDrawing() const
    {
        glBindFramebuffer(
            GL_FRAMEBUFFER,
            m_fbo);
    }

    telescope::PrimitiveId
        PickingFramebuffer::readPixel(
            int x,
            int yFromTop) const
    {
        if (
            m_fbo == 0 ||
            x < 0 ||
            yFromTop < 0 ||
            x >= m_width ||
            yFromTop >= m_height)
        {
            return InvalidPrimitiveId;
        }

        //
        // SDL/window coordinates use top-left origin.
        // OpenGL framebuffer coordinates use bottom-left.
        //
        const int glY =
            m_height -
            yFromTop -
            1;

        GLuint words[2] =
        {
            0xffffffffu,
            0xffffffffu
        };

        glBindFramebuffer(
            GL_READ_FRAMEBUFFER,
            m_fbo);

        glReadBuffer(
            GL_COLOR_ATTACHMENT0);

        glReadPixels(
            x,
            glY,
            1,
            1,
            GL_RG_INTEGER,
            GL_UNSIGNED_INT,
            words);

        const auto low =
            static_cast<std::uint64_t>(
                words[0]);

        const auto high =
            static_cast<std::uint64_t>(
                words[1]);

        return
            static_cast<telescope::PrimitiveId>(
                low |
                (high << 32));
    }

    void PickingFramebuffer::destroy()
    {
        if (m_depthRenderbuffer != 0)
        {
            glDeleteRenderbuffers(
                1,
                &m_depthRenderbuffer);

            m_depthRenderbuffer = 0;
        }

        if (m_idTexture != 0)
        {
            glDeleteTextures(
                1,
                &m_idTexture);

            m_idTexture = 0;
        }

        if (m_fbo != 0)
        {
            glDeleteFramebuffers(
                1,
                &m_fbo);

            m_fbo = 0;
        }

        m_width = 0;
        m_height = 0;
    }
}