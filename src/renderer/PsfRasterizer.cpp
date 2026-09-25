// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "PsfRasterizer.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace opticforge::renderer
{
	namespace
	{
		bool finite(const glm::dvec3& v)
		{
			return
				std::isfinite(v.x) &&
				std::isfinite(v.y) &&
				std::isfinite(v.z);
		}

		struct Sample
		{
			glm::dvec2 p;
			glm::dvec3 color;
			double weight;
		};
	}

	PsfImage rasterizePsf(
		const raytracer::TraceResult& result,
		const telescope::ObservationPlane& plane,
	    const PsfRenderSettings& s,
		const PsfColorFunction& color)
	{
		if (
			s.width < 1 ||
			s.height < 1 ||
			s.width > 2048 ||
			s.height > 2048 ||
			!std::isfinite(s.fieldWidth) ||
			s.fieldWidth <= 0.0 ||
			!std::isfinite(s.center.x) ||
			!std::isfinite(s.center.y) ||
			!std::isfinite(s.sigmaPixels) ||
			s.sigmaPixels < 0.25 ||
			s.sigmaPixels > 32.0 ||
			!std::isfinite(s.exposure) ||
			s.exposure < 0.0 ||
			s.exposure > 1e6)
		{
			throw std::invalid_argument(
				"Invalid PSF render settings");
		}

		const bool white =
			s.background == PsfBackground::White;

		const glm::dvec3 background(
			white ? 1.0 : 0.0);

		std::vector<Sample> samples;

		glm::dvec2 lo(0.0);
		glm::dvec2 hi(0.0);

		// Collect valid terminal observation-plane hits.
		for (const auto& path : result.paths)
		{
			if (
				path.termination !=
				raytracer::RayTermination::DetectorHit ||
				path.interactions.size() <2)
			{
				continue;
			}

			const auto& hit = path.interactions.back();

			if (
				hit.primitiveId != 0 ||
				!finite(hit.hit.position) ||
				!std::isfinite(hit.incoming.intensity) ||
				hit.incoming.intensity <= 0.0)
			{
				continue;
			}

			const auto local =
				plane.transform.worldToLocalPoint(
					hit.hit.position);

			if (!finite(local))
				continue;

			const auto rgb = color
				? color(path)
				: glm::dvec3(white ? 0.0 : 1.0);

			if (!finite(rgb))
				continue;

			const glm::dvec2 p(local.x, local.y);

			if (samples.empty())
			{
				lo = hi = p;
			}
			else
			{
				lo = glm::min(lo, p);
				hi = glm::max(hi, p);
			}

			samples.push_back({
				p,
				glm::clamp(rgb, 0.0, 1.0),
				hit.incoming.intensity
				});
		}

		PsfImage image;
		image.width = s.width;
		image.height = s.height;
		image.center = s.center;
		image.observationHits = samples.size();

		double width = s.fieldWidth;

		const double aspect =
			static_cast<double>(s.width) / s.height;

		// Automatically center the view on the intensity centroid.
		if (s.autoCenter && !samples.empty())
		{
			glm::dvec2 weightedPosition(0.0);
			double totalWeight = 0.0;

			for (const auto& sample : samples)
			{
				weightedPosition += sample.p * sample.weight;
				totalWeight += sample.weight;
			}

			if (totalWeight > 0.0)
			{
				image.center =
					weightedPosition / totalWeight;
			}
		}

		// Automatically enlarge the field enough to contain the spot.
		// fieldWidth remains the lower bound.
		if (s.autoFit && !samples.empty())
		{
			const double fittedWidth =
				1.2 * std::max(
					hi.x - lo.x,
					(hi.y - lo.y) * aspect);

			width = std::max(
				s.fieldWidth,
				fittedWidth);
				
		}
	
		// Preserve square physical pixels.
		image.fieldSize = {
			width,
			width / aspect
		};

		if (
			!std::isfinite(width) ||
			!std::isfinite(image.fieldSize.y) ||
			image.fieldSize.y <= 0.0)
		{
			throw std::invalid_argument(
				"PSF field is out of range");
		}

		const std::size_t count =
			static_cast<std::size_t>(s.width) * s.height;

		// XYZ = weighted RGB sum.
		// W   = total intensity.
		std::vector<glm::dvec4> accum(
			count,
			glm::dvec4(0.0));

		const auto add = [&](
			int x,
			int y,
			const Sample& sample,
			double fraction)
			{
				const double w =
					sample.weight * fraction;

				accum[
					static_cast<std::size_t>(y) * s.width + x
				] += glm::dvec4(sample.color * w, w);
			};

		const double radius =
			4.0 * s.sigmaPixels;

		const double divisor =
			std::sqrt(2.0) * s.sigmaPixels;

		for (const auto& sample : samples)
		{
			const auto pixel =
				(
					(sample.p - image.center) /
					image.fieldSize +
					0.5
					) * glm::dvec2(s.width, s.height);

			if (
				!std::isfinite(pixel.x) ||
				!std::isfinite(pixel.y))
			{
				continue;
			}

			if (s.mark == PsfMark::Point)
			{
				if (
					pixel.x >= 0 &&
					pixel.x < s.width &&
					pixel.y >= 0 &&
					pixel.y < s.height)
				{
					add(
						static_cast<int>(pixel.x),
						static_cast<int>(pixel.y),
						sample,
						1.0);
				}

				continue;
			}

			if (
				pixel.x < -radius ||
				pixel.x > s.width + radius ||
				pixel.y < -radius ||
				pixel.y > s.height + radius)
			{
				continue;
			}

			const int x0 = static_cast<int>(
				std::max(
					0.0,
					std::floor(pixel.x - radius)));

			const int x1 = static_cast<int>(
				std::min(
					static_cast<double>(s.width - 1),
					std::floor(pixel.x + radius)));

			const int y0 = static_cast<int>(
				std::max(
					0.0,
					std::floor(pixel.y - radius)));

			const int y1 = static_cast<int>(
				std::min(
					static_cast<double>(s.height - 1),
					std::floor(pixel.y + radius)));

			// Integrate a normalized Gaussian over each pixel.
			// This keeps total deposited intensity approximately
			// constant when sigma changes.
			std::vector<double> xWeights;

			for (int x = x0; x <= x1; ++x)
			{
				xWeights.push_back(
					0.5 * (
						std::erf(
							(x + 1.0 - pixel.x) / divisor) -
						std::erf(
							(x - pixel.x) / divisor)
						));
			}

			for (int y = y0; y <= y1; ++y)
			{
				const double fy =
					0.5 * (
						std::erf(
							(y + 1.0 - pixel.y) / divisor) -
						std::erf(
							(y - pixel.y) / divisor)
						);

				for (int x = x0; x <= x1; ++x)
				{
					add(
						x,
						y,
						sample,
						xWeights[
							static_cast<std::size_t>(x - x0)
						] * fy);
				}
			}
		}

		double peak = 0.0;

		for (const auto& a : accum)
			peak = std::max(peak, a.w);

		image.rgba.resize(count * 4);

		for (std::size_t i = 0; i < count; ++i)
		{
			const auto& a = accum[i];

			glm::dvec3 rgb = background;

			if (a.w > 0.0)
			{
				const double level = s.normalizePeak
					? a.w / peak
					: a.w;

				const double coverage =
					std::clamp(
						level * s.exposure,
						0.0,
						1.0);

				rgb =
					background * (1.0 - coverage) +
					glm::dvec3(a) / a.w * coverage;
			}

			for (int c = 0; c < 3; ++c)
			{
				image.rgba[i * 4 + c] =
					static_cast<std::uint8_t>(
						std::lround(
							std::clamp(rgb[c], 0.0, 1.0) *
							255.0));
			}

			image.rgba[i * 4 + 3] = 255;
		}

		return image;
	}
}