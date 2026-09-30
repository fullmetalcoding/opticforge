// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "MaterialLibrarySerializer.h"

#include <stdexcept>
#include <string>
#include <type_traits>

#include <nlohmann/json.hpp>

namespace opticforge::project
{
    namespace
    {
        using nlohmann::json;

        [[noreturn]] void fail(const std::string& message)
        {
            throw std::runtime_error(
                "OpticForge material library deserialization failed: " +
                message);
        }

        json serializeRange(
            const optics::WavelengthRangeNm& range)
        {
            return {
                { "min", range.min },
                { "max", range.max }
            };
        }

        optics::WavelengthRangeNm deserializeWavelengthRange(
            const json& value)
        {
            return {
                value.at("min").get<double>(),
                value.at("max").get<double>()
            };
        }

        json serializeTemperatureRange(
            const optics::TemperatureRangeC& range)
        {
            return {
                { "min", range.min },
                { "max", range.max }
            };
        }

        optics::TemperatureRangeC deserializeTemperatureRange(
            const json& value)
        {
            return {
                value.at("min").get<double>(),
                value.at("max").get<double>()
            };
        }

        json serializeProvenance(
            const optics::Provenance& value)
        {
            json result = json::object();

            if (!value.source.empty()) result["source"] = value.source;
            if (!value.sourceRevision.empty()) result["sourceRevision"] = value.sourceRevision;
            if (!value.sourceUrl.empty()) result["sourceUrl"] = value.sourceUrl;
            if (!value.citation.empty()) result["citation"] = value.citation;
            if (!value.license.empty()) result["license"] = value.license;
            if (!value.retrievedDate.empty()) result["retrievedDate"] = value.retrievedDate;
            if (!value.notes.empty()) result["notes"] = value.notes;

            return result;
        }

        optics::Provenance deserializeProvenance(
            const json& value)
        {
            optics::Provenance result;

            result.source = value.value("source", std::string{});
            result.sourceRevision = value.value("sourceRevision", std::string{});
            result.sourceUrl = value.value("sourceUrl", std::string{});
            result.citation = value.value("citation", std::string{});
            result.license = value.value("license", std::string{});
            result.retrievedDate = value.value("retrievedDate", std::string{});
            result.notes = value.value("notes", std::string{});

            return result;
        }

        const char* categoryName(
            optics::MaterialCategory value)
        {
            switch (value)
            {
            case optics::MaterialCategory::OpticalGlass: return "optical-glass";
            case optics::MaterialCategory::FusedSilica: return "fused-silica";
            case optics::MaterialCategory::Crystal: return "crystal";
            case optics::MaterialCategory::Ceramic: return "ceramic";
            case optics::MaterialCategory::Polymer: return "polymer";
            case optics::MaterialCategory::Liquid: return "liquid";
            case optics::MaterialCategory::Gas: return "gas";
            case optics::MaterialCategory::Other: return "other";
            }

            return "other";
        }

        optics::MaterialCategory deserializeCategory(
            const std::string& value)
        {
            if (value == "optical-glass") return optics::MaterialCategory::OpticalGlass;
            if (value == "fused-silica") return optics::MaterialCategory::FusedSilica;
            if (value == "crystal") return optics::MaterialCategory::Crystal;
            if (value == "ceramic") return optics::MaterialCategory::Ceramic;
            if (value == "polymer") return optics::MaterialCategory::Polymer;
            if (value == "liquid") return optics::MaterialCategory::Liquid;
            if (value == "gas") return optics::MaterialCategory::Gas;
            if (value == "other") return optics::MaterialCategory::Other;

            fail("unknown material category '" + value + "'.");
        }

        const char* statusName(
            optics::CatalogIdentity::Status value)
        {
            switch (value)
            {
            case optics::CatalogIdentity::Status::Current: return "current";
            case optics::CatalogIdentity::Status::Obsolete: return "obsolete";
            case optics::CatalogIdentity::Status::Deprecated: return "deprecated";
            case optics::CatalogIdentity::Status::Experimental: return "experimental";
            }

            return "current";
        }

