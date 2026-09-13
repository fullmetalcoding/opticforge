#pragma once

namespace opticforge::raytracer {
	class Raytracer
	{
	public:

		TraceResult traceRayBundle(const std::vector<optics::OpticalRay>& rayBundle, const telescope::TelescopeProject& project) const;
	private:

	};
}