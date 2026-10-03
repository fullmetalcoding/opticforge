// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "MaterialLibraryWindow.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <type_traits>

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

namespace opticforge::ui
{
    namespace
    {
        const char* materialCategoryName(
            optics::MaterialCategory value)
        {
            switch (value)
            {
            case optics::MaterialCategory::OpticalGlass: return "Optical glass";
            case optics::MaterialCategory::FusedSilica: return "Fused silica";
            case optics::MaterialCategory::Crystal: return "Crystal";
            case optics::MaterialCategory::Ceramic: return "Ceramic";
            case optics::MaterialCategory::Polymer: return "Polymer";
            case optics::MaterialCategory::Liquid: return "Liquid";
            case optics::MaterialCategory::Gas: return "Gas";
            case optics::MaterialCategory::Other: return "Other";
            }

            return "Other";
        }

        const char* modelName(
            const optics::RefractiveIndexModel& model)
        {
            if (std::holds_alternative<optics::ConstantDispersion>(model)) return "Constant";
            if (std::holds_alternative<optics::SellmeierDispersion>(model)) return "Sellmeier";
            if (std::holds_alternative<optics::CauchyDispersion>(model)) return "Cauchy";
            if (std::holds_alternative<optics::SchottDispersion>(model)) return "Schott";
            if (std::holds_alternative<optics::TabulatedDispersion>(model)) return "Tabulated";
            return "Air";
        }

        int modelIndex(
            const optics::RefractiveIndexModel& model)
        {
            if (std::holds_alternative<optics::ConstantDispersion>(model)) return 0;
            if (std::holds_alternative<optics::SellmeierDispersion>(model)) return 1;
            if (std::holds_alternative<optics::CauchyDispersion>(model)) return 2;
            if (std::holds_alternative<optics::SchottDispersion>(model)) return 3;
            if (std::holds_alternative<optics::TabulatedDispersion>(model)) return 4;
            return 5;
        }

        optics::RefractiveIndexModel defaultModel(
            int index)
        {
            switch (index)
            {
            case 0: return optics::ConstantDispersion{};
            case 1: return optics::SellmeierDispersion{};
            case 2: return optics::CauchyDispersion{};
            case 3: return optics::SchottDispersion{};
            case 4: return optics::TabulatedDispersion{};
            default: return optics::AirDispersion{};
            }
        }

        void drawWavelengthRange(
            std::optional<optics::WavelengthRangeNm>& range)
        {
            bool enabled = range.has_value();

            if (ImGui::Checkbox("Limit wavelength range", &enabled))
            {
                if (enabled && !range)
                    range = optics::WavelengthRangeNm{};
                else if (!enabled)
                    range.reset();
            }

            if (range)
            {
                ImGui::InputDouble(
                    "Minimum wavelength (nm)",
                    &range->min,
                    1.0,
                    10.0,
                    "%.3f");

                ImGui::InputDouble(
                    "Maximum wavelength (nm)",
                    &range->max,
                    1.0,
                    10.0,
                    "%.3f");
            }
        }

        void drawVector(
            const char* id,
            std::vector<double>& values,
            std::size_t minCount,
            std::size_t maxCount,
            const char* prefix)
        {
            ImGui::PushID(id);

            for (std::size_t i = 0; i < values.size(); ++i)
            {
                ImGui::PushID(static_cast<int>(i));

                const std::string label =
                    std::string(prefix) +
                    std::to_string(i + 1);

                ImGui::InputDouble(
                    label.c_str(),
                    &values[i],
                    0.001,
                    0.01,
                    "%.12g");

                ImGui::SameLine();

                if (
                    values.size() > minCount &&
                    ImGui::SmallButton("Remove"))
                {
                    values.erase(
                        values.begin() +
                        static_cast<std::ptrdiff_t>(i));

                    ImGui::PopID();
                    break;
                }

                ImGui::PopID();
            }

            if (
                values.size() < maxCount &&
                ImGui::SmallButton("Add coefficient"))
            {
                values.push_back(0.0);
            }

            ImGui::PopID();
        }