        optics::CatalogIdentity::Status deserializeStatus(
            const std::string& value)
        {
            if (value == "current") return optics::CatalogIdentity::Status::Current;
            if (value == "obsolete") return optics::CatalogIdentity::Status::Obsolete;
            if (value == "deprecated") return optics::CatalogIdentity::Status::Deprecated;
            if (value == "experimental") return optics::CatalogIdentity::Status::Experimental;

            fail("unknown catalog status '" + value + "'.");
        }

        json serializeCatalog(
            const optics::CatalogIdentity& value)
        {
            json result = json::object();

            if (!value.manufacturer.empty()) result["manufacturer"] = value.manufacturer;
            if (!value.catalog.empty()) result["catalog"] = value.catalog;
            if (!value.catalogCode.empty()) result["catalogCode"] = value.catalogCode;
            if (value.status) result["status"] = statusName(*value.status);

            return result;
        }

        optics::CatalogIdentity deserializeCatalog(
            const json& value)
        {
            optics::CatalogIdentity result;

            result.manufacturer = value.value("manufacturer", std::string{});
            result.catalog = value.value("catalog", std::string{});
            result.catalogCode = value.value("catalogCode", std::string{});

            if (value.contains("status"))
            {
                result.status =
                    deserializeStatus(
                        value.at("status").get<std::string>());
            }

            return result;
        }

        const char* interpolationName(
            optics::InterpolationMode value)
        {
            return
                value == optics::InterpolationMode::Cubic
                ? "cubic"
                : "linear";
        }

        optics::InterpolationMode deserializeInterpolation(
            const std::string& value)
        {
            if (value == "linear") return optics::InterpolationMode::Linear;
            if (value == "cubic") return optics::InterpolationMode::Cubic;

            fail("unknown interpolation mode '" + value + "'.");
        }

        const char* extrapolationName(
            optics::ExtrapolationMode value)
        {
            return
                value == optics::ExtrapolationMode::Clamp
                ? "clamp"
                : "error";
        }

        optics::ExtrapolationMode deserializeExtrapolation(
            const std::string& value)
        {
            if (value == "error") return optics::ExtrapolationMode::Error;
            if (value == "clamp") return optics::ExtrapolationMode::Clamp;

            fail("unknown extrapolation mode '" + value + "'.");
        }

        void addWavelengthRange(
            json& result,
            const std::optional<optics::WavelengthRangeNm>& range)
        {
            if (range)
                result["validWavelengthNm"] = serializeRange(*range);
        }

        std::optional<optics::WavelengthRangeNm> readWavelengthRange(
            const json& value)
        {
            if (!value.contains("validWavelengthNm"))
                return std::nullopt;

            return
                deserializeWavelengthRange(
                    value.at("validWavelengthNm"));
        }

