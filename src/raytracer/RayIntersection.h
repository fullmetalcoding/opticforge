// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include "telescope/TelescopePrimitives.h"


namespace opticforge::raytracer
{
	enum class IntersectionBehavior
	{
		OpticalSurface,
		Absorb
	};
	struct RayIntersection {
		telescope::PrimitiveId primitiveId; //ID of the primitive we hit (Who did we hit?)
		optics::SurfaceHit hit; //Where did we hit?
		const optics::OpticalSurface* surface = nullptr; //What part did we hit?
		IntersectionBehavior behavior =
			IntersectionBehavior::OpticalSurface;
	};
}