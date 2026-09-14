#include "RayPathGeometry.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace opticforge::renderer
{
    RayPathGeometry buildRayPathGeometry(const raytracer::TraceResult& result,
        const RayPathRenderSettings& settings, std::size_t maxSegments)
    {
        RayPathGeometry geometry;
        if (settings.maxRays <= 0 || result.paths.empty()) return geometry;
        const auto count = std::min(result.paths.size(),
            static_cast<std::size_t>(settings.maxRays));
        // Keep byte sizes and draw counts representable for the GL upload.
        maxSegments = std::min<std::size_t>(maxSegments, 1000000);
        const auto append = [&](const glm::dvec3& from, const glm::dvec3& to,
            float category)
            {
                if (geometry.vertices.size() / 2 >= maxSegments)
                {
                    geometry.truncated = true;
                    return;
                }
                const auto valid = [](const glm::dvec3& p)
                    {
                        const double limit = std::numeric_limits<float>::max();
                        return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z)
                            && std::abs(p.x * 0.001) < limit
                            && std::abs(p.y * 0.001) < limit
                            && std::abs(p.z * 0.001) < limit;
                    };
                if (!valid(from) || !valid(to)) return;
                geometry.vertices.push_back({ glm::vec3(from * 0.001), category });
                geometry.vertices.push_back({ glm::vec3(to * 0.001), category });
            };

        for (std::size_t i = 0; i < count; ++i)
        {
            // Equivalent to floor(i * total / count), avoiding integer overflow.
            const auto index = i * (result.paths.size() / count)
                + (i * (result.paths.size() % count)) / count;
            const auto& path = result.paths[index];
            const bool escaped = path.termination == raytracer::RayTermination::Escaped
                || path.termination == raytracer::RayTermination::Missed;
            const bool observed = path.termination == raytracer::RayTermination::DetectorHit
                && !path.interactions.empty()
                && path.interactions.back().primitiveId == 0; // Observation-plane sentinel.
            const float category = observed ? 0.0f : escaped ? 1.0f : 2.0f;
            auto point = path.initialRay.ray.origin;
            const auto before = geometry.vertices.size();
            for (const auto& interaction : path.interactions)
            {
                append(point, interaction.hit.position, category);
                point = interaction.hit.position;
                if (geometry.truncated) break;
            }
            // Escaped rays have no final hit. Extend only these, never rays
            // absorbed by a surface or stopped by the interaction limit.
            if (escaped && !geometry.truncated && std::isfinite(settings.escapeLengthMm)
                && settings.escapeLengthMm > 0.0)
            {
                const optics::OpticalRay* outgoing = &path.initialRay;
                if (!path.interactions.empty())
                    outgoing = path.interactions.back().outgoing
                    ? &*path.interactions.back().outgoing : nullptr;
                if (outgoing)
                {
                    const double length = glm::length(outgoing->ray.direction);
                    if (std::isfinite(length) && length > 0.0)
                        append(point, point + outgoing->ray.direction / length
                            * settings.escapeLengthMm, category);
                }
            }
            if (geometry.vertices.size() != before) ++geometry.displayedRays;
            if (geometry.truncated) break;
        }
        return geometry;
    }
}
