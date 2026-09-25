// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "OpticalInterface.h"

#include <algorithm>

namespace opticforge::optics
{

    // -----------------------------------------------------------------------------
    // RefractiveInterface
    // -----------------------------------------------------------------------------

    RefractiveInterface::RefractiveInterface(
        double negativeSideMaterial_,
        double positiveSideMaterial_)
        : negativeSideMaterial(negativeSideMaterial_),
        positiveSideMaterial(positiveSideMaterial_)
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
        double negativeSideMaterial,
        double positiveSideMaterial)
    {
        m_type =
            RefractiveInterface{
                negativeSideMaterial,
                positiveSideMaterial
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