        json serializeRefractiveIndexModel(
            const optics::RefractiveIndexModel& model)
        {
            return std::visit(
                [](const auto& value) -> json
                {
                    using T = std::decay_t<decltype(value)>;
                    json result = json::object();

                    if constexpr (std::is_same_v<T, optics::ConstantDispersion>)
                    {
                        result["type"] = "constant";
                        result["refractiveIndex"] = value.refractiveIndex;
                        addWavelengthRange(result, value.validWavelengthNm);
                    }
                    else if constexpr (std::is_same_v<T, optics::SellmeierDispersion>)
                    {
                        result["type"] = "sellmeier";
                        result["B"] = value.B;
                        result["C"] = value.C;
                        result["coefficientWavelengthUnit"] = "um";
                        addWavelengthRange(result, value.validWavelengthNm);
                    }
                    else if constexpr (std::is_same_v<T, optics::CauchyDispersion>)
                    {
                        result["type"] = "cauchy";
                        result["coefficients"] = value.coefficients;
                        result["coefficientWavelengthUnit"] = "um";
                        addWavelengthRange(result, value.validWavelengthNm);
                    }
                    else if constexpr (std::is_same_v<T, optics::SchottDispersion>)
                    {
                        result["type"] = "schott";
                        result["coefficients"] = value.coefficients;
                        result["coefficientWavelengthUnit"] = "um";
                        addWavelengthRange(result, value.validWavelengthNm);
                    }
                    else if constexpr (std::is_same_v<T, optics::TabulatedDispersion>)
                    {
                        result["type"] = "tabulated";
                        result["interpolation"] = interpolationName(value.interpolation);
                        result["extrapolation"] = extrapolationName(value.extrapolation);
                        result["samples"] = json::array();

                        for (const auto& sample : value.samples)
                        {
                            result["samples"].push_back({
                                { "wavelengthNm", sample.wavelengthNm },
                                { "n", sample.n }
                            });
                        }
                    }
                    else
                    {
                        static_assert(std::is_same_v<T, optics::AirDispersion>);

                        result["type"] = "air";
                        result["formula"] =
                            value.formula == optics::AirFormula::Edlen1966
                            ? "edlen1966"
                            : "ciddor1996";

                        result["conditions"] = {
                            { "temperatureC", value.conditions.temperatureC },
                            { "pressurePa", value.conditions.pressurePa },
                            { "relativeHumidity", value.conditions.relativeHumidity },
                            { "co2Ppm", value.conditions.co2Ppm }
                        };

                        addWavelengthRange(result, value.validWavelengthNm);
                    }

                    return result;
                },
                model);
        }

        optics::RefractiveIndexModel deserializeRefractiveIndexModel(
            const json& value)
        {
            const std::string type =
                value.at("type").get<std::string>();

            if (type == "constant")
            {
                optics::ConstantDispersion result;
                result.refractiveIndex = value.at("refractiveIndex").get<double>();
                result.validWavelengthNm = readWavelengthRange(value);
                return result;
            }

            if (type == "sellmeier")
            {
                optics::SellmeierDispersion result;
                result.B = value.at("B").get<std::vector<double>>();
                result.C = value.at("C").get<std::vector<double>>();
                result.validWavelengthNm = readWavelengthRange(value);
                return result;
            }

            if (type == "cauchy")
            {
                optics::CauchyDispersion result;
                result.coefficients = value.at("coefficients").get<std::vector<double>>();
                result.validWavelengthNm = readWavelengthRange(value);
                return result;
            }

            if (type == "schott")
            {
                const auto coefficients =
                    value.at("coefficients").get<std::vector<double>>();

                if (coefficients.size() != 6)
                    fail("Schott model requires exactly six coefficients.");

                optics::SchottDispersion result;

                for (std::size_t i = 0; i < 6; ++i)
                    result.coefficients[i] = coefficients[i];

                result.validWavelengthNm = readWavelengthRange(value);
                return result;
            }

            if (type == "tabulated")
            {
                optics::TabulatedDispersion result;

                result.interpolation =
                    deserializeInterpolation(
                        value.at("interpolation").get<std::string>());

                result.extrapolation =
                    deserializeExtrapolation(
                        value.value("extrapolation", std::string("error")));

                result.samples.clear();

                for (const auto& sample : value.at("samples"))
                {
                    result.samples.push_back({
                        sample.at("wavelengthNm").get<double>(),
                        sample.at("n").get<double>()
                    });
                }

                return result;
            }

            if (type == "air")
            {
                optics::AirDispersion result;

                const std::string formula =
                    value.at("formula").get<std::string>();

                if (formula == "ciddor1996")
                    result.formula = optics::AirFormula::Ciddor1996;
                else if (formula == "edlen1966")
                    result.formula = optics::AirFormula::Edlen1966;
                else
                    fail("unknown air formula '" + formula + "'.");

                const auto& conditions =
                    value.at("conditions");

                result.conditions.temperatureC =
                    conditions.at("temperatureC").get<double>();

                result.conditions.pressurePa =
                    conditions.at("pressurePa").get<double>();

                result.conditions.relativeHumidity =
                    conditions.at("relativeHumidity").get<double>();

                result.conditions.co2Ppm =
                    conditions.at("co2Ppm").get<double>();

                result.validWavelengthNm =
                    readWavelengthRange(value);

                return result;
            }

            fail("unknown refractive-index model '" + type + "'.");
        }

