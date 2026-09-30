// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "raytracer/Raytracer.h"

#include <cmath>
#include <iostream>
#include <vector>

using namespace opticforge;

namespace {
int failures = 0;

void check(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

telescope::PrimitiveRecord mirror(const optics::SurfaceGeometry& geometry,
                                    bool rotated = false)
{
    telescope::Mirror m;
    m.surface.setGeometry(geometry);
    m.surface.aperture().setCircular(20.0);
    m.surface.opticalInterface().setReflective(1.0);
    if (rotated)
        m.transform.setEulerDegrees({0.0, 180.0, 0.0});
    return {42, m, "test mirror"};
}

raytracer::RayPath trace(const telescope::PrimitiveRecord& primitive,
                         const glm::dvec3& origin,
                         const glm::dvec3& direction)
{
    telescope::ObservationPlane plane;
    plane.transform.setPosition({0.0, 0.0, 1000.0});
    optics::OpticalRay ray;
    ray.ray = optics::Ray(origin, direction);
    return raytracer::RayTracer{}.traceRay(ray, {primitive}, plane, 1);
}

void expectSide(const telescope::PrimitiveRecord& primitive,
                const glm::dvec3& normal,
                bool fromBack)
{
    const auto path = trace(primitive,
        fromBack ? normal * 10.0 : -normal * 10.0,
        fromBack ? -normal : normal);
    check(path.interactions.size() == 1, "mirror hit records one interaction");
    if (path.interactions.size() != 1) return;
    check(path.interactions.front().primitiveId == 42,
          "interaction identifies mirror");
    if (fromBack) {
        check(path.termination == raytracer::RayTermination::Absorbed,
              "backside ray is absorbed");
        check(!path.interactions.front().outgoing,
              "absorption has no outgoing ray");
    } else {
        check(path.termination == raytracer::RayTermination::MaxInteractions,
              "frontside reflection reaches interaction limit");
        check(path.interactions.front().outgoing.has_value(),
              "frontside reflection has outgoing ray");
        if (path.interactions.front().outgoing)
            check(glm::dot(path.interactions.front().outgoing->ray.direction,
                           normal) < -0.999,
                  "reflected ray travels back toward incident side");
    }
}
}

int main()
{
    // Exercise the parallel bundle entry point, including output ordering.
    telescope::ObservationPlane bundlePlane;
    bundlePlane.transform.setPosition({0.0, 0.0, 1000.0});
    const auto bundleMirror = mirror(optics::PlaneGeometry{});
    std::vector<optics::OpticalRay> bundle(256);
    for (std::size_t i = 0; i < bundle.size(); ++i) {
        const bool fromBack = i % 2 != 0;
        bundle[i].ray = optics::Ray(
            {static_cast<double>(i % 10), 0.0, fromBack ? 10.0 : -10.0},
            {0.0, 0.0, fromBack ? -1.0 : 1.0});
    }
    raytracer::RayTracer tracer;
    const auto bundleResult = tracer.traceRayBundle(
        bundle, {bundleMirror}, bundlePlane, 1);
    check(bundleResult.paths.size() == bundle.size(), "bundle preserves ray count");
    for (std::size_t i = 0; i < bundleResult.paths.size(); ++i) {
        const auto& path = bundleResult.paths[i];
        check(path.initialRay.ray.origin.x == bundle[i].ray.origin.x,
              "parallel bundle preserves input order");
        check(path.termination == (i % 2 ? raytracer::RayTermination::Absorbed
                                        : raytracer::RayTermination::MaxInteractions),
              "parallel bundle keeps each ray's outcome");
    }
    check(tracer.traceRayBundle({}, {bundleMirror}, bundlePlane).paths.empty(),
          "empty bundle yields empty paths");

    expectSide(mirror(optics::PlaneGeometry{}), {0, 0, 1}, false);
    expectSide(mirror(optics::PlaneGeometry{}), {0, 0, 1}, true);
    expectSide(mirror(optics::PlaneGeometry{}, true), {0, 0, -1}, false);
    expectSide(mirror(optics::PlaneGeometry{}, true), {0, 0, -1}, true);

    for (double radius : {100.0, -100.0}) {
        expectSide(mirror(optics::ConicGeometry{radius, 0.0}),
                   {0, 0, 1}, false);
        expectSide(mirror(optics::ConicGeometry{radius, 0.0}),
                   {0, 0, 1}, true);
    }

    // Refractive interfaces must resolve wavelength-dependent material indices.
    optics::MaterialLibrary materials;

    optics::Material vacuum;
    vacuum.key = "opticforge:vacuum";
    vacuum.name = "Vacuum";
    vacuum.optics =
        optics::IsotropicOptics{
            optics::ConstantDispersion{ 1.0, std::nullopt },
            std::nullopt
        };

    check(
        materials.add(vacuum),
        "test vacuum material added");

    optics::Material bk7;
    bk7.key = "test:N-BK7";
    bk7.name = "N-BK7";

    optics::SellmeierDispersion bk7Dispersion;
    bk7Dispersion.B =
        {
            1.03961212,
            0.231792344,
            1.01046945
        };

    bk7Dispersion.C =
        {
            0.00600069867,
            0.0200179144,
            103.560653
        };

    bk7.optics =
        optics::IsotropicOptics{
            bk7Dispersion,
            std::nullopt
        };

    check(
        materials.add(bk7),
        "test N-BK7 material added");

    telescope::Lens refractiveLens;
    refractiveLens.centerThickness = 100.0;
    refractiveLens.frontSurface.setGeometry(
        optics::PlaneGeometry{});
    refractiveLens.frontSurface.aperture().setCircular(
        100.0);
    refractiveLens.frontSurface.opticalInterface().setRefractive(
        "opticforge:vacuum",
        "test:N-BK7");

    refractiveLens.rearSurface.setGeometry(
        optics::PlaneGeometry{});
    refractiveLens.rearSurface.aperture().setCircular(
        100.0);
    refractiveLens.rearSurface.opticalInterface().setRefractive(
        "test:N-BK7",
        "opticforge:vacuum");

    telescope::ObservationPlane refractivePlane;
    refractivePlane.transform.setPosition(
        { 0.0, 0.0, 1000.0 });

    const telescope::PrimitiveRecord refractiveRecord{
        50,
        refractiveLens,
        "test refractive lens"
    };

    const auto traceWavelength =
        [&](double wavelength)
        {
            optics::OpticalRay ray;
            ray.ray =
                optics::Ray(
                    { 0.0, 0.0, -10.0 },
                    glm::normalize(
                        glm::dvec3(
                            0.2,
                            0.0,
                            1.0)));
            ray.wavelength = wavelength;

            return
                raytracer::RayTracer{
                    materials
                }.traceRay(
                    ray,
                    { refractiveRecord },
                    refractivePlane,
                    1);
        };

    const auto bluePath =
        traceWavelength(486.13);

    const auto redPath =
        traceWavelength(656.27);

    check(
        bluePath.interactions.size() == 1 &&
        redPath.interactions.size() == 1,
        "chromatic test rays hit refractive surface");

    if (
        bluePath.interactions.size() == 1 &&
        redPath.interactions.size() == 1 &&
        bluePath.interactions.front().outgoing &&
        redPath.interactions.front().outgoing)
    {
        const double blueX =
            std::abs(
                bluePath.interactions.front().
                outgoing->ray.direction.x);

        const double redX =
            std::abs(
                redPath.interactions.front().
                outgoing->ray.direction.x);

        check(
            blueX < redX,
            "N-BK7 bends blue light more strongly than red light");
    }
    else
    {
        check(
            false,
            "chromatic refractive rays produce outgoing rays");
    }

    // A planar reflective grating should follow m * lambda / d.
    telescope::DiffractionGrating grating;
    grating.surface.setGeometry(
        optics::PlaneGeometry{});

    grating.surface.aperture().setRectangular(
        100.0,
        100.0);

    grating.surface.opticalInterface().setDiffractionGrating(
        600.0,
        1,
        0.0);

    telescope::ObservationPlane gratingPlane;
    gratingPlane.transform.setPosition(
        { 0.0, 0.0, -1000.0 });

    optics::OpticalRay gratingRay;
    gratingRay.ray =
        optics::Ray(
            { 0.0, 0.0, -10.0 },
            { 0.0, 0.0, 1.0 });

    gratingRay.wavelength =
        500.0;

    const auto gratingPath =
        raytracer::RayTracer{}.traceRay(
            gratingRay,
            {
                telescope::PrimitiveRecord{
                    60,
                    grating,
                    "test grating"
                }
            },
            gratingPlane,
            1);

    check(
        gratingPath.interactions.size() == 1,
        "grating ray records one interaction");

    if (
        gratingPath.interactions.size() == 1 &&
        gratingPath.interactions.front().outgoing)
    {
        const auto& direction =
            gratingPath.interactions.front().
            outgoing->ray.direction;

        check(
            std::abs(direction.x - 0.3) <
                1.0e-12,
            "600 groove/mm grating gives expected first-order X direction cosine at 500 nm");

        check(
            std::abs(
                direction.z +
                std::sqrt(1.0 - 0.3 * 0.3)) <
                1.0e-12,
            "grating reflects into the incident half-space");
    }
    else
    {
        check(
            false,
            "grating produces an outgoing first-order ray");
    }

    // Rotating the grooves by +90 degrees should rotate dispersion from +X
    // to -Y under the documented +Y -> +X groove-angle convention.
    telescope::DiffractionGrating rotatedGrating =
        grating;

    rotatedGrating.surface.opticalInterface().setDiffractionGrating(
        600.0,
        1,
        90.0);

    const auto rotatedGratingPath =
        raytracer::RayTracer{}.traceRay(
            gratingRay,
            {
                telescope::PrimitiveRecord{
                    61,
                    rotatedGrating,
                    "rotated test grating"
                }
            },
            gratingPlane,
            1);

    if (
        rotatedGratingPath.interactions.size() == 1 &&
        rotatedGratingPath.interactions.front().outgoing)
    {
        const auto& direction =
            rotatedGratingPath.interactions.front().
            outgoing->ray.direction;

        check(
            std::abs(direction.y + 0.3) <
                1.0e-12,
            "groove rotation rotates the dispersion direction");
    }
    else
    {
        check(
            false,
            "rotated grating produces an outgoing ray");
    }

    telescope::DiffractionGrating unavailableGrating =
        grating;

    unavailableGrating.surface.opticalInterface().setDiffractionGrating(
        2400.0,
        1,
        0.0);

    gratingRay.wavelength =
        700.0;

    const auto unavailablePath =
        raytracer::RayTracer{}.traceRay(
            gratingRay,
            {
                telescope::PrimitiveRecord{
                    62,
                    unavailableGrating,
                    "unavailable order grating"
                }
            },
            gratingPlane,
            1);

    check(
        unavailablePath.termination ==
            raytracer::RayTermination::Blocked,
        "non-propagating diffraction order is blocked");

    // The sign test must be limited to mirrors: detector incidence is two-sided.
    telescope::Detector detector;
    detector.surface.setGeometry(optics::PlaneGeometry{});
    detector.surface.aperture().setCircular(20.0);
    detector.surface.opticalInterface().setDetector();
    auto detectorPath = trace({43, detector, "detector"},
                              {0, 0, 10}, {0, 0, -1});
    check(detectorPath.termination == raytracer::RayTermination::DetectorHit,
          "detector still accepts negative incidence");

    if (failures) std::cerr << failures << " assertion(s) failed\n";
    return failures ? 1 : 0;
}
