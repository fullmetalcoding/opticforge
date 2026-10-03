// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <cstddef>
#include <vector>

#include <glm/glm.hpp>

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
        // Representative wavelength used for geometric/material tracing.
        double wavelengthNm = 550.0;

        // Fraction of the source's integrated power represented by this band.
        // Reference-spectrum sample weights sum to 1.
        double weight = 1.0;

        // CIE XYZ contribution per unit radiant power for the whole band.
        // Multiplying by weight reconstructs the band's integrated
        // tristimulus contribution.
        glm::dvec3 cieXyzPerUnitPower{ 0.0 };
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

    // Partition 380-780 nm into equal-width spectral bands.
    //
    // Each returned sample stores:
    //   - the source-power-weighted wavelength used for ray tracing,
    //   - the band's integrated source power normalized across all bands,
    //   - the band's integrated CIE XYZ response per unit radiant power.
    //
    // This keeps coarse 3/7-sample traces colorimetrically stable while still
    // making spectral sample count control geometric/chromatic ray accuracy.
    std::vector<SpectralSample> buildReferenceSpectrum(
        ReferenceSpectrum spectrum,
        std::size_t sampleCount);

} // namespace opticforge::raytracer