        json serializeExtinction(
            const optics::ExtinctionModel& model)
        {
            return std::visit(
                [](const auto& value) -> json
                {
                    using T = std::decay_t<decltype(value)>;

                    if constexpr (std::is_same_v<T, optics::ConstantExtinction>)
                    {
                        return {
                            { "type", "constant" },
                            { "k", value.k }
                        };
                    }
                    else
                    {
                        json samples = json::array();

                        for (const auto& sample : value.samples)
                        {
                            samples.push_back({
                                { "wavelengthNm", sample.wavelengthNm },
                                { "k", sample.k }
                            });
                        }

                        return {
                            { "type", "tabulated" },
                            { "interpolation", interpolationName(value.interpolation) },
                            { "extrapolation", extrapolationName(value.extrapolation) },
                            { "samples", std::move(samples) }
                        };
                    }
                },
                model);
        }

        optics::ExtinctionModel deserializeExtinction(
            const json& value)
        {
            const std::string type =
                value.at("type").get<std::string>();

            if (type == "constant")
                return optics::ConstantExtinction{
                    value.at("k").get<double>()
                };

            if (type == "tabulated")
            {
                optics::TabulatedExtinction result;

                result.interpolation =
                    deserializeInterpolation(
                        value.at("interpolation").get<std::string>());

                result.extrapolation =
                    deserializeExtrapolation(
                        value.value("extrapolation", std::string("error")));

                result.samples.clear();

                for (const auto& sample : value.at("samples"))
                {
                    result.samples.push_back({
                        sample.at("wavelengthNm").get<double>(),
                        sample.at("k").get<double>()
                    });
                }

                return result;
            }

            fail("unknown extinction model '" + type + "'.");
        }

        json serializeOpticalProperties(
            const optics::OpticalProperties& optics)
        {
            return std::visit(
                [](const auto& value) -> json
                {
                    using T = std::decay_t<decltype(value)>;
                    json result = json::object();

                    if constexpr (std::is_same_v<T, optics::IsotropicOptics>)
                    {
                        result["type"] = "isotropic";
                        result["refractiveIndex"] =
                            serializeRefractiveIndexModel(value.refractiveIndex);

                        if (value.extinctionCoefficient)
                        {
                            result["extinctionCoefficient"] =
                                serializeExtinction(*value.extinctionCoefficient);
                        }
                    }
                    else if constexpr (std::is_same_v<T, optics::UniaxialOptics>)
                    {
                        result["type"] = "uniaxial";
                        result["ordinaryIndex"] =
                            serializeRefractiveIndexModel(value.ordinaryIndex);
                        result["extraordinaryIndex"] =
                            serializeRefractiveIndexModel(value.extraordinaryIndex);

                        if (value.ordinaryExtinctionCoefficient)
                            result["ordinaryExtinctionCoefficient"] =
                                serializeExtinction(*value.ordinaryExtinctionCoefficient);

                        if (value.extraordinaryExtinctionCoefficient)
                            result["extraordinaryExtinctionCoefficient"] =
                                serializeExtinction(*value.extraordinaryExtinctionCoefficient);

                        if (value.opticAxis)
                            result["opticAxis"] = *value.opticAxis;
                    }
                    else
                    {
                        result["type"] = "biaxial";
                        result["xIndex"] = serializeRefractiveIndexModel(value.xIndex);
                        result["yIndex"] = serializeRefractiveIndexModel(value.yIndex);
                        result["zIndex"] = serializeRefractiveIndexModel(value.zIndex);

                        if (value.xExtinctionCoefficient)
                            result["xExtinctionCoefficient"] =
                                serializeExtinction(*value.xExtinctionCoefficient);

                        if (value.yExtinctionCoefficient)
                            result["yExtinctionCoefficient"] =
                                serializeExtinction(*value.yExtinctionCoefficient);

                        if (value.zExtinctionCoefficient)
                            result["zExtinctionCoefficient"] =
                                serializeExtinction(*value.zExtinctionCoefficient);
                    }

                    return result;
                },
                optics);
        }

