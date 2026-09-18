#pragma once

#include "raytracer/TraceResult.h"

#include <cstdint>
#include <functional>
#include <vector>

namespace opticforge::renderer
{
	enum class PsfBackground
	{
		Black,
		White
	};

	enum class PsfMark
	{
		Point,
		Gaussian
	};

	struct PsfRenderSettings
	{
		int width = 512;
		int height = 512;

		PsfBackground background = PsfBackground::Black;
		PsfMark mark = PsfMark::Gaussian;

		// Gaussian standard deviation in texture pixels.
		double sigmaPixels = 1.25;

		bool autoFit = true;
		bool autoCenter = true;

		// Horizontal display width.
		// With autoFit enabled, this is the minimum fitted width.
		// With autoFit disabled, this is the exact display width.
		double fieldWidth = 0.01;

		// Used when autoCenter is disabled.
		glm::dvec2 center{ 0.0 };

		bool normalizePeak = true;
		double exposure = 1.0;
	};

	// Return display RGB in [0, 1].
	//
	// The callback can inspect the incoming wavelength, intensity,
	// and other information in the path.
	//
	// Without a callback:
	//   Black background -> white marks.
	//   White background -> black marks.
	using PsfColorFunction =
		std::function<glm::dvec3(const raytracer::RayPath&)>;

	struct PsfImage
	{
		int width = 0;
		int height = 0;

		glm::dvec2 center{ 0.0 };
		glm::dvec2 fieldSize{ 0.0 };

		std::size_t observationHits = 0;

		// Opaque RGBA8.
		// First row corresponds to local -Y: OpenGL's bottom row.
		std::vector<std::uint8_t> rgba;
	};

	// Does not require an OpenGL context.
	//
	// The current raytracer identifies the observation plane using
	// primitiveId == 0. Only terminal DetectorHit paths at that
	// primitive are included.
	PsfImage rasterizePsf(
		const raytracer::TraceResult& result,
		const telescope::ObservationPlane& plane,
		const PsfRenderSettings& settings = {},
		const PsfColorFunction& color = {});
}