        bool finitePositive(double value)
        {
            return
                std::isfinite(value) &&
                value > 0.0;
        }

        bool validRefractiveModel(
            const optics::RefractiveIndexModel& model)
        {
            return std::visit(
                [](const auto& value)
                {
                    using T =
                        std::decay_t<decltype(value)>;

                    if constexpr (
                        std::is_same_v<T, optics::ConstantDispersion>)
                    {
                        return
                            finitePositive(
                                value.refractiveIndex);
                    }
                    else if constexpr (
                        std::is_same_v<T, optics::SellmeierDispersion>)
                    {
                        return
                            !value.B.empty() &&
                            value.B.size() == value.C.size() &&
                            value.B.size() <= 8;
                    }
                    else if constexpr (
                        std::is_same_v<T, optics::CauchyDispersion>)
                    {
                        return
                            !value.coefficients.empty() &&
                            value.coefficients.size() <= 8;
                    }
                    else if constexpr (
                        std::is_same_v<T, optics::SchottDispersion>)
                    {
                        return true;
                    }
                    else if constexpr (
                        std::is_same_v<T, optics::TabulatedDispersion>)
                    {
                        if (value.samples.size() < 2)
                            return false;

                        for (std::size_t i = 0; i < value.samples.size(); ++i)
                        {
                            if (
                                !finitePositive(value.samples[i].wavelengthNm) ||
                                !finitePositive(value.samples[i].n))
                            {
                                return false;
                            }

                            if (
                                i > 0 &&
                                value.samples[i - 1].wavelengthNm >=
                                value.samples[i].wavelengthNm)
                            {
                                return false;
                            }
                        }

                        return true;
                    }
                    else
                    {
                        return
                            finitePositive(
                                value.conditions.pressurePa) &&
                            value.conditions.relativeHumidity >= 0.0 &&
                            value.conditions.relativeHumidity <= 1.0 &&
                            value.conditions.co2Ppm >= 0.0;
                    }
                },
                model);
        }
    }

    void MaterialLibraryWindow::beginNew(
        const optics::MaterialLibrary& library)
    {
        m_editMaterial =
            optics::Material{};

        const std::string prefix =
            library.metadata().id.empty()
            ? "local"
            : library.metadata().id;

        std::string key =
            prefix + ":new-material";

        int suffix = 2;

        while (library.contains(key))
        {
            key =
                prefix +
                ":new-material-" +
                std::to_string(suffix++);
        }

        m_editMaterial.key =
            std::move(key);

        m_originalKey.reset();
        m_error.clear();
        m_editing = true;
    }

    void MaterialLibraryWindow::beginEdit(
        const optics::MaterialLibrary& library)
    {
        const optics::Material* material =
            library.find(m_selectedKey);

        if (!material)
            return;

        m_editMaterial =
            *material;

        m_originalKey =
            material->key;

        m_error.clear();
        m_editing = true;
    }

    bool MaterialLibraryWindow::validMaterial(
        const optics::Material& material)
    {
        if (
            material.key.empty() ||
            material.name.empty() ||
            material.key.find(':') ==
                std::string::npos)
        {
            return false;
        }

        return std::visit(
            [](const auto& value)
            {
                using T =
                    std::decay_t<decltype(value)>;

                if constexpr (
                    std::is_same_v<T, optics::IsotropicOptics>)
                {
                    return
                        validRefractiveModel(
                            value.refractiveIndex);
                }
                else if constexpr (
                    std::is_same_v<T, optics::UniaxialOptics>)
                {
                    return
                        validRefractiveModel(
                            value.ordinaryIndex) &&
                        validRefractiveModel(
                            value.extraordinaryIndex);
                }
                else
                {
                    return
                        validRefractiveModel(value.xIndex) &&
                        validRefractiveModel(value.yIndex) &&
                        validRefractiveModel(value.zIndex);
                }
            },
            material.optics);
    }

