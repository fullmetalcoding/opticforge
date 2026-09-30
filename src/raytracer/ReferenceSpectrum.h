// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <string_view>

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

    const char* referenceSpectrumName(
        ReferenceSpectrum spectrum) noexcept;

    // Draw one wavelength from the selected reference spectrum.
    //
    // u must be in [0, 1). The return value is in nanometres.
    double sampleReferenceSpectrum(
        ReferenceSpectrum spectrum,
        double u);

    // Representative effective temperature used for the stellar continuum
    // spectra. Returns 0 for non-stellar spectra.
    double referenceSpectrumTemperatureK(
        ReferenceSpectrum spectrum) noexcept;

} // namespace opticforge::raytracer
