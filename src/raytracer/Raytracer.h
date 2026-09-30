// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once
#include "telescope/TelescopePrimitives.h"
#include "TraceResult.h"
#include "RayInteraction.h"
#include "RayIntersection.h"
#include "RayPath.h"
#include "optics/MaterialLibrary.h"

#include <optional>
#include <vector> 

namespace opticforge::raytracer {
	class RayTracer
	{
	private:
		struct OpticalResponse
		{
			std::optional<optics::OpticalRay> outGoing;
			RayTermination termination = RayTermination::Active;

		};
	public:
        RayTracer() = default;

        explicit RayTracer(
            const optics::MaterialLibrary& materials)
            : m_materialLibrary(&materials)
        {
        }

		TraceResult traceRayBundle(const std::vector<optics::OpticalRay>& rayBundle,
			const std::vector<telescope::PrimitiveRecord>& scene,
			const telescope::ObservationPlane& observationPlane,
			uint32_t max_interactions = 1000) const;

		RayPath traceRay(optics::OpticalRay ray,
			const std::vector<telescope::PrimitiveRecord>& scene,
			const telescope::ObservationPlane& observationPlane,
			uint32_t max_interactions = 1000) const;
	private:



		std::optional<RayIntersection> findClosestIntersection(
			const optics::OpticalRay& ray,
			const std::vector<telescope::PrimitiveRecord>& scene,
			const telescope::ObservationPlane& observationPlane) const;

		OpticalResponse handleInteraction(
			const optics::OpticalRay& incoming,
			const RayIntersection& intersection) const;

		OpticalResponse handleReflection(
			const optics::OpticalRay& incoming,
			const optics::SurfaceHit& hit,
			const optics::ReflectiveInterface& interface) const;

		OpticalResponse handleRefraction(
			const optics::OpticalRay& incoming,
			const optics::SurfaceHit& hit,
			const optics::RefractiveInterface& interface) const;

        OpticalResponse handleDiffractionGrating(
            const optics::OpticalRay& incoming,
            const RayIntersection& intersection,
            const optics::DiffractionGratingInterface& interface) const;

        const optics::MaterialLibrary*
            m_materialLibrary = nullptr;

	};
}