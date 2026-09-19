#pragma once

#include "telescope/TelescopePrimitives.h"

#include <GL/glew.h>

#include <cstdint>
#include <limits>

namespace opticforge
{
    class PickingFramebuffer
    {
    public:
        static constexpr telescope::PrimitiveId InvalidPrimitiveId =
            std::numeric_limits<telescope::PrimitiveId>::max();

        PickingFramebuffer() = default;
        ~PickingFramebuffer();

        PickingFramebuffer(const PickingFramebuffer&) = delete;
        PickingFramebuffer& operator=(const PickingFramebuffer&) = delete;

        void ensureSize(int width, int height);

        void bindForDrawing() const;

        telescope::PrimitiveId readPixel(
            int x,
            int yFromTop) const;

        void destroy();

        [[nodiscard]]
        int width() const noexcept
        {
            return m_width;
        }

        [[nodiscard]]
        int height() const noexcept
        {
            return m_height;
        }

    private:
        void create(int width, int height);

        GLuint m_fbo = 0;
        GLuint m_idTexture = 0;
        GLuint m_depthRenderbuffer = 0;

        int m_width = 0;
        int m_height = 0;
    };
}