    void MaterialLibraryWindow::drawRefractiveIndexModel(
        const char* id,
        optics::RefractiveIndexModel& model)
    {
        ImGui::PushID(id);

        int index =
            modelIndex(model);

        const char* names[] =
        {
            "Constant",
            "Sellmeier",
            "Cauchy",
            "Schott",
            "Tabulated",
            "Air"
        };

        if (ImGui::Combo(
            "Dispersion model",
            &index,
            names,
            IM_ARRAYSIZE(names)))
        {
            model =
                defaultModel(index);
        }

        ImGui::TextDisabled(
            "Current model: %s",
            modelName(model));

        std::visit(
            [](auto& value)
            {
                using T =
                    std::decay_t<decltype(value)>;

                if constexpr (
                    std::is_same_v<T, optics::ConstantDispersion>)
                {
                    ImGui::InputDouble(
                        "Refractive index",
                        &value.refractiveIndex,
                        0.001,
                        0.01,
                        "%.12g");

                    drawWavelengthRange(
                        value.validWavelengthNm);
                }
                else if constexpr (
                    std::is_same_v<T, optics::SellmeierDispersion>)
                {
                    drawVector(
                        "B",
                        value.B,
                        1,
                        8,
                        "B");

                    drawVector(
                        "C",
                        value.C,
                        1,
                        8,
                        "C");

                    ImGui::TextDisabled(
                        "Sellmeier coefficient wavelength unit: um");

                    drawWavelengthRange(
                        value.validWavelengthNm);
                }
                else if constexpr (
                    std::is_same_v<T, optics::CauchyDispersion>)
                {
                    drawVector(
                        "Cauchy",
                        value.coefficients,
                        1,
                        8,
                        "A");

                    ImGui::TextDisabled(
                        "Cauchy coefficient wavelength unit: um");

                    drawWavelengthRange(
                        value.validWavelengthNm);
                }
                else if constexpr (
                    std::is_same_v<T, optics::SchottDispersion>)
                {
                    for (
                        std::size_t i = 0;
                        i < value.coefficients.size();
                        ++i)
                    {
                        const std::string label =
                            "A" +
                            std::to_string(i);

                        ImGui::InputDouble(
                            label.c_str(),
                            &value.coefficients[i],
                            0.001,
                            0.01,
                            "%.12g");
                    }

                    ImGui::TextDisabled(
                        "Schott coefficient wavelength unit: um");

                    drawWavelengthRange(
                        value.validWavelengthNm);
                }
                else if constexpr (
                    std::is_same_v<T, optics::TabulatedDispersion>)
                {
                    int interpolation =
                        value.interpolation ==
                        optics::InterpolationMode::Cubic
                        ? 1
                        : 0;

                    const char* interpolationNames[] =
                    {
                        "Linear",
                        "Cubic"
                    };

                    if (ImGui::Combo(
                        "Interpolation",
                        &interpolation,
                        interpolationNames,
                        2))
                    {
                        value.interpolation =
                            interpolation == 1
                            ? optics::InterpolationMode::Cubic
                            : optics::InterpolationMode::Linear;
                    }

                    int extrapolation =
                        value.extrapolation ==
                        optics::ExtrapolationMode::Clamp
                        ? 1
                        : 0;

                    const char* extrapolationNames[] =
                    {
                        "Error",
                        "Clamp"
                    };

                    if (ImGui::Combo(
                        "Extrapolation",
                        &extrapolation,
                        extrapolationNames,
                        2))
                    {
                        value.extrapolation =
                            extrapolation == 1
                            ? optics::ExtrapolationMode::Clamp
                            : optics::ExtrapolationMode::Error;
                    }

                    for (
                        std::size_t i = 0;
                        i < value.samples.size();
                        ++i)
                    {
                        ImGui::PushID(
                            static_cast<int>(i));

                        ImGui::InputDouble(
                            "Wavelength (nm)",
                            &value.samples[i].wavelengthNm,
                            1.0,
                            10.0,
                            "%.3f");

                        ImGui::SameLine();

                        ImGui::InputDouble(
                            "n",
                            &value.samples[i].n,
                            0.001,
                            0.01,
                            "%.12g");

                        ImGui::SameLine();

                        if (
                            value.samples.size() > 2 &&
                            ImGui::SmallButton("Remove"))
                        {
                            value.samples.erase(
                                value.samples.begin() +
                                static_cast<std::ptrdiff_t>(i));

                            ImGui::PopID();
                            break;
                        }

                        ImGui::PopID();
                    }

                    if (ImGui::SmallButton("Add sample"))
                    {
                        const double wavelength =
                            value.samples.empty()
                            ? 550.0
                            : value.samples.back().wavelengthNm +
                                10.0;

                        value.samples.push_back(
                            {
                                wavelength,
                                value.samples.empty()
                                    ? 1.5
                                    : value.samples.back().n
                            });
                    }
                }
                else
                {
                    int formula =
                        value.formula ==
                        optics::AirFormula::Edlen1966
                        ? 1
                        : 0;

                    const char* formulaNames[] =
                    {
                        "Ciddor 1996",
                        "Edlen 1966"
                    };

                    if (ImGui::Combo(
                        "Formula",
                        &formula,
                        formulaNames,
                        2))
                    {
                        value.formula =
                            formula == 1
                            ? optics::AirFormula::Edlen1966
                            : optics::AirFormula::Ciddor1996;
                    }

                    ImGui::InputDouble(
                        "Temperature (C)",
                        &value.conditions.temperatureC,
                        0.1,
                        1.0,
                        "%.3f");

                    ImGui::InputDouble(
                        "Pressure (Pa)",
                        &value.conditions.pressurePa,
                        10.0,
                        100.0,
                        "%.3f");

                    ImGui::InputDouble(
                        "Relative humidity",
                        &value.conditions.relativeHumidity,
                        0.01,
                        0.1,
                        "%.4f");

                    ImGui::InputDouble(
                        "CO2 (ppm)",
                        &value.conditions.co2Ppm,
                        1.0,
                        10.0,
                        "%.3f");

                    drawWavelengthRange(
                        value.validWavelengthNm);
                }
            },
            model);

        ImGui::PopID();
    }

