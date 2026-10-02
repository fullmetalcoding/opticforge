// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "PsfRasterizer.h"
#include "optics/Colorimetry.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <mutex>
#include <stdexcept>
#include <thread>

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

		// Pixel-space footprint of one sample. Computed once, reused by
		// every row band the footprint overlaps.
		struct Footprint
		{
			double px = 0.0;
			double py = 0.0;
			int x0 = 0;
			int x1 = -1; // x1 < x0: sample contributes nothing.
			int y0 = 0;
			int y1 = -1;
		};

		// sigmaPixels <= 32  ->  radius <= 128  ->  at most 258 columns.
		constexpr std::size_t MaxTaps = 264;

		unsigned int resolveThreads(
			const PsfExecution& execution,
			std::size_t work)
		{
			unsigned int threads = execution.threads;

			if (threads == 0)
			{
				const unsigned int hardware =
					std::max(1u, std::thread::hardware_concurrency());

				// Same policy as TraceController: leave one logical CPU
				// for the UI/render thread.
				threads = hardware > 1 ? hardware - 1 : 1;
			}

			return static_cast<unsigned int>(
				std::max<std::size_t>(
					1,
					std::min<std::size_t>(threads, work)));
		}

		void throwIfCancelled(const PsfExecution& execution)
		{
			if (
				execution.cancel &&
				execution.cancel->load(std::memory_order_relaxed))
			{
				throw PsfCancelled{};
			}
		}

		// Monotonic progress reporting into PsfExecution::progress.
		//
		// A call is divided into stages; stage() maps the following
		// advance() calls onto [begin, end] of this call's share of the
		// progress range. stage() must be called between parallel
		// sections; advance() is thread-safe.
		class Progress
		{
		public:
			explicit Progress(const PsfExecution& execution)
				: m_out(execution.progress),
				m_low(execution.progressBegin),
				m_high(execution.progressEnd)
			{
			}

			void stage(double begin, double end, std::size_t total)
			{
				m_begin = begin;
				m_end = end;
				m_total = std::max<std::size_t>(total, 1);
				m_done.store(0, std::memory_order_relaxed);
				publish(begin);
			}

			void advance(std::size_t amount)
			{
				if (!m_out || amount == 0)
					return;

				const std::size_t done =
					m_done.fetch_add(amount, std::memory_order_relaxed) +
					amount;

				publish(
					m_begin +
					(m_end - m_begin) *
					std::min(
						1.0,
						static_cast<double>(done) /
						static_cast<double>(m_total)));
			}

			void finish()
			{
				publish(1.0);
			}

		private:
			void publish(double fraction)
			{
				if (!m_out)
					return;

				const double value =
					m_low + (m_high - m_low) * fraction;

				double current = m_out->load(std::memory_order_relaxed);

				while (
					value > current &&
					!m_out->compare_exchange_weak(
						current,
						value,
						std::memory_order_relaxed))
				{
				}
			}

			std::atomic<double>* m_out;
			double m_low;
			double m_high;

			double m_begin = 0.0;
			double m_end = 1.0;
			std::size_t m_total = 1;
			std::atomic<std::size_t> m_done{ 0 };
		};

		// Dynamic parallel-for over [0, count). Each index is executed
		// exactly once; the first exception is rethrown on the caller.
		template <typename Fn>
		void parallelFor(
			std::size_t count,
			unsigned int threads,
			Fn&& fn)
		{
			if (count == 0)
				return;

			threads = static_cast<unsigned int>(
				std::min<std::size_t>(threads, count));

			if (threads <= 1)
			{
				for (std::size_t i = 0; i < count; ++i)
					fn(i);

				return;
			}

			std::atomic<std::size_t> next{ 0 };
			std::atomic<bool> stop{ false };
			std::mutex errorMutex;
			std::exception_ptr error;

			const auto worker = [&]()
				{
					try
					{
						for (;;)
						{
							if (stop.load(std::memory_order_relaxed))
								return;

							const std::size_t i =
								next.fetch_add(1, std::memory_order_relaxed);

							if (i >= count)
								return;

							fn(i);
						}
					}
					catch (...)
					{
						std::lock_guard<std::mutex> lock(errorMutex);

						if (!error)
							error = std::current_exception();

						stop.store(true, std::memory_order_relaxed);
					}
				};

			std::vector<std::thread> pool;
			pool.reserve(threads - 1);

			try
			{
				for (unsigned int t = 1; t < threads; ++t)
					pool.emplace_back(worker);
			}
			catch (...)
			{
				// Thread creation failed: finish with what we have.
			}

			worker();

			for (auto& thread : pool)
				thread.join();

			if (error)
				std::rethrow_exception(error);
		}

		void validate(const PsfRenderSettings& s)
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
		}

		// Returns false if the path is not a usable observation hit.
		bool makeSample(
			const raytracer::RayPath& path,
			const telescope::ObservationPlane& plane,
			const PsfColorFunction& color,
			Sample& out)
		{
			if (
				path.termination !=
				raytracer::RayTermination::DetectorHit ||
				path.interactions.size() < 2)
			{
				return false;
			}

			const auto& hit = path.interactions.back();

			if (
				hit.primitiveId != 0 ||
				!finite(hit.hit.position) ||
				!std::isfinite(hit.incoming.intensity) ||
				hit.incoming.intensity <= 0.0)
			{
				return false;
			}

			const auto local =
				plane.transform.worldToLocalPoint(
					hit.hit.position);

			if (!finite(local))
				return false;

			glm::dvec3 sampleColor;

			if (color)
			{
				sampleColor =
					glm::clamp(
						color(path),
						0.0,
						1.0);
			}
			else if (
				hit.incoming.cieXyzPerUnitPower &&
				finite(
					*hit.incoming.cieXyzPerUnitPower))
			{
				sampleColor =
					*hit.incoming.cieXyzPerUnitPower;
			}
			else
			{
				sampleColor =
					optics::cie1931Xyz(
						hit.incoming.wavelength);

				// Keep non-visible diagnostic rays visible instead of
				// silently dropping them from a spot diagram.
				if (
					sampleColor.x <= 0.0 &&
					sampleColor.y <= 0.0 &&
					sampleColor.z <= 0.0)
				{
					sampleColor =
						glm::dvec3(
							0.95047,
							1.0,
							1.08883);
				}
			}

			if (!finite(sampleColor))
				return false;

			out = {
				glm::dvec2(local.x, local.y),
				sampleColor,
				hit.incoming.intensity
			};

			return true;
		}

		// Collect valid terminal observation-plane hits, preserving path
		// order (so accumulation order, and therefore output, is identical
		// to the serial implementation).
		std::vector<Sample> collectSamples(
			const raytracer::TraceResult& result,
			const telescope::ObservationPlane& plane,
			const PsfColorFunction& color,
			const PsfExecution& execution,
			Progress& progress,
			double progressEnd)
		{
			const auto& paths = result.paths;

			constexpr std::size_t ChunkSize = 4096;

			const std::size_t chunks =
				(paths.size() + ChunkSize - 1) / ChunkSize;

			// A user-supplied colour callback is not required to be
			// thread-safe, so keep that path on one thread.
			const unsigned int threads =
				color
				? 1u
				: resolveThreads(execution, chunks);

			std::vector<std::vector<Sample>> partial(chunks);

			progress.stage(0.0, progressEnd, chunks);

			parallelFor(
				chunks,
				threads,
				[&](std::size_t c)
				{
					throwIfCancelled(execution);

					const std::size_t begin = c * ChunkSize;
					const std::size_t end =
						std::min(paths.size(), begin + ChunkSize);

					auto& out = partial[c];
					out.reserve(end - begin);

					Sample sample;

					for (std::size_t i = begin; i < end; ++i)
					{
						if (makeSample(paths[i], plane, color, sample))
							out.push_back(sample);
					}

					progress.advance(1);
				});

			std::size_t total = 0;

			for (const auto& p : partial)
				total += p.size();

			std::vector<Sample> samples;
			samples.reserve(total);

			for (auto& p : partial)
			{
				samples.insert(samples.end(), p.begin(), p.end());
				std::vector<Sample>().swap(p);
			}

			return samples;
		}

		// Large-sigma path.
		//
		// 1. Splat each ray bilinearly (4 pixels) into a padded impulse
		//    image. Bilinear splatting preserves each ray's sub-pixel
		//    centroid and adds variance t(1-t) <= 1/4 (1/6 on average).
		// 2. Blur with a separable, pixel-integrated Gaussian whose
		//    variance is reduced by 1/6 to compensate.
		//
		// Cost is O(rays * 4 + nonEmptyPixels * taps + W * H * taps)
		// instead of O(rays * taps^2).
		void accumulateSeparable(
			const std::vector<Sample>& samples,
			const PsfRenderSettings& s,
			PsfAccumulation& acc,
			const PsfExecution& execution,
			Progress& progress,
			double progressBegin)
		{
			// Rough cost split of the three stages below.
			const double span = 1.0 - progressBegin;
			const double splatEnd = progressBegin + 0.1 * span;
			const double horizontalEnd = progressBegin + 0.4 * span;

			const double sigma = s.sigmaPixels;
			const double radius = 4.0 * sigma;

			const int R = static_cast<int>(std::ceil(radius)) + 1;
			const int pad = R + 1;

			const int W = s.width;
			const int H = s.height;
			const int PW = W + 2 * pad;
			const int PH = H + 2 * pad;

			std::vector<glm::dvec4> impulse(
				static_cast<std::size_t>(PW) * PH,
				glm::dvec4(0.0));

			std::vector<unsigned char> rowUsed(
				static_cast<std::size_t>(PH), 0);

			progress.stage(progressBegin, splatEnd, samples.size());

			for (std::size_t i = 0; i < samples.size(); ++i)
			{
				if ((i & 4095u) == 0)
				{
					throwIfCancelled(execution);
					progress.advance(i == 0 ? 0 : 4096);
				}

				const auto& sample = samples[i];

				const auto pixel =
					(
						(sample.p - acc.center) /
						acc.fieldSize +
						0.5
						) * glm::dvec2(W, H);

				if (
					!std::isfinite(pixel.x) ||
					!std::isfinite(pixel.y) ||
					pixel.x < -radius ||
					pixel.x > W + radius ||
					pixel.y < -radius ||
					pixel.y > H + radius)
				{
					continue;
				}

				// Padded coordinates of the ray relative to pixel centres.
				const double fx = pixel.x - 0.5 + pad;
				const double fy = pixel.y - 0.5 + pad;

				const int ix = static_cast<int>(std::floor(fx));
				const int iy = static_cast<int>(std::floor(fy));

				if (ix < 0 || iy < 0 || ix + 1 >= PW || iy + 1 >= PH)
					continue;

				const double tx = fx - ix;
				const double ty = fy - iy;

				const glm::dvec4 v(
					sample.color * sample.weight,
					sample.weight);

				glm::dvec4* p =
					impulse.data() +
					static_cast<std::size_t>(iy) * PW + ix;

				p[0] += v * ((1.0 - tx) * (1.0 - ty));
				p[1] += v * (tx * (1.0 - ty));
				p[PW] += v * ((1.0 - tx) * ty);
				p[PW + 1] += v * (tx * ty);

				rowUsed[static_cast<std::size_t>(iy)] = 1;
				rowUsed[static_cast<std::size_t>(iy) + 1] = 1;
			}

			// Pixel-integrated kernel. K[k + R] is the weight a pixel
			// receives from an impulse centred k pixels away.
			const double kernelSigma =
				std::sqrt(sigma * sigma - 1.0 / 6.0);

			const double d = std::sqrt(2.0) * kernelSigma;

			std::vector<double> K(static_cast<std::size_t>(2 * R + 1));

			for (int k = -R; k <= R; ++k)
			{
				K[static_cast<std::size_t>(k + R)] =
					0.5 * (
						std::erf((k + 0.5) / d) -
						std::erf((k - 0.5) / d));
			}

			const unsigned int threads =
				resolveThreads(execution, static_cast<std::size_t>(PH));

			// Pass 1: horizontal. Scatter only non-zero impulses; most of
			// a PSF image is background.
			// tmp has PH (padded) rows of W output columns.
			std::vector<glm::dvec4> tmp(
				static_cast<std::size_t>(PH) * W,
				glm::dvec4(0.0));

			progress.stage(
				splatEnd,
				horizontalEnd,
				static_cast<std::size_t>(PH));

			parallelFor(
				static_cast<std::size_t>(PH),
				threads,
				[&](std::size_t y)
				{
					progress.advance(1);

					if (!rowUsed[y])
						return;

					if ((y & 15u) == 0)
						throwIfCancelled(execution);

					const glm::dvec4* src =
						impulse.data() + y * static_cast<std::size_t>(PW);

					glm::dvec4* dst =
						tmp.data() + y * static_cast<std::size_t>(W);

					for (int px = 0; px < PW; ++px)
					{
						const glm::dvec4 v = src[px];

						if (v.w == 0.0)
							continue;

						// Output column x corresponds to padded column
						// x + pad; offset k = x + pad - px.
						const int xBegin = std::max(0, px - pad - R);
						const int xEnd = std::min(W - 1, px - pad + R);

						for (int x = xBegin; x <= xEnd; ++x)
						{
							dst[x] +=
								v *
								K[static_cast<std::size_t>(x + pad - px + R)];
						}
					}
				});

			// Pass 2: vertical, one output row per task.
			progress.stage(horizontalEnd, 1.0, static_cast<std::size_t>(H));

			parallelFor(
				static_cast<std::size_t>(H),
				threads,
				[&](std::size_t y)
				{
					if ((y & 15u) == 0)
						throwIfCancelled(execution);

					progress.advance(1);

					glm::dvec4* dst =
						acc.accum.data() + y * static_cast<std::size_t>(W);

					for (int k = -R; k <= R; ++k)
					{
						const int srcRow =
							static_cast<int>(y) + pad - k;

						if (
							srcRow < 0 ||
							srcRow >= PH ||
							!rowUsed[static_cast<std::size_t>(srcRow)])
						{
							continue;
						}

						const double weight =
							K[static_cast<std::size_t>(k + R)];

						const glm::dvec4* src =
							tmp.data() +
							static_cast<std::size_t>(srcRow) * W;

						for (int x = 0; x < W; ++x)
							dst[x] += src[x] * weight;
					}
				});

			double peak = 0.0;

			for (const auto& a : acc.accum)
				peak = std::max(peak, a.w);

			acc.peak = peak;
		}
	}

	bool psfAccumulationSettingsEqual(
		const PsfRenderSettings& a,
		const PsfRenderSettings& b) noexcept
	{
		if (
			a.width != b.width ||
			a.height != b.height ||
			a.mark != b.mark ||
			a.autoFit != b.autoFit ||
			a.autoCenter != b.autoCenter ||
			a.fieldWidth != b.fieldWidth)
		{
			return false;
		}

		if (
			a.mark == PsfMark::Gaussian &&
			a.sigmaPixels != b.sigmaPixels)
		{
			return false;
		}

		if (!a.autoCenter && a.center != b.center)
			return false;

		return true;
	}

	PsfAccumulation accumulatePsf(
		const raytracer::TraceResult& result,
		const telescope::ObservationPlane& plane,
		const PsfRenderSettings& s,
		const PsfColorFunction& color,
		const PsfExecution& execution)
	{
		validate(s);

		Progress progress(execution);

		// Share of progress for walking the trace result. Measured cost is
		// roughly 20 pixel-updates per path; splatting costs one per
		// footprint pixel (direct) or per pass element (separable).
		const bool gaussian = s.mark == PsfMark::Gaussian;

		const double radius =
			4.0 * s.sigmaPixels;

		const bool separable =
			gaussian &&
			execution.allowSeparable &&
			s.sigmaPixels >= 2.0;

		const double taps = 2.0 * std::ceil(radius) + 3.0;

		const double paths =
			std::max<double>(1.0, static_cast<double>(result.paths.size()));

		const double directCost =
			gaussian ? paths * taps * taps : paths;

		const double separableCost =
			2.0 * (s.width + taps) * (s.height + taps) * taps +
			4.0 * paths;

		const double splatCost =
			separable && directCost > 2.0 * separableCost
			? separableCost
			: directCost;

		const double collectShare =
			std::clamp(
				20.0 * paths / (20.0 * paths + splatCost),
				0.05,
				0.6);

		const std::vector<Sample> samples =
			collectSamples(
				result,
				plane,
				color,
				execution,
				progress,
				collectShare);

		throwIfCancelled(execution);

		PsfAccumulation acc;
		acc.width = s.width;
		acc.height = s.height;
		acc.center = s.center;
		acc.observationHits = samples.size();
		acc.customColor = static_cast<bool>(color);

		glm::dvec2 lo(0.0);
		glm::dvec2 hi(0.0);

		if (!samples.empty())
		{
			lo = hi = samples.front().p;

			for (const auto& sample : samples)
			{
				lo = glm::min(lo, sample.p);
				hi = glm::max(hi, sample.p);
			}
		}

		double width = s.fieldWidth;

		const double aspect =
			static_cast<double>(s.width) / s.height;

		// Automatically center the view on the intensity centroid.
		// Sequential sum: keeps the result bit-identical to the
		// original serial implementation.
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
				acc.center =
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
		acc.fieldSize = {
			width,
			width / aspect
		};

		if (
			!std::isfinite(width) ||
			!std::isfinite(acc.fieldSize.y) ||
			acc.fieldSize.y <= 0.0)
		{
			throw std::invalid_argument(
				"PSF field is out of range");
		}

		const std::size_t count =
			static_cast<std::size_t>(s.width) * s.height;

		acc.accum.assign(count, glm::dvec4(0.0));

		const double divisor =
			std::sqrt(2.0) * s.sigmaPixels;

		if (
			gaussian &&
			execution.allowSeparable &&
			s.sigmaPixels >= 2.0)
		{
			// Cost model: direct splatting touches (2r+1)^2 pixels per
			// sample; separable touches 4 per sample plus two 1-D passes
			// over the (padded) image.
			const double direct =
				static_cast<double>(samples.size()) * taps * taps;

			const double separable =
				2.0 *
				(s.width + taps) * (s.height + taps) * taps +
				4.0 * static_cast<double>(samples.size());

			if (direct > 2.0 * separable)
			{
				accumulateSeparable(
					samples,
					s,
					acc,
					execution,
					progress,
					collectShare);

				progress.finish();
				return acc;
			}
		}
		// --------------------------------------------------------------
		// Footprints (independent per sample).
		// --------------------------------------------------------------

		std::vector<Footprint> footprints(samples.size());

		for (std::size_t i = 0; i < samples.size(); ++i)
		{
			const auto& sample = samples[i];
			auto& f = footprints[i];

			const auto pixel =
				(
					(sample.p - acc.center) /
					acc.fieldSize +
					0.5
					) * glm::dvec2(s.width, s.height);

			if (
				!std::isfinite(pixel.x) ||
				!std::isfinite(pixel.y))
			{
				continue;
			}

			f.px = pixel.x;
			f.py = pixel.y;

			if (!gaussian)
			{
				if (
					pixel.x >= 0 &&
					pixel.x < s.width &&
					pixel.y >= 0 &&
					pixel.y < s.height)
				{
					f.x0 = f.x1 = static_cast<int>(pixel.x);
					f.y0 = f.y1 = static_cast<int>(pixel.y);
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

			f.x0 = static_cast<int>(
				std::max(0.0, std::floor(pixel.x - radius)));

			f.x1 = static_cast<int>(
				std::min(
					static_cast<double>(s.width - 1),
					std::floor(pixel.x + radius)));

			f.y0 = static_cast<int>(
				std::max(0.0, std::floor(pixel.y - radius)));

			f.y1 = static_cast<int>(
				std::min(
					static_cast<double>(s.height - 1),
					std::floor(pixel.y + radius)));
		}

		// --------------------------------------------------------------
		// Row bands.
		//
		// Each band owns a contiguous block of rows, so workers never
		// write the same pixel and no locking or per-thread image copies
		// are needed. Samples are binned into every band their footprint
		// overlaps, in original order, so each pixel sums its
		// contributions in exactly the serial order.
		// --------------------------------------------------------------

		const unsigned int threads =
			resolveThreads(execution, static_cast<std::size_t>(s.height));

		// Several bands per thread so a dense PSF core does not leave one
		// thread with all of the work.
		const int bandRows =
			threads <= 1
			? s.height
			: std::max(
				8,
				s.height / static_cast<int>(threads * 8));

		const int bandCount =
			(s.height + bandRows - 1) / bandRows;

		std::vector<std::vector<std::uint32_t>> bins(
			static_cast<std::size_t>(bandCount));

		for (std::size_t i = 0; i < footprints.size(); ++i)
		{
			const auto& f = footprints[i];

			if (f.x1 < f.x0 || f.y1 < f.y0)
				continue;

			const int b0 = f.y0 / bandRows;
			const int b1 = f.y1 / bandRows;

			for (int b = b0; b <= b1; ++b)
			{
				bins[static_cast<std::size_t>(b)].push_back(
					static_cast<std::uint32_t>(i));
			}
		}

		throwIfCancelled(execution);

		std::size_t binEntries = 0;

		for (const auto& bin : bins)
			binEntries += bin.size();

		progress.stage(collectShare, 1.0, binEntries);

		parallelFor(
			static_cast<std::size_t>(bandCount),
			threads,
			[&](std::size_t band)
			{
				const int rowBegin =
					static_cast<int>(band) * bandRows;

				const int rowEnd =
					std::min(s.height, rowBegin + bandRows);

				std::array<double, MaxTaps + 1> xEdge{};
				std::array<double, MaxTaps> xWeight{};

				std::size_t processed = 0;

				for (const std::uint32_t index : bins[band])
				{
					if ((++processed & 1023u) == 0)
					{
						throwIfCancelled(execution);
						progress.advance(1024);
					}

					const auto& sample = samples[index];
					const auto& f = footprints[index];

					if (!gaussian)
					{
						const double w = sample.weight * 1.0;

						acc.accum[
							static_cast<std::size_t>(f.y0) * s.width + f.x0
						] += glm::dvec4(sample.color * w, w);

						continue;
					}

					// Integrate a normalized Gaussian over each pixel.
					// Adjacent pixels share an edge, so evaluate erf once
					// per edge instead of twice per pixel.
					const int nx = f.x1 - f.x0 + 1;

					for (int i = 0; i <= nx; ++i)
					{
						xEdge[static_cast<std::size_t>(i)] =
							std::erf(
								(static_cast<double>(f.x0 + i) - f.px) /
								divisor);
					}

					for (int i = 0; i < nx; ++i)
					{
						xWeight[static_cast<std::size_t>(i)] =
							0.5 * (
								xEdge[static_cast<std::size_t>(i) + 1] -
								xEdge[static_cast<std::size_t>(i)]);
					}

					const int y0 = std::max(f.y0, rowBegin);
					const int y1 = std::min(f.y1, rowEnd - 1);

					double yLow =
						std::erf(
							(static_cast<double>(y0) - f.py) /
							divisor);

					for (int y = y0; y <= y1; ++y)
					{
						const double yHigh =
							std::erf(
								(static_cast<double>(y) + 1.0 - f.py) /
								divisor);

						const double fy = 0.5 * (yHigh - yLow);
						yLow = yHigh;

						glm::dvec4* row =
							acc.accum.data() +
							static_cast<std::size_t>(y) * s.width +
							f.x0;

						for (int i = 0; i < nx; ++i)
						{
							const double w =
								sample.weight *
								(xWeight[static_cast<std::size_t>(i)] * fy);

							row[i] += glm::dvec4(sample.color * w, w);
						}
					}
				}

				progress.advance(processed & 1023u);
			});

		double peak = 0.0;

		for (const auto& a : acc.accum)
			peak = std::max(peak, a.w);

		acc.peak = peak;

		progress.finish();

		return acc;
	}

	PsfImage tonemapPsf(
		const PsfAccumulation& acc,
		const PsfRenderSettings& s,
		const PsfExecution& execution)
	{
		validate(s);

		PsfImage image;
		image.width = acc.width;
		image.height = acc.height;
		image.center = acc.center;
		image.fieldSize = acc.fieldSize;
		image.observationHits = acc.observationHits;

		const std::size_t count =
			static_cast<std::size_t>(acc.width) * acc.height;

		image.rgba.resize(count * 4);

		const bool white =
			s.background == PsfBackground::White;

		const glm::dvec3 background(
			white ? 1.0 : 0.0);

		const double peak = acc.peak;

		constexpr int RowsPerChunk = 16;

		const std::size_t chunks =
			static_cast<std::size_t>(
				(acc.height + RowsPerChunk - 1) / RowsPerChunk);

		Progress progress(execution);
		progress.stage(0.0, 1.0, chunks);

		parallelFor(
			chunks,
			resolveThreads(execution, chunks),
			[&](std::size_t chunk)
			{
				throwIfCancelled(execution);

				const std::size_t begin =
					chunk * RowsPerChunk * static_cast<std::size_t>(acc.width);

				const std::size_t end =
					std::min(
						count,
						begin + RowsPerChunk * static_cast<std::size_t>(acc.width));

				for (std::size_t i = begin; i < end; ++i)
				{
					const auto& a = acc.accum[i];

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

						if (acc.customColor)
						{
							// Preserve the legacy contract for explicitly
							// supplied display-RGB callbacks.
							rgb =
								background * (1.0 - coverage) +
								glm::clamp(
									glm::dvec3(a) / a.w,
									0.0,
									1.0) *
								coverage;
						}
						else
						{
							// Average XYZ describes the spectrum/chromaticity
							// at this pixel; intensity separately determines
							// spot brightness.
							const glm::dvec3 meanXyz =
								glm::dvec3(a) /
								a.w;

							glm::dvec3 linearColor =
								optics::xyzToLinearSrgb(
									meanXyz);

							linearColor =
								glm::max(
									linearColor,
									glm::dvec3(0.0));

							const double colorPeak =
								std::max(
									linearColor.x,
									std::max(
										linearColor.y,
										linearColor.z));

							if (
								std::isfinite(colorPeak) &&
								colorPeak > 0.0)
							{
								linearColor /=
									colorPeak;
							}
							else
							{
								linearColor =
									glm::dvec3(1.0);
							}

							const glm::dvec3 linearOutput =
								background * (1.0 - coverage) +
								linearColor * coverage;

							rgb =
								optics::linearToSrgb(
									glm::clamp(
										linearOutput,
										0.0,
										1.0));
						}
					}

					for (int channel = 0; channel < 3; ++channel)
					{
						image.rgba[i * 4 + channel] =
							static_cast<std::uint8_t>(
								std::lround(
									std::clamp(
										rgb[channel],
										0.0,
										1.0) *
									255.0));
					}

					image.rgba[i * 4 + 3] = 255;
				}

				progress.advance(1);
			});

		progress.finish();

		return image;
	}

	PsfImage rasterizePsf(
		const raytracer::TraceResult& result,
		const telescope::ObservationPlane& plane,
		const PsfRenderSettings& s,
		const PsfColorFunction& color,
		const PsfExecution& execution)
	{
		return tonemapPsf(
			accumulatePsf(result, plane, s, color, execution),
			s,
			execution);
	}
}
