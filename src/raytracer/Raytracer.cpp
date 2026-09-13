#include "Raytracer.h"
#include <algorithm>
#include <execution>

namespace opticforge::raytracer {


	//Top level. Trace the whole initial ray bundle provided by the system
	TraceResult RayTracer::traceRayBundle(const std::vector<optics::OpticalRay>& rayBundle, 
		const std::vector<telescope::PrimitiveRecord>& scene) const
	{
		TraceResult result;

		// Create one output slot per input ray before parallel work starts.
		result.paths.resize(rayBundle.size());

		std::transform(
			std::execution::par,
			rayBundle.begin(),
			rayBundle.end(),
			result.paths.begin(),
			[this, &scene](const optics::OpticalRay& ray)
			{
				return traceRay(ray, scene);
			});

		return result;
	}
	const uint32_t MAX_INTERACTIONS = 1000;
	//Mid level. Trace the current ray to its completion across the whole scene
	RayPath RayTracer::traceRay(optics::OpticalRay ray, 
		const std::vector<telescope::PrimitiveRecord>& scene) const
	{
		RayPath path;
		path.initialRay = ray;
		unsigned int interactions = 0;
		optics::OpticalRay currentRay = ray;
		while ((path.termination == RayTermination::Active)
			&& (interactions < MAX_INTERACTIONS))
		{
			if (auto intersection = findClosestIntersection(currentRay, scene))
			{
				//We got an interaction with something. 
				//Build the interaction structure and store it in the raypath.
				RayInteraction interaction; 
				interaction.hit = (*intersection).hit;
				interaction.primitiveId = (*intersection).primtiveId;
				interaction.incoming = currentRay; 
				
				interactions++; 
				//Continue with a new ray or terminate?
				//Handle the interaction here, using an overloaded method that can work with
				// std::variant<OpticalSurface> 
				//If we build a new ray, then currentRay becomes the new ray.
			}
			else {
				//This ray did not interact with anything and thus escaped. 
				path.termination = RayTermination::Escaped; 
			}
		}
		if (path.termination == RayTermination::Active) {
			//Only way for this to happen is if we exceed max interactions... 
			path.termination = RayTermination::MaxInteractions;
		}

		return path; 
	}

	//Lowest level. Find the closest interaction for the current ray
	std::optional<RayIntersection> RayTracer::findClosestIntersection(
		const optics::OpticalRay& ray,
		const std::vector<telescope::PrimitiveRecord>& scene) const
	{
		RayIntersection closestIntersection;
		for (auto& primitive : scene) {
			

		}

		return {};
	}
}