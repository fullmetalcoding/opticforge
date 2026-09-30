// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <cstddef>
#include <vector>

namespace opticforge::raytracer
{
    enum class SpectrumMode
    {
        Monochromatic,
        Reference
    };

    enum class ReferenceSpectrum
    {
        D65,
        O5,
        B0,
        A0,
        F0,
        G0,
        K0,
        M0
    };

    struct SpectralSample
    {
        double wavelengthNm = 550.0;
        double weight = 1.0;
    };

    const char* referenceSpectrumName(
        ReferenceSpectrum spectrum) noexcept;

    // Representative effective temperature used for the stellar continuum
    // spectra. Returns 0 for non-stellar spectra.
    double referenceSpectrumTemperatureK(
        ReferenceSpectrum spectrum) noexcept;

    // Supported discrete spectral resolutions for broadband tracing.
    // Keeping this set small makes the cost multiplier explicit in the UI.
    bool isSupportedSpectralSampleCount(
        std::size_t sampleCount) noexcept;

    // Build evenly spaced visible-wavelength samples spanning 380-780 nm.
    //
    // Weights are source spectral power integrated with trapezoidal endpoint
    // factors and normalized so their sum is 1.0. A ray bundle should reuse
    // each pupil launch point for every returned spectral sample and put the
    // sample weight into OpticalRay::intensity.
    std::vector<SpectralSample> buildReferenceSpectrum(
        ReferenceSpectrum spectrum,
        std::size_t sampleCount);

} // namespace opticforge::raytracer
