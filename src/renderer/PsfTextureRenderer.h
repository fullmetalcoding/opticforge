// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include "PsfRasterizer.h"

namespace opticforge::renderer
{
    class PsfTextureRenderer
    {
    public:
        glm::dvec2 fieldSize() const noexcept
        {
            return m_fieldSize;
        }

        glm::dvec2 center() const noexcept
        {
            return m_center;
        }

        std::size_t observationHits() const noexcept
        {
            return m_observationHits;
        }
           
    public:
        PsfTextureRenderer() = default;
        ~PsfTextureRenderer();

        PsfTextureRenderer(
            const PsfTextureRenderer&) = delete;

        PsfTextureRenderer& operator=(
            const PsfTextureRenderer&) = delete;

        // Call on the main thread with a current OpenGL context,
        // after GLEW has been initialized.
        void render(
            const raytracer::TraceResult& result,
            const telescope::ObservationPlane& plane,
            const PsfRenderSettings& settings = {},
            const PsfColorFunction& color = {});

        // Call before destroying the OpenGL context.
        // The destructor also releases resources if necessary.
        void release() noexcept;

        unsigned int texture() const noexcept
        {
            return m_texture;
        }

        int width() const noexcept
        {
            return m_width;
        }

        int height() const noexcept
        {
            return m_height;
        }

    private:
        unsigned int m_texture = 0;

        int m_width = 0;
        int m_height = 0;

        glm::dvec2 m_fieldSize{ 0.0 };
        glm::dvec2 m_center{ 0.0 };

        std::size_t m_observationHits = 0;
    };
}