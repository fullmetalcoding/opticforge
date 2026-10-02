// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "OpticalInterface.h"

#include <algorithm>
#include <utility>

namespace opticforge::optics
{

    // -----------------------------------------------------------------------------
    // RefractiveInterface
    // -----------------------------------------------------------------------------

    RefractiveInterface::RefractiveInterface(
        std::string negativeSideMaterial_,
        std::string positiveSideMaterial_)
        :
        negativeSideMaterial(
            std::move(negativeSideMaterial_)),
        positiveSideMaterial(
            std::move(positiveSideMaterial_))
    {
    }


    // -----------------------------------------------------------------------------
    // ReflectiveInterface
    // -----------------------------------------------------------------------------

    ReflectiveInterface::ReflectiveInterface(
        double reflectivity_)
        : reflectivity(
            std::clamp(
                reflectivity_,
                0.0,
                1.0))
    {
    }


    // -----------------------------------------------------------------------------
    // OpticalInterface
    // -----------------------------------------------------------------------------

    OpticalInterface::OpticalInterface()
        : m_type(
            AbsorbingInterface{})
    {
    }

    OpticalInterface::OpticalInterface(
        const RefractiveInterface& interface)
        : m_type(interface)
    {
    }

    OpticalInterface::OpticalInterface(
        const ReflectiveInterface& interface)
        : m_type(interface)
    {
    }

    OpticalInterface::OpticalInterface(
        const DiffractionGratingInterface& interface)
        : m_type(interface)
    {
    }

    OpticalInterface::OpticalInterface(
        const DetectorInterface& interface)
        : m_type(interface)
    {
    }

    OpticalInterface::OpticalInterface(
        const AbsorbingInterface& interface)
        : m_type(interface)
    {
    }

    OpticalInterface::OpticalInterface(
        const OpticalInterfaceType& interface)
        : m_type(interface)
    {
    }

    const OpticalInterfaceType&
        OpticalInterface::type() const
    {
        return m_type;
    }

    OpticalInterfaceType&
        OpticalInterface::type()
    {
        return m_type;
    }

    void OpticalInterface::setType(
        const OpticalInterfaceType& interface)
    {
        m_type = interface;
    }

    void OpticalInterface::setRefractive(
        std::string negativeSideMaterial,
        std::string positiveSideMaterial)
    {
        m_type =
            RefractiveInterface{
                std::move(negativeSideMaterial),
                std::move(positiveSideMaterial)
        };
    }

    void OpticalInterface::setReflective(
        double reflectivity)
    {
        m_type =
            ReflectiveInterface{
                reflectivity
        };
    }

    void OpticalInterface::setDiffractionGrating(
        double groovesPerMm,
        int order,
        double grooveAngleDegrees)
    {
        m_type =
            DiffractionGratingInterface{
                groovesPerMm,
                order,
                grooveAngleDegrees
            };
    }

    void OpticalInterface::setDetector()
    {
        m_type =
            DetectorInterface{};
    }

    void OpticalInterface::setAbsorbing()
    {
        m_type =
            AbsorbingInterface{};
    }

    bool OpticalInterface::isRefractive() const
    {
        return std::holds_alternative<
            RefractiveInterface>(m_type);
    }

    bool OpticalInterface::isReflective() const
    {
        return std::holds_alternative<
            ReflectiveInterface>(m_type);
    }

    bool OpticalInterface::isDiffractionGrating() const
    {
        return std::holds_alternative<
            DiffractionGratingInterface>(m_type);
    }

    bool OpticalInterface::isDetector() const
    {
        return std::holds_alternative<
            DetectorInterface>(m_type);
    }

    bool OpticalInterface::isAbsorbing() const
    {
        return std::holds_alternative<
            AbsorbingInterface>(m_type);
    }

} // namespace opticforge::optics