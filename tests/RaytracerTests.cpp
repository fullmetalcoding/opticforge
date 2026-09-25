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
