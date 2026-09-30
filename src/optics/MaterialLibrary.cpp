// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "MaterialLibrary.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace opticforge::optics
{
    Material* MaterialLibrary::find(
        std::string_view key) noexcept
    {
        const auto it =
            std::find_if(
                m_materials.begin(),
                m_materials.end(),
                [key](const Material& material)
                {
                    return material.key == key;
                });

        return
            it == m_materials.end()
            ? nullptr
            : &*it;
    }

    const Material* MaterialLibrary::find(
        std::string_view key) const noexcept
    {
        const auto it =
            std::find_if(
                m_materials.begin(),
                m_materials.end(),
                [key](const Material& material)
                {
                    return material.key == key;
                });

        return
            it == m_materials.end()
            ? nullptr
            : &*it;
    }

    bool MaterialLibrary::contains(
        std::string_view key) const noexcept
    {
        return find(key) != nullptr;
    }

    bool MaterialLibrary::add(
        Material material)
    {
        if (
            material.key.empty() ||
            contains(material.key))
        {
            return false;
        }

        m_materials.push_back(
            std::move(material));

        return true;
    }

    bool MaterialLibrary::update(
        std::string_view originalKey,
        Material material)
    {
        Material* existing =
            find(originalKey);

        if (!existing || material.key.empty())
            return false;

        if (
            material.key != originalKey &&
            contains(material.key))
        {
            return false;
        }

        *existing =
            std::move(material);

        return true;
    }

    bool MaterialLibrary::remove(
        std::string_view key)
    {
        const auto it =
            std::remove_if(
                m_materials.begin(),
                m_materials.end(),
                [key](const Material& material)
                {
                    return material.key == key;
                });

        if (it == m_materials.end())
            return false;

        m_materials.erase(
            it,
            m_materials.end());

        return true;
    }

    double MaterialLibrary::refractiveIndex(
        std::string_view key,
        double wavelengthNm) const
    {
        if (
            !std::isfinite(wavelengthNm) ||
            wavelengthNm <= 0.0)
        {
            throw std::invalid_argument(
                "Wavelength must be finite and positive.");
        }

        const Material* material =
            find(key);

        if (!material)
        {
            throw std::runtime_error(
                "Unknown optical material '" +
                std::string(key) +
                "'.");
        }

        const auto* isotropic =
            std::get_if<IsotropicOptics>(
                &material->optics);

        if (!isotropic)
        {
            throw std::runtime_error(
                "Material '" +
                material->key +
                "' is anisotropic; anisotropic refraction is not yet supported.");
        }

        const double wavelengthUm =
            wavelengthNm * 1.0e-3;

        const auto checkRange =
            [wavelengthNm, &material](
                const std::optional<WavelengthRangeNm>& range)
            {
                if (
                    range &&
                    (wavelengthNm < range->min ||
                     wavelengthNm > range->max))
                {
                    throw std::runtime_error(
                        "Wavelength " +
                        std::to_string(wavelengthNm) +
                        " nm is outside the valid range for material '" +
                        material->key +
                        "'.");
                }
            };

        const double n =
            std::visit(
                [&](const auto& model) -> double
                {
                    using T =
                        std::decay_t<decltype(model)>;

                    if constexpr (
                        std::is_same_v<T, ConstantDispersion>)
                    {
                        checkRange(
                            model.validWavelengthNm);

                        return
                            model.refractiveIndex;
                    }
                    else if constexpr (
                        std::is_same_v<T, SellmeierDispersion>)
                    {
                        checkRange(
                            model.validWavelengthNm);

                        if (
                            model.B.empty() ||
                            model.B.size() != model.C.size())
                        {
                            throw std::runtime_error(
                                "Invalid Sellmeier coefficients for material '" +
                                material->key +
                                "'.");
                        }

                        const double lambda2 =
                            wavelengthUm * wavelengthUm;

                        double n2 = 1.0;

                        for (
                            std::size_t i = 0;
                            i < model.B.size();
                            ++i)
                        {
                            const double denominator =
                                lambda2 - model.C[i];

                            if (
                                !std::isfinite(denominator) ||
                                std::abs(denominator) < 1.0e-15)
                            {
                                throw std::runtime_error(
                                    "Sellmeier pole encountered for material '" +
                                    material->key +
                                    "'.");
                            }

                            n2 +=
                                model.B[i] *
                                lambda2 /
                                denominator;
                        }

                        if (
                            !std::isfinite(n2) ||
                            n2 <= 0.0)
                        {
                            throw std::runtime_error(
                                "Invalid Sellmeier result for material '" +
                                material->key +
                                "'.");
                        }

                        return std::sqrt(n2);
                    }
                    else if constexpr (
                        std::is_same_v<T, CauchyDispersion>)
                    {
                        checkRange(
                            model.validWavelengthNm);

                        if (model.coefficients.empty())
                        {
                            throw std::runtime_error(
                                "Invalid Cauchy coefficients for material '" +
                                material->key +
                                "'.");
                        }

                        double result =
                            model.coefficients[0];

                        const double inverseLambda2 =
                            1.0 /
                            (wavelengthUm * wavelengthUm);

                        double power =
                            inverseLambda2;

                        for (
                            std::size_t i = 1;
                            i < model.coefficients.size();
                            ++i)
                        {
                            result +=
                                model.coefficients[i] *
                                power;

                            power *=
                                inverseLambda2;
                        }

                        return result;
                    }
                    else if constexpr (
                        std::is_same_v<T, SchottDispersion>)
                    {
                        checkRange(
                            model.validWavelengthNm);

                        const double lambda2 =
                            wavelengthUm * wavelengthUm;

                        const double inverseLambda2 =
                            1.0 / lambda2;

                        double n2 =
                            model.coefficients[0] +
                            model.coefficients[1] *
                                lambda2;

                        double power =
                            inverseLambda2;

                        for (std::size_t i = 2; i < 6; ++i)
                        {
                            n2 +=
                                model.coefficients[i] *
                                power;

                            power *=
                                inverseLambda2;
                        }

                        if (
                            !std::isfinite(n2) ||
                            n2 <= 0.0)
                        {
                            throw std::runtime_error(
                                "Invalid Schott dispersion result for material '" +
                                material->key +
                                "'.");
                        }

                        return std::sqrt(n2);
                    }
                    else if constexpr (
                        std::is_same_v<T, TabulatedDispersion>)
                    {
                        if (model.samples.size() < 2)
                        {
                            throw std::runtime_error(
                                "Tabulated dispersion for material '" +
                                material->key +
                                "' requires at least two samples.");
                        }

                        const auto& samples =
                            model.samples;

                        if (wavelengthNm <= samples.front().wavelengthNm)
                        {
                            if (
                                wavelengthNm < samples.front().wavelengthNm &&
                                model.extrapolation == ExtrapolationMode::Error)
                            {
                                throw std::runtime_error(
                                    "Wavelength is below tabulated range for material '" +
                                    material->key +
                                    "'.");
                            }

                            return
                                samples.front().n;
                        }

                        if (wavelengthNm >= samples.back().wavelengthNm)
                        {
                            if (
                                wavelengthNm > samples.back().wavelengthNm &&
                                model.extrapolation == ExtrapolationMode::Error)
                            {
                                throw std::runtime_error(
                                    "Wavelength is above tabulated range for material '" +
                                    material->key +
                                    "'.");
                            }

                            return
                                samples.back().n;
                        }

                        const auto upper =
                            std::upper_bound(
                                samples.begin(),
                                samples.end(),
                                wavelengthNm,
                                [](double wavelength, const auto& sample)
                                {
                                    return
                                        wavelength <
                                        sample.wavelengthNm;
                                });

                        const auto lower =
                            upper - 1;

                        const double span =
                            upper->wavelengthNm -
                            lower->wavelengthNm;

                        if (span <= 0.0)
                        {
                            throw std::runtime_error(
                                "Tabulated wavelengths must be strictly increasing for material '" +
                                material->key +
                                "'.");
                        }

                        const double t =
                            (wavelengthNm -
                             lower->wavelengthNm) /
                            span;

                        // Cubic interpolation is intentionally reduced to
                        // linear at the ends and when fewer than four samples
                        // are available. Linear is also the exact requested
                        // mode.
                        if (
                            model.interpolation == InterpolationMode::Linear ||
                            samples.size() < 4)
                        {
                            return
                                lower->n +
                                (upper->n - lower->n) *
                                t;
                        }

                        const std::size_t upperIndex =
                            static_cast<std::size_t>(
                                upper - samples.begin());

                        const std::size_t i1 =
                            upperIndex - 1;

                        const std::size_t i0 =
                            i1 > 0 ? i1 - 1 : i1;

                        const std::size_t i2 =
                            upperIndex;

                        const std::size_t i3 =
                            std::min(
                                i2 + 1,
                                samples.size() - 1);

                        const double p0 = samples[i0].n;
                        const double p1 = samples[i1].n;
                        const double p2 = samples[i2].n;
                        const double p3 = samples[i3].n;

                        const double t2 = t * t;
                        const double t3 = t2 * t;

                        return
                            0.5 *
                            (
                                2.0 * p1 +
                                (-p0 + p2) * t +
                                (2.0 * p0 - 5.0 * p1 +
                                 4.0 * p2 - p3) * t2 +
                                (-p0 + 3.0 * p1 -
                                 3.0 * p2 + p3) * t3
                            );
                    }
                    else
                    {
                        throw std::runtime_error(
                            "Air dispersion models are not yet supported by the raytracer.");
                    }
                },
                isotropic->refractiveIndex);

        if (
            !std::isfinite(n) ||
            n <= 0.0)
        {
            throw std::runtime_error(
                "Material '" +
                material->key +
                "' produced an invalid refractive index.");
        }

        return n;
    }

    void MaterialLibrary::clear()
    {
        m_metadata =
            MaterialLibraryMetadata{};

        m_materials.clear();
    }

} // namespace opticforge::optics