        optics::OpticalProperties deserializeOpticalProperties(
            const json& value)
        {
            const std::string type =
                value.at("type").get<std::string>();

            if (type == "isotropic")
            {
                optics::IsotropicOptics result;

                result.refractiveIndex =
                    deserializeRefractiveIndexModel(
                        value.at("refractiveIndex"));

                if (value.contains("extinctionCoefficient"))
                {
                    result.extinctionCoefficient =
                        deserializeExtinction(
                            value.at("extinctionCoefficient"));
                }

                return result;
            }

            if (type == "uniaxial")
            {
                optics::UniaxialOptics result;

                result.ordinaryIndex =
                    deserializeRefractiveIndexModel(
                        value.at("ordinaryIndex"));

                result.extraordinaryIndex =
                    deserializeRefractiveIndexModel(
                        value.at("extraordinaryIndex"));

                if (value.contains("ordinaryExtinctionCoefficient"))
                    result.ordinaryExtinctionCoefficient =
                        deserializeExtinction(value.at("ordinaryExtinctionCoefficient"));

                if (value.contains("extraordinaryExtinctionCoefficient"))
                    result.extraordinaryExtinctionCoefficient =
                        deserializeExtinction(value.at("extraordinaryExtinctionCoefficient"));

                if (value.contains("opticAxis"))
                {
                    const auto axis =
                        value.at("opticAxis").get<std::vector<double>>();

                    if (axis.size() != 3)
                        fail("uniaxial opticAxis requires three elements.");

                    result.opticAxis =
                        std::array<double, 3>{
                            axis[0], axis[1], axis[2]
                        };
                }
                else
                {
                    result.opticAxis.reset();
                }

                return result;
            }

            if (type == "biaxial")
            {
                optics::BiaxialOptics result;

                result.xIndex =
                    deserializeRefractiveIndexModel(value.at("xIndex"));
                result.yIndex =
                    deserializeRefractiveIndexModel(value.at("yIndex"));
                result.zIndex =
                    deserializeRefractiveIndexModel(value.at("zIndex"));

                if (value.contains("xExtinctionCoefficient"))
                    result.xExtinctionCoefficient =
                        deserializeExtinction(value.at("xExtinctionCoefficient"));

                if (value.contains("yExtinctionCoefficient"))
                    result.yExtinctionCoefficient =
                        deserializeExtinction(value.at("yExtinctionCoefficient"));

                if (value.contains("zExtinctionCoefficient"))
                    result.zExtinctionCoefficient =
                        deserializeExtinction(value.at("zExtinctionCoefficient"));

                return result;
            }

            fail("unknown optical properties type '" + type + "'.");
        }

        json serializeDnDt(
            const optics::DnDtModel& model)
        {
            return std::visit(
                [](const auto& value) -> json
                {
                    using T = std::decay_t<decltype(value)>;

                    if constexpr (std::is_same_v<T, optics::ConstantDnDt>)
                    {
                        return {
                            { "type", "constant" },
                            { "valuePerK", value.valuePerK }
                        };
                    }
                    else
                    {
                        json samples = json::array();

                        for (const auto& sample : value.samples)
                        {
                            samples.push_back({
                                { "wavelengthNm", sample.wavelengthNm },
                                { "valuePerK", sample.valuePerK }
                            });
                        }

                        return {
                            { "type", "tabulated" },
                            { "interpolation", interpolationName(value.interpolation) },
                            { "samples", std::move(samples) }
                        };
                    }
                },
                model);
        }

