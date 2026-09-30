// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace opticforge::optics
{
    struct WavelengthRangeNm
    {
        double min = 400.0;
        double max = 700.0;
    };

    struct TemperatureRangeC
    {
        double min = 0.0;
        double max = 40.0;
    };

    struct Provenance
    {
        std::string source;
        std::string sourceRevision;
        std::string sourceUrl;
        std::string citation;
        std::string license;
        std::string retrievedDate;
        std::string notes;
    };

    struct CatalogIdentity
    {
        enum class Status
        {
            Current,
            Obsolete,
            Deprecated,
            Experimental
        };

        std::string manufacturer;
        std::string catalog;
        std::string catalogCode;
        std::optional<Status> status;
    };

    enum class MaterialCategory
    {
        OpticalGlass,
        FusedSilica,
        Crystal,
        Ceramic,
        Polymer,
        Liquid,
        Gas,
        Other
    };

    struct ConstantDispersion
    {
        double refractiveIndex = 1.5;
        std::optional<WavelengthRangeNm> validWavelengthNm;
    };

    struct SellmeierDispersion
    {
        std::vector<double> B{ 1.0, 0.0, 0.0 };
        std::vector<double> C{ 0.01, 0.02, 100.0 };
        std::optional<WavelengthRangeNm> validWavelengthNm;
    };

    struct CauchyDispersion
    {
        // A0, A2, A4, ... for n(lambda) = A0 + A2/lambda^2 + ...
        std::vector<double> coefficients{ 1.5, 0.0, 0.0 };
        std::optional<WavelengthRangeNm> validWavelengthNm;
    };

    struct SchottDispersion
    {
        std::array<double, 6> coefficients{
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0
        };

        std::optional<WavelengthRangeNm> validWavelengthNm;
    };

    enum class InterpolationMode
    {
        Linear,
        Cubic
    };

    enum class ExtrapolationMode
    {
        Error,
        Clamp
    };

    struct TabulatedRefractiveIndexSample
    {
        double wavelengthNm = 550.0;
        double n = 1.5;
    };

    struct TabulatedDispersion
    {
        InterpolationMode interpolation =
            InterpolationMode::Linear;

        ExtrapolationMode extrapolation =
            ExtrapolationMode::Error;

        std::vector<TabulatedRefractiveIndexSample> samples{
            { 486.13, 1.5 },
            { 656.27, 1.5 }
        };
    };

    enum class AirFormula
    {
        Ciddor1996,
        Edlen1966
    };

    struct AirConditions
    {
        double temperatureC = 15.0;
        double pressurePa = 101325.0;
        double relativeHumidity = 0.0;
        double co2Ppm = 450.0;
    };

    struct AirDispersion
    {
        AirFormula formula =
            AirFormula::Ciddor1996;

        AirConditions conditions;
        std::optional<WavelengthRangeNm> validWavelengthNm;
    };

    using RefractiveIndexModel =
        std::variant<
            ConstantDispersion,
            SellmeierDispersion,
            CauchyDispersion,
            SchottDispersion,
            TabulatedDispersion,
            AirDispersion>;

    struct ConstantExtinction
    {
        double k = 0.0;
    };

    struct TabulatedExtinctionSample
    {
        double wavelengthNm = 550.0;
        double k = 0.0;
    };

    struct TabulatedExtinction
    {
        InterpolationMode interpolation =
            InterpolationMode::Linear;

        ExtrapolationMode extrapolation =
            ExtrapolationMode::Error;

        std::vector<TabulatedExtinctionSample> samples{
            { 486.13, 0.0 },
            { 656.27, 0.0 }
        };
    };

    using ExtinctionModel =
        std::variant<
            ConstantExtinction,
            TabulatedExtinction>;

    struct IsotropicOptics
    {
        RefractiveIndexModel refractiveIndex =
            ConstantDispersion{};

        std::optional<ExtinctionModel>
            extinctionCoefficient;
    };

    struct UniaxialOptics
    {
        RefractiveIndexModel ordinaryIndex =
            ConstantDispersion{};

        RefractiveIndexModel extraordinaryIndex =
            ConstantDispersion{};

        std::optional<ExtinctionModel>
            ordinaryExtinctionCoefficient;

        std::optional<ExtinctionModel>
            extraordinaryExtinctionCoefficient;

        std::optional<std::array<double, 3>>
            opticAxis =
                std::array<double, 3>{ 0.0, 0.0, 1.0 };
    };

    struct BiaxialOptics
    {
        RefractiveIndexModel xIndex =
            ConstantDispersion{};

        RefractiveIndexModel yIndex =
            ConstantDispersion{};

        RefractiveIndexModel zIndex =
            ConstantDispersion{};

        std::optional<ExtinctionModel>
            xExtinctionCoefficient;

        std::optional<ExtinctionModel>
            yExtinctionCoefficient;

        std::optional<ExtinctionModel>
            zExtinctionCoefficient;
    };

    using OpticalProperties =
        std::variant<
            IsotropicOptics,
            UniaxialOptics,
            BiaxialOptics>;

    struct ConstantDnDt
    {
        double valuePerK = 0.0;
    };

    struct TabulatedDnDtSample
    {
        double wavelengthNm = 550.0;
        double valuePerK = 0.0;
    };

    struct TabulatedDnDt
    {
        InterpolationMode interpolation =
            InterpolationMode::Linear;

        std::vector<TabulatedDnDtSample> samples{
            { 486.13, 0.0 },
            { 656.27, 0.0 }
        };
    };

    using DnDtModel =
        std::variant<
            ConstantDnDt,
            TabulatedDnDt>;

    struct ThermalProperties
    {
        std::optional<double>
            referenceTemperatureC;

        std::optional<TemperatureRangeC>
            validTemperatureC;

        std::optional<DnDtModel>
            dnDt;

        std::optional<double>
            linearExpansionPerK;
    };

    struct PhysicalProperties
    {
        std::optional<double> densityKgM3;
        std::optional<double> youngsModulusPa;
        std::optional<double> poissonRatio;
        std::optional<double> thermalConductivityWmK;
        std::optional<double> specificHeatJKgK;
    };

    struct Material
    {
        std::string key = "local:new-material";
        std::string name = "New Material";

        std::vector<std::string> aliases;
        std::string description;

        std::optional<MaterialCategory> category;
        std::optional<CatalogIdentity> catalog;

        OpticalProperties optics =
            IsotropicOptics{};

        std::optional<ThermalProperties> thermal;
        std::optional<PhysicalProperties> physical;
        std::optional<Provenance> provenance;

        std::vector<std::string> tags;
    };

    struct MaterialLibraryMetadata
    {
        std::string id = "local";
        std::string name = "Local Materials";
        std::string revision = "1";

        std::string description;
        std::string manufacturer;
        std::string generatedBy;
        std::string createdUtc;

        std::optional<Provenance> provenance;
    };

    class MaterialLibrary
    {
    public:
        MaterialLibraryMetadata& metadata() noexcept
        {
            return m_metadata;
        }

        const MaterialLibraryMetadata& metadata() const noexcept
        {
            return m_metadata;
        }

        std::vector<Material>& materials() noexcept
        {
            return m_materials;
        }

        const std::vector<Material>& materials() const noexcept
        {
            return m_materials;
        }

        Material* find(
            std::string_view key) noexcept;

        const Material* find(
            std::string_view key) const noexcept;

        bool contains(
            std::string_view key) const noexcept;

        bool add(
            Material material);

        bool update(
            std::string_view originalKey,
            Material material);

        bool remove(
            std::string_view key);

        void clear();

    private:
        MaterialLibraryMetadata m_metadata;
        std::vector<Material> m_materials;
    };

} // namespace opticforge::optics
