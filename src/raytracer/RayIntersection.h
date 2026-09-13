#pragma once

#include "telescope/TelescopePrimitives.h"


namespace opticforge::raytracer
{
	struct RayIntersection {
		telescope::PrimitiveId primtiveId; //ID of the primitive we hit (Who did we hit?)
		optics::SurfaceHit hit; //Where did we hit?
		const optics::OpticalSurface* surface = nullptr; //What part did we hit?
	};
}