#include "Raytracer.h"
#include <algorithm>
#include <execution>

#include <cmath>
#include <type_traits>
#include <limits>
#include <variant>
#include <utility>
#include <iostream>

namespace opticforge::raytracer {


	//Top level. Trace the whole initial ray bundle provided by the system
	TraceResult RayTracer::traceRayBundle(const std::vector<optics::OpticalRay>& rayBundle,
		const std::vector<telescope::PrimitiveRecord>& scene,
		const telescope::ObservationPlane& observationPlane, 
		uint32_t maxInteractions) const
	{
		TraceResult result;

		// Create one output slot per input ray before parallel work starts.
		result.paths.resize(rayBundle.size());

		std::transform(
			std::execution::par,
			rayBundle.begin(),
			rayBundle.end(),
			result.paths.begin(),
			[this, &scene, &observationPlane, maxInteractions](const optics::OpticalRay& ray)
			{
				return traceRay(ray, scene, observationPlane,maxInteractions);
			});

		return result;
	}

	//Mid level. Trace the current ray to its completion across the whole scene
	RayPath RayTracer::traceRay(optics::OpticalRay ray,
		const std::vector<telescope::PrimitiveRecord>& scene,
		const telescope::ObservationPlane& observationPlane, 
		uint32_t max_interactions) const
	{
		RayPath path;
		path.interactions.reserve(
			std::min<std::uint32_t>(
				max_interactions,
				8));
		path.initialRay = ray;
		unsigned int interactions = 0;
		optics::OpticalRay currentRay = ray;
		int rayCount = 0; 
		while ((path.termination == RayTermination::Active)
			&& (interactions < max_interactions))
		{
			rayCount++; 
			if (auto intersection = findClosestIntersection(currentRay, scene, observationPlane))
			{
				//We got an interaction with something. 
				//Build the interaction structure and store it in the raypath.
				RayInteraction interaction{};
				interaction.hit = (*intersection).hit;
				interaction.primitiveId = (*intersection).primitiveId;
				interaction.incoming = currentRay;

				interactions++;
				OpticalResponse resp = handleInteraction(currentRay, (*intersection));

				path.termination = resp.termination;
				//Continue with a new ray or terminate?
				//If there is an outgoing ray, the ray did not terminate, so continue... 
				if (resp.outGoing) {
					interaction.outgoing = (*resp.outGoing);
					currentRay = (*resp.outGoing);
				}
				path.interactions.push_back(std::move(interaction));

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
		const std::vector<telescope::PrimitiveRecord>& scene,
		const telescope::ObservationPlane& observationPlane) const
	{
		// Project units are currently millimetres.
		// Eventually move this into TraceSettings.
		constexpr double minHitDistance = 1.0e-9;

		std::optional<RayIntersection> closest;
		double closestT = std::numeric_limits<double>::infinity();

		// Test the observation plane here.
		// World coordinates -> observation-plane coordinates.
		const optics::Ray planeRay(
			observationPlane.transform.worldToLocalPoint(
				ray.ray.origin),
			observationPlane.transform.worldToLocalDirection(
				ray.ray.direction));

		optics::SurfaceHit planeHit{};
		bool hitPlane = false;

		if (observationPlane.infiniteExtent)
		{
			// Bypass aperture clipping, while respecting the surface transform.
			const auto& surfaceTransform =
				observationPlane.surface.transform();

			const optics::Ray surfaceRay(
				surfaceTransform.worldToLocalPoint(planeRay.origin),
				surfaceTransform.worldToLocalDirection(planeRay.direction));

			optics::SurfaceHit localHit{};

			if (optics::PlaneGeometry{}.intersect(surfaceRay, localHit, minHitDistance))
			{
				planeHit.t = localHit.t;
				planeHit.position =
					surfaceTransform.localToWorldPoint(localHit.position);
				planeHit.normal =
					surfaceTransform.localToWorldNormal(localHit.normal);

				hitPlane = true;
			}
		}
		else
		{
			// Includes the surface transform and its configured aperture.
			hitPlane = observationPlane.surface.intersect(
				planeRay, planeHit);
		}
		//For now we don't support crossing the observation plane. It is the explicit backstop
       
		if (hitPlane
			&& std::isfinite(planeHit.t)
			&& planeHit.t > minHitDistance)
		{
				RayIntersection candidate{};
				//candidate.target = IntersectionTarget::ObservationPlane;
				candidate.primitiveId = 0;// std::nullopt;
				candidate.surface = &observationPlane.surface;

				// Observation-plane coordinates -> world coordinates.
				candidate.hit.t = planeHit.t;
				candidate.hit.position =
					observationPlane.transform.localToWorldPoint(
						planeHit.position);
				candidate.hit.normal =
					observationPlane.transform.localToWorldNormal(
						planeHit.normal);

				closestT = candidate.hit.t;
				closest = candidate;
		}
		

		// Replace closest only when a primitive hit has t < closestT.

		// Assumes ray.ray.direction is normalized, as required by Ray.
		for (const auto& record : scene)
		{
			std::visit(
				[&](const auto& primitive)
				{
					using Primitive =
						std::decay_t<decltype(primitive)>;

					// World coordinates -> primitive coordinates.
					const optics::Ray primitiveRay(
						primitive.transform.worldToLocalPoint(
							ray.ray.origin),
						primitive.transform.worldToLocalDirection(
							ray.ray.direction));

					const auto testSurface =
						[&](const optics::OpticalSurface& surface,
							const glm::dvec3& nominalOffset)
						{
							// Remove the nominal surface placement.
							// OpticalSurface::intersect will then remove
							// the surface's own rotation and translation.
							const optics::Ray surfaceParentRay(
								primitiveRay.origin - nominalOffset,
								primitiveRay.direction);

							optics::SurfaceHit parentHit{};

							if (!surface.intersect(
								surfaceParentRay, parentHit, minHitDistance))
							{
								return;
							}

							// All transforms are rigid, so t is preserved.
							if (!std::isfinite(parentHit.t)
								|| parentHit.t <= minHitDistance
								|| parentHit.t >= closestT)
							{
								return;
							}

							// intersect() has already applied the surface
							// transform to its hit. Restore nominal placement,
							// then apply the owning primitive's transform.
							optics::SurfaceHit worldHit{};
							worldHit.t = parentHit.t;

							worldHit.position =
								primitive.transform.localToWorldPoint(
									parentHit.position + nominalOffset);

							worldHit.normal =
								primitive.transform.localToWorldNormal(
									parentHit.normal);

							RayIntersection candidate{};

							// Matches the spelling in your current header.
							candidate.primitiveId = record.id;
							candidate.hit = worldHit;

							// Points to the original surface in the scene,
							// not a transformed temporary copy.
							candidate.surface = &surface;

							closestT = worldHit.t;
							closest = candidate;
						};
					const auto acceptAbsorbingHit =
						[&](const optics::SurfaceHit& worldHit)
						{
							if (!std::isfinite(worldHit.t)
								|| worldHit.t <= minHitDistance
								|| worldHit.t >= closestT)
							{
								return;
							}

							RayIntersection candidate{};

							candidate.primitiveId = record.id;
							candidate.hit = worldHit;

							candidate.behavior =
								IntersectionBehavior::Absorb;

							candidate.surface = nullptr;

							closestT = worldHit.t;
							closest = candidate;
						};

					if constexpr (
						std::is_same_v<Primitive, telescope::Lens>)
					{
						testSurface(
							primitive.frontSurface,
							glm::dvec3(0.0));

						testSurface(
							primitive.rearSurface,
							glm::dvec3(
								0.0,
								0.0,
								primitive.centerThickness));
					}
					else if constexpr (
						std::is_same_v<Primitive, telescope::Mirror>
						)
					{
						testSurface(
							primitive.surface,
							glm::dvec3(0.0));

					}
					else if constexpr (
						std::is_same_v<Primitive, telescope::Detector>)
					{
						testSurface(
							primitive.surface,
							glm::dvec3(0.0));
					}
					else
					{
						static_assert(
							std::is_same_v<Primitive, telescope::Lens>,
							"Add intersection handling for this primitive type");
					}
				},
				record.primitive);
		}

		return closest;
	}
	namespace
	{
		bool isFinite(const glm::dvec3& value)
		{
			return std::isfinite(value.x)
				&& std::isfinite(value.y)
				&& std::isfinite(value.z);
		}

		bool isValidDirection(const glm::dvec3& value)
		{
			const double lengthSquared = glm::dot(value, value);

			return isFinite(value)
				&& std::isfinite(lengthSquared)
				&& lengthSquared > 0.0;
		}

		optics::OpticalRay makeOutgoingRay(
			const optics::OpticalRay& incoming,
			const optics::SurfaceHit& hit,
			const glm::dvec3& direction,
			double intensity)
		{
			// Preserve wavelength and any other existing ray metadata.
			optics::OpticalRay outgoing = incoming;

			outgoing.ray.origin = hit.position;
			outgoing.ray.direction = glm::normalize(direction);
			outgoing.intensity = intensity;

			// Keep the physical origin at the intersection.
			// Self-intersection rejection belongs in the intersection query.
			return outgoing;
		}
	}

	RayTracer::OpticalResponse RayTracer::handleInteraction(
		const optics::OpticalRay& incoming,
		const RayIntersection& intersection) const
	{
		if (intersection.behavior == IntersectionBehavior::Absorb)
		{
			return {
				std::nullopt,
				RayTermination::Absorbed
			};
		}
		if (!intersection.surface
			|| !isFinite(intersection.hit.position)
			|| !isValidDirection(intersection.hit.normal)
			|| !isValidDirection(incoming.ray.direction)
			|| !std::isfinite(incoming.intensity)
			|| incoming.intensity < 0.0
			|| !std::isfinite(incoming.wavelength)
			|| incoming.wavelength <= 0.0)
		{
			return { std::nullopt, RayTermination::InvalidState };
		}

		const auto& interface =
			intersection.surface->opticalInterface().type();

		if (interface.valueless_by_exception())
		{
			return { std::nullopt, RayTermination::InvalidState };
		}

		return std::visit(
			[&](const auto& concrete) -> OpticalResponse
			{
				using T = std::decay_t<decltype(concrete)>;

				if constexpr (
					std::is_same_v<T, optics::ReflectiveInterface>)
				{
					return handleReflection(
						incoming, intersection.hit, concrete);
				}
				else if constexpr (
					std::is_same_v<T, optics::RefractiveInterface>)
				{
					return handleRefraction(
						incoming, intersection.hit, concrete);
				}
				else if constexpr (
					std::is_same_v<T, optics::DetectorInterface>)
				{
					return {
						std::nullopt,
						RayTermination::DetectorHit
					};
				}
				else
				{
					static_assert(
						std::is_same_v<T, optics::AbsorbingInterface>,
						"Unhandled optical interface type");

					return {
						std::nullopt,
						RayTermination::Absorbed
					};
				}
			},
			interface);
	}

	RayTracer::OpticalResponse RayTracer::handleReflection(
		const optics::OpticalRay& incoming,
		const optics::SurfaceHit& hit,
		const optics::ReflectiveInterface& interface) const
	{
		const double reflectivity = interface.reflectivity;

		// Validate here too: the public field can bypass the
		// ReflectiveInterface constructor's clamping.
		if (!std::isfinite(reflectivity)
			|| reflectivity < 0.0
			|| reflectivity > 1.0)
		{
			return { std::nullopt, RayTermination::InvalidState };
		}

		const double intensity =
			incoming.intensity * reflectivity;

		if (intensity == 0.0)
		{
			return { std::nullopt, RayTermination::Absorbed };
		}

		const glm::dvec3 direction =
			glm::normalize(incoming.ray.direction);

		const glm::dvec3 normal =
			glm::normalize(hit.normal);

		const glm::dvec3 reflected =
			direction
			- 2.0 * glm::dot(direction, normal) * normal;

		return {
			makeOutgoingRay(incoming, hit, reflected, intensity),
			RayTermination::Active
		};
	}

	RayTracer::OpticalResponse RayTracer::handleRefraction(
		const optics::OpticalRay& incoming,
		const optics::SurfaceHit& hit,
		const optics::RefractiveInterface& interface) const
	{
		// Despite their names, these currently store indices, not IDs.
		const double nNegative = interface.negativeSideMaterial;
		const double nPositive = interface.positiveSideMaterial;

		if (!std::isfinite(nNegative) || nNegative <= 0.0
			|| !std::isfinite(nPositive) || nPositive <= 0.0)
		{
			return { std::nullopt, RayTermination::InvalidState };
		}

		const glm::dvec3 direction =
			glm::normalize(incoming.ray.direction);

		const glm::dvec3 surfaceNormal =
			glm::normalize(hit.normal);

		// The surface normal points from negative to positive medium.
		const bool towardPositive =
			glm::dot(direction, surfaceNormal) > 0.0;

		const double nIncident =
			towardPositive ? nNegative : nPositive;

		const double nTransmitted =
			towardPositive ? nPositive : nNegative;

		// Snell's vector form uses a normal opposing the incoming ray.
		const glm::dvec3 normal =
			towardPositive ? -surfaceNormal : surfaceNormal;

		if (nIncident == nTransmitted)
		{
			return {
				makeOutgoingRay(
					incoming, hit, direction, incoming.intensity),
				RayTermination::Active
			};
		}

		const double eta = nIncident / nTransmitted;

		const double cosIncident = std::clamp(
			-glm::dot(direction, normal), 0.0, 1.0);

		const double sinTransmittedSquared =
			eta * eta
			* std::max(0.0, 1.0 - cosIncident * cosIncident);

		if (!std::isfinite(sinTransmittedSquared))
		{
			return { std::nullopt, RayTermination::InvalidState };
		}

		if (sinTransmittedSquared > 1.0)
		{
			// Total internal reflection continues the path.
			return handleReflection(
				incoming,
				hit,
				optics::ReflectiveInterface{ 1.0 });
		}

		const double cosTransmitted =
			std::sqrt(std::max(0.0, 1.0 - sinTransmittedSquared));

		const glm::dvec3 transmitted =
			eta * direction
			+ (eta * cosIncident - cosTransmitted) * normal;

		return {
			makeOutgoingRay(
				incoming, hit, transmitted, incoming.intensity),
			RayTermination::Active
		};
	}

}