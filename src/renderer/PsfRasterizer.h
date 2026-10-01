// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include "raytracer/TraceResult.h"

#include <atomic>
#include <cstdint>
#include <exception>
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
		double fieldWidth = 1.94048; 
		double pixelSizeMicrons = 3.79;

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
	// Without a callback, the renderer accumulates CIE XYZ spectral
	// contributions carried by the incoming rays (falling back to the
	// monochromatic CIE response for rays without band metadata), then
	// converts the mixed result to sRGB.
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

	// Execution options for the CPU rasterizer.
	struct PsfExecution
	{
		// 0 = automatic (hardware_concurrency - 1, minimum 1).
		// 1 = run serially on the calling thread.
		unsigned int threads = 0;

		// Polled between work chunks. When set, the call throws
		// PsfCancelled. May be null.
		const std::atomic<bool>* cancel = nullptr;

		// For large Gaussian marks (sigma >= 2 px and enough rays that it
		// pays off), splat each ray bilinearly and blur the whole image
		// with a separable pixel-integrated Gaussian instead of
		// integrating a (8 sigma)^2 footprint per ray. Matches the exact
		// result to within ~1/255 per channel; set false for bit-exact.
		bool allowSeparable = true;
	};

	struct PsfCancelled : std::exception
	{
		const char* what() const noexcept override
		{
			return "PSF rasterization cancelled";
		}
	};

	// Intermediate, linear accumulation buffer.
	//
	// Depends only on the trace result and on the "geometry" settings
	// (size, mark, sigma, fit/center/field). Background, exposure and
	// normalizePeak are applied afterwards by tonemapPsf(), so changing
	// those does not require re-splatting every ray.
	struct PsfAccumulation
	{
		int width = 0;
		int height = 0;

		glm::dvec2 center{ 0.0 };
		glm::dvec2 fieldSize{ 0.0 };

		std::size_t observationHits = 0;

		// True when a PsfColorFunction supplied display RGB.
		bool customColor = false;

		// Max of accum[i].w.
		double peak = 0.0;

		// xyz = colour * weight (XYZ, or RGB for custom colour), w = weight.
		std::vector<glm::dvec4> accum;
	};

	// True when two settings produce the same PsfAccumulation.
	bool psfAccumulationSettingsEqual(
		const PsfRenderSettings& a,
		const PsfRenderSettings& b) noexcept;

	// Stage 1: collect observation hits and splat them. This is the
	// expensive part (O(hits * footprint)). Thread-safe; no OpenGL.
	PsfAccumulation accumulatePsf(
		const raytracer::TraceResult& result,
		const telescope::ObservationPlane& plane,
		const PsfRenderSettings& settings = {},
		const PsfColorFunction& color = {},
		const PsfExecution& execution = {});

	// Stage 2: convert the accumulation to display RGBA8.
	// O(pixels); cheap enough to re-run for every exposure slider tick.
	PsfImage tonemapPsf(
		const PsfAccumulation& accumulation,
		const PsfRenderSettings& settings,
		const PsfExecution& execution = {});

	// Does not require an OpenGL context.
	//
	// The current raytracer identifies the observation plane using
	// primitiveId == 0. Only terminal DetectorHit paths at that
	// primitive are included.
	//
	// Equivalent to tonemapPsf(accumulatePsf(...)). Output is identical
	// for every thread count.
	PsfImage rasterizePsf(
		const raytracer::TraceResult& result,
		const telescope::ObservationPlane& plane,
		const PsfRenderSettings& settings = {},
		const PsfColorFunction& color = {},
		const PsfExecution& execution = {});
}