        optics::DnDtModel deserializeDnDt(
            const json& value)
        {
            const std::string type =
                value.at("type").get<std::string>();

            if (type == "constant")
            {
                return optics::ConstantDnDt{
                    value.at("valuePerK").get<double>()
                };
            }

            if (type == "tabulated")
            {
                optics::TabulatedDnDt result;

                result.interpolation =
                    deserializeInterpolation(
                        value.at("interpolation").get<std::string>());

                result.samples.clear();

                for (const auto& sample : value.at("samples"))
                {
                    result.samples.push_back({
                        sample.at("wavelengthNm").get<double>(),
                        sample.at("valuePerK").get<double>()
                    });
                }

                return result;
            }

            fail("unknown dn/dT model '" + type + "'.");
        }

        json serializeThermal(
            const optics::ThermalProperties& value)
        {
            json result = json::object();

            if (value.referenceTemperatureC)
                result["referenceTemperatureC"] = *value.referenceTemperatureC;

            if (value.validTemperatureC)
                result["validTemperatureC"] =
                    serializeTemperatureRange(*value.validTemperatureC);

            if (value.dnDt)
                result["dnDt"] = serializeDnDt(*value.dnDt);

            if (value.linearExpansionPerK)
                result["linearExpansionPerK"] = *value.linearExpansionPerK;

            return result;
        }

        optics::ThermalProperties deserializeThermal(
            const json& value)
        {
            optics::ThermalProperties result;

            if (value.contains("referenceTemperatureC"))
                result.referenceTemperatureC =
                    value.at("referenceTemperatureC").get<double>();

            if (value.contains("validTemperatureC"))
                result.validTemperatureC =
                    deserializeTemperatureRange(value.at("validTemperatureC"));

            if (value.contains("dnDt"))
                result.dnDt =
                    deserializeDnDt(value.at("dnDt"));

            if (value.contains("linearExpansionPerK"))
                result.linearExpansionPerK =
                    value.at("linearExpansionPerK").get<double>();

            return result;
        }

        json serializePhysical(
            const optics::PhysicalProperties& value)
        {
            json result = json::object();

            if (value.densityKgM3) result["densityKgM3"] = *value.densityKgM3;
            if (value.youngsModulusPa) result["youngsModulusPa"] = *value.youngsModulusPa;
            if (value.poissonRatio) result["poissonRatio"] = *value.poissonRatio;
            if (value.thermalConductivityWmK) result["thermalConductivityWmK"] = *value.thermalConductivityWmK;
            if (value.specificHeatJKgK) result["specificHeatJKgK"] = *value.specificHeatJKgK;

            return result;
        }

        optics::PhysicalProperties deserializePhysical(
            const json& value)
        {
            optics::PhysicalProperties result;

            if (value.contains("densityKgM3")) result.densityKgM3 = value.at("densityKgM3").get<double>();
            if (value.contains("youngsModulusPa")) result.youngsModulusPa = value.at("youngsModulusPa").get<double>();
            if (value.contains("poissonRatio")) result.poissonRatio = value.at("poissonRatio").get<double>();
            if (value.contains("thermalConductivityWmK")) result.thermalConductivityWmK = value.at("thermalConductivityWmK").get<double>();
            if (value.contains("specificHeatJKgK")) result.specificHeatJKgK = value.at("specificHeatJKgK").get<double>();

            return result;
        }

        json serializeMaterial(
            const optics::Material& material)
        {
            json result = {
                { "key", material.key },
                { "name", material.name },
                { "optics", serializeOpticalProperties(material.optics) }
            };

            if (!material.aliases.empty()) result["aliases"] = material.aliases;
            if (!material.description.empty()) result["description"] = material.description;
            if (material.category) result["category"] = categoryName(*material.category);
            if (material.catalog) result["catalog"] = serializeCatalog(*material.catalog);
            if (material.thermal) result["thermal"] = serializeThermal(*material.thermal);
            if (material.physical) result["physical"] = serializePhysical(*material.physical);
            if (material.provenance) result["provenance"] = serializeProvenance(*material.provenance);
            if (!material.tags.empty()) result["tags"] = material.tags;

            return result;
        }

        optics::Material deserializeMaterial(
            const json& value)
        {
            optics::Material result;

            result.key = value.at("key").get<std::string>();
            result.name = value.at("name").get<std::string>();
            result.optics =
                deserializeOpticalProperties(
                    value.at("optics"));

            if (value.contains("aliases"))
                result.aliases =
                    value.at("aliases").get<std::vector<std::string>>();

            result.description =
                value.value("description", std::string{});

            if (value.contains("category"))
                result.category =
                    deserializeCategory(
                        value.at("category").get<std::string>());

            if (value.contains("catalog"))
                result.catalog =
                    deserializeCatalog(value.at("catalog"));

            if (value.contains("thermal"))
                result.thermal =
                    deserializeThermal(value.at("thermal"));

            if (value.contains("physical"))
                result.physical =
                    deserializePhysical(value.at("physical"));

            if (value.contains("provenance"))
                result.provenance =
                    deserializeProvenance(value.at("provenance"));

            if (value.contains("tags"))
                result.tags =
                    value.at("tags").get<std::vector<std::string>>();

            return result;
        }
    }

    nlohmann::json MaterialLibrarySerializer::serialize(
        const optics::MaterialLibrary& library)
    {
        const auto& metadata =
            library.metadata();

        json libraryJson = {
            { "id", metadata.id },
            { "name", metadata.name },
            { "revision", metadata.revision }
        };

        if (!metadata.description.empty()) libraryJson["description"] = metadata.description;
        if (!metadata.manufacturer.empty()) libraryJson["manufacturer"] = metadata.manufacturer;
        if (!metadata.generatedBy.empty()) libraryJson["generatedBy"] = metadata.generatedBy;
        if (!metadata.createdUtc.empty()) libraryJson["createdUtc"] = metadata.createdUtc;
        if (metadata.provenance) libraryJson["provenance"] = serializeProvenance(*metadata.provenance);

        json materials = json::array();

        for (const auto& material : library.materials())
            materials.push_back(serializeMaterial(material));

        return {
            { "format", "OpticForgeMaterialLibrary" },
            { "version", FormatVersion },
            { "library", std::move(libraryJson) },
            { "materials", std::move(materials) }
        };
    }

    optics::MaterialLibrary MaterialLibrarySerializer::deserialize(
        const nlohmann::json& document)
    {
        if (
            document.at("format").get<std::string>() !=
            "OpticForgeMaterialLibrary")
        {
            fail("unexpected material library format.");
        }

        if (
            document.at("version").get<int>() !=
            FormatVersion)
        {
            fail("unsupported material library version.");
        }

        optics::MaterialLibrary result;

        const auto& libraryJson =
            document.at("library");

        auto& metadata =
            result.metadata();

        metadata.id = libraryJson.at("id").get<std::string>();
        metadata.name = libraryJson.at("name").get<std::string>();
        metadata.revision = libraryJson.at("revision").get<std::string>();
        metadata.description = libraryJson.value("description", std::string{});
        metadata.manufacturer = libraryJson.value("manufacturer", std::string{});
        metadata.generatedBy = libraryJson.value("generatedBy", std::string{});
        metadata.createdUtc = libraryJson.value("createdUtc", std::string{});

        if (libraryJson.contains("provenance"))
            metadata.provenance =
                deserializeProvenance(
                    libraryJson.at("provenance"));

        for (const auto& materialJson : document.at("materials"))
        {
            optics::Material material =
                deserializeMaterial(materialJson);

            if (!result.add(std::move(material)))
                fail("duplicate or invalid material key.");
        }

        return result;
    }

} // namespace opticforge::project