    void MaterialLibraryWindow::drawEditor(
        optics::MaterialLibrary& library,
        const MaterialLibraryCommands& commands)
    {
        if (!m_editing)
            return;

        if (!ImGui::Begin(
            m_originalKey
                ? "Edit Material"
                : "New Material",
            &m_editing,
            ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::End();
            return;
        }

        ImGui::InputText(
            "Key",
            &m_editMaterial.key);

        ImGui::InputText(
            "Name",
            &m_editMaterial.name);

        ImGui::InputTextMultiline(
            "Description",
            &m_editMaterial.description,
            ImVec2(500.0f, 80.0f));

        bool hasCategory =
            m_editMaterial.category.has_value();

        if (ImGui::Checkbox(
            "Set category",
            &hasCategory))
        {
            if (hasCategory)
                m_editMaterial.category =
                    optics::MaterialCategory::OpticalGlass;
            else
                m_editMaterial.category.reset();
        }

        if (m_editMaterial.category)
        {
            int category =
                static_cast<int>(
                    *m_editMaterial.category);

            const char* categories[] =
            {
                "Optical glass",
                "Fused silica",
                "Crystal",
                "Ceramic",
                "Polymer",
                "Liquid",
                "Gas",
                "Other"
            };

            if (ImGui::Combo(
                "Category",
                &category,
                categories,
                IM_ARRAYSIZE(categories)))
            {
                m_editMaterial.category =
                    static_cast<optics::MaterialCategory>(
                        category);
            }
        }

        ImGui::SeparatorText(
            "Optical Properties");

        int opticalType =
            std::holds_alternative<
                optics::IsotropicOptics>(
                    m_editMaterial.optics)
            ? 0
            : std::holds_alternative<
                optics::UniaxialOptics>(
                    m_editMaterial.optics)
                ? 1
                : 2;

        const char* opticalTypes[] =
        {
            "Isotropic",
            "Uniaxial",
            "Biaxial"
        };

        if (ImGui::Combo(
            "Material symmetry",
            &opticalType,
            opticalTypes,
            3))
        {
            if (opticalType == 0)
                m_editMaterial.optics =
                    optics::IsotropicOptics{};
            else if (opticalType == 1)
                m_editMaterial.optics =
                    optics::UniaxialOptics{};
            else
                m_editMaterial.optics =
                    optics::BiaxialOptics{};
        }

        std::visit(
            [&](auto& value)
            {
                using T =
                    std::decay_t<decltype(value)>;

                if constexpr (
                    std::is_same_v<T, optics::IsotropicOptics>)
                {
                    drawRefractiveIndexModel(
                        "isotropic",
                        value.refractiveIndex);
                }
                else if constexpr (
                    std::is_same_v<T, optics::UniaxialOptics>)
                {
                    ImGui::SeparatorText(
                        "Ordinary index");

                    drawRefractiveIndexModel(
                        "ordinary",
                        value.ordinaryIndex);

                    ImGui::SeparatorText(
                        "Extraordinary index");

                    drawRefractiveIndexModel(
                        "extraordinary",
                        value.extraordinaryIndex);

                    bool hasAxis =
                        value.opticAxis.has_value();

                    if (ImGui::Checkbox(
                        "Specify optic axis",
                        &hasAxis))
                    {
                        if (hasAxis)
                            value.opticAxis =
                                std::array<double, 3>{
                                    0.0, 0.0, 1.0
                                };
                        else
                            value.opticAxis.reset();
                    }

                    if (value.opticAxis)
                    {
                        ImGui::InputDouble(
                            "Optic axis X",
                            &(*value.opticAxis)[0],
                            0.01,
                            0.1,
                            "%.6f");

                        ImGui::InputDouble(
                            "Optic axis Y",
                            &(*value.opticAxis)[1],
                            0.01,
                            0.1,
                            "%.6f");

                        ImGui::InputDouble(
                            "Optic axis Z",
                            &(*value.opticAxis)[2],
                            0.01,
                            0.1,
                            "%.6f");
                    }
                }
                else
                {
                    ImGui::SeparatorText("X index");
                    drawRefractiveIndexModel(
                        "x",
                        value.xIndex);

                    ImGui::SeparatorText("Y index");
                    drawRefractiveIndexModel(
                        "y",
                        value.yIndex);

                    ImGui::SeparatorText("Z index");
                    drawRefractiveIndexModel(
                        "z",
                        value.zIndex);
                }
            },
            m_editMaterial.optics);

        if (!m_error.empty())
        {
            ImGui::TextColored(
                ImVec4(1.0f, 0.4f, 0.4f, 1.0f),
                "%s",
                m_error.c_str());
        }

        const bool valid =
            validMaterial(
                m_editMaterial);

        if (!valid)
            ImGui::BeginDisabled();

        if (ImGui::Button("Apply"))
        {
            bool changed = false;

            if (m_originalKey)
            {
                changed =
                    library.update(
                        *m_originalKey,
                        m_editMaterial);
            }
            else
            {
                changed =
                    library.add(
                        m_editMaterial);
            }

            if (changed)
            {
                m_selectedKey =
                    m_editMaterial.key;

                m_editing = false;
                m_originalKey.reset();
                m_error.clear();

                if (commands.changed)
                    commands.changed();
            }
            else
            {
                m_error =
                    "Material key is invalid or already exists.";
            }
        }

        if (!valid)
            ImGui::EndDisabled();

        ImGui::SameLine();

        if (ImGui::Button("Cancel"))
        {
            m_editing = false;
            m_originalKey.reset();
            m_error.clear();
        }

        ImGui::End();
    }

    void MaterialLibraryWindow::draw(
        optics::MaterialLibrary& library,
        const MaterialLibraryCommands& commands)
    {
        if (!m_visible)
            return;

        if (ImGui::Begin(
            "Material Library",
            &m_visible,
            ImGuiWindowFlags_MenuBar))
        {
            if (ImGui::BeginMenuBar())
            {
                if (
                    ImGui::BeginMenu(
                        "Library"))
                {
                    if (
                        ImGui::MenuItem("Load...") &&
                        commands.load)
                    {
                        commands.load();
                    }

                    if (
                        ImGui::MenuItem("Save") &&
                        commands.save)
                    {
                        commands.save();
                    }

                    if (
                        ImGui::MenuItem("Save As...") &&
                        commands.saveAs)
                    {
                        commands.saveAs();
                    }

                    ImGui::EndMenu();
                }

                ImGui::EndMenuBar();
            }

            auto& metadata =
                library.metadata();

            if (ImGui::CollapsingHeader(
                "Library metadata"))
            {
                ImGui::InputText(
                    "Library ID",
                    &metadata.id);

                ImGui::InputText(
                    "Library name",
                    &metadata.name);

                ImGui::InputText(
                    "Revision",
                    &metadata.revision);

                ImGui::InputText(
                    "Manufacturer",
                    &metadata.manufacturer);

                ImGui::InputTextMultiline(
                    "Library description",
                    &metadata.description,
                    ImVec2(500.0f, 60.0f));
            }

            ImGui::Separator();

            if (ImGui::Button("New"))
                beginNew(library);

            ImGui::SameLine();

            const bool hasSelection =
                library.contains(
                    m_selectedKey);

            if (!hasSelection)
                ImGui::BeginDisabled();

            if (ImGui::Button("Edit"))
                beginEdit(library);

            ImGui::SameLine();

            if (ImGui::Button("Remove"))
            {
                if (library.remove(
                    m_selectedKey))
                {
                    m_selectedKey.clear();

                    if (commands.changed)
                        commands.changed();
                }
            }

            if (!hasSelection)
                ImGui::EndDisabled();

            ImGui::SameLine();

            if (
                ImGui::Button("Save") &&
                commands.save)
            {
                commands.save();
            }

            ImGui::SameLine();

            if (
                ImGui::Button("Load...") &&
                commands.load)
            {
                commands.load();
            }

            ImGui::Separator();

            if (ImGui::BeginTable(
                "Materials",
                4,
                ImGuiTableFlags_Borders |
                ImGuiTableFlags_RowBg |
                ImGuiTableFlags_Resizable |
                ImGuiTableFlags_ScrollY,
                ImVec2(760.0f, 360.0f)))
            {
                ImGui::TableSetupColumn("Name");
                ImGui::TableSetupColumn("Key");
                ImGui::TableSetupColumn("Category");
                ImGui::TableSetupColumn("Optics");
                ImGui::TableHeadersRow();

                for (
                    const auto& material :
                    library.materials())
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);

                    const bool selected =
                        m_selectedKey ==
                        material.key;

                    if (ImGui::Selectable(
                        material.name.c_str(),
                        selected,
                        ImGuiSelectableFlags_SpanAllColumns))
                    {
                        m_selectedKey =
                            material.key;
                    }

                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextUnformatted(
                        material.key.c_str());

                    ImGui::TableSetColumnIndex(2);

                    if (material.category)
                        ImGui::TextUnformatted(
                            materialCategoryName(
                                *material.category));
                    else
                        ImGui::TextUnformatted("-");

                    ImGui::TableSetColumnIndex(3);

                    if (std::holds_alternative<
                        optics::IsotropicOptics>(
                            material.optics))
                    {
                        ImGui::TextUnformatted(
                            "Isotropic");
                    }
                    else if (std::holds_alternative<
                        optics::UniaxialOptics>(
                            material.optics))
                    {
                        ImGui::TextUnformatted(
                            "Uniaxial");
                    }
                    else
                    {
                        ImGui::TextUnformatted(
                            "Biaxial");
                    }
                }

                ImGui::EndTable();
            }
        }

        ImGui::End();

        drawEditor(
            library,
            commands);
    }

} // namespace opticforge::ui
