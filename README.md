# OpticForge

OpticForge is an interactive telescope and optical-system design application focused on geometric ray tracing, telescope modeling, and optical performance visualization.

The project is intended to provide an accessible environment for designing and experimenting with astronomical optical systems, including Newtonian, Cassegrain-family, refractor, and other telescope configurations.

OpticForge is currently under active development.

## Features

Current capabilities include:

* Interactive 3D optical-system visualization
* Geometric ray tracing through telescope systems
* Spherical, parabolic, and hyperbolic mirror surfaces
* Refractive lens elements
* Annular optical surfaces and central obstructions
* Configurable launch pupil
* Configurable observation / focal plane
* On-axis and off-axis ray tracing
* Spot / PSF visualization based on geometric ray intersections
* Multi-threaded ray tracing
* Visualization of ray paths through the optical system
* Interactive placement and orientation of optical components
* Project serialization and loading
* JSON-based project representation
* Cross-platform C++ codebase
* SDL3 / OpenGL / ImGui user interface

OpticForge is being developed toward more complete physical-optics simulation, including complex wavefront propagation, diffraction, chromatic analysis, and MTF/OTF calculation.

## Project Goals

OpticForge is intended to be useful for:

* Amateur telescope design
* Amateur telescope making
* Optical experimentation
* Telescope prescription development
* Educational use
* Academic and noncommercial research
* Visualization of optical-system geometry
* Comparison of telescope designs
* Development and testing of ray-tracing and optical-analysis techniques

The project is not intended to replace certified engineering analysis where safety, regulatory compliance, or production qualification requires validated commercial optical-design software.

## Building

OpticForge uses CMake and is written in C++.

The project currently targets Windows and Linux.

A typical build is:

```bash
git clone https://github.com/fullmetalcoding/opticforge.git
cd opticforge

cmake -S . -B build
cmake --build build --config Release
```

On multi-configuration generators such as Visual Studio, the desired configuration can be selected when building.

Dependencies are managed through the project's CMake configuration where applicable.

See the repository's CMake files and build documentation for platform-specific requirements.

## Basic Use

An OpticForge project consists of an optical scene containing components such as:

* mirrors;
* lenses;
* launch pupils;
* observation planes; and
* other optical primitives.

Components can be positioned and oriented to construct a telescope or other optical system.

A ray trace can then be performed from the launch pupil through the system. Results can be inspected using the 3D ray-path display and focal-plane / PSF visualization tools.

Off-axis sources can be simulated to examine field-dependent aberrations.

## Development Status

OpticForge is experimental software and APIs, file formats, project schemas, and optical models may change as development progresses.

Areas planned or under development include:

* diffraction simulation;
* complex wavefront tracing;
* optical path difference and phase tracking;
* MTF and OTF calculation;
* wavelength-dependent tracing;
* chromatic aberration analysis;
* aspheric and even-aspheric surfaces;
* Schmidt and Maksutov corrector support;
* improved PSF visualization;
* additional optical-analysis tools; and
* expanded telescope-design workflows.

Bug reports, testing, and technical discussion are welcome.

## License

OpticForge is source-available software licensed under the **PolyForm Noncommercial License 1.0.0**.

See [LICENSE](LICENSE) for the complete licensing terms.

In general, the public license is intended to allow broad noncommercial use, including:

* personal use;
* hobby use;
* amateur astronomy;
* amateur telescope making;
* experimentation;
* education; and
* qualifying academic and research use.

Commercial use is not granted by the standard OpticForge public license.

### Small-Scale Artisan and Amateur Commercial Use

OpticForge is intended to support the amateur telescope-making community, including people who occasionally sell equipment or optics they make themselves.

If you are an amateur telescope maker, mirror maker, telescope builder, optical artisan, or other small-scale maker who would like to sell products designed, analyzed, or validated using OpticForge, please contact the project maintainer.

**No-cost or low-cost commercial licenses may be available for small-scale artisan production.**

The purpose of this policy is to allow individual makers and small-scale craftspeople to use OpticForge without treating occasional sales in the same manner as commercial-scale optical manufacturing.

Permission for commercial use must be obtained before using OpticForge for such purposes.

Commercial organizations, manufacturers, engineering firms, software vendors, and other businesses interested in using OpticForge should contact the project maintainer regarding commercial licensing.

## Contributions

Contributions, bug reports, testing, documentation improvements, and feature proposals are welcome.

Before making substantial code contributions, please review the project's contribution terms.

Because OpticForge may be made available under separate commercial licenses, contributions may require an agreement allowing the project maintainer to distribute contributed code under both the public OpticForge license and separate commercial licenses.

See `CONTRIBUTING.md` for details.

## Reporting Issues

Please use the GitHub issue tracker for:

* bug reports;
* feature requests;
* optical-modeling issues;
* build problems; and
* reproducible ray-tracing problems.

When reporting an optical-modeling problem, including the project file, optical prescription, or enough information to reproduce the system is particularly helpful.

## Disclaimer

OpticForge is provided without warranty.

Optical calculations and simulations should be independently verified before being relied upon for manufacturing, safety-critical systems, regulated applications, or other uses where incorrect results could cause significant loss or harm.
