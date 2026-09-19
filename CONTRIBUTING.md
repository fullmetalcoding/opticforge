# Contributing to OpticForge

Thank you for your interest in contributing to OpticForge.

Contributions are welcome in the form of bug fixes, new optical models, rendering improvements, performance improvements, documentation, tests, platform support, and other enhancements.

OpticForge is developed as a source-available project under the PolyForm Noncommercial License 1.0.0. The project maintainer may also distribute OpticForge under separate commercial licenses.

Because of this dual-licensing model, contributions are accepted subject to the contribution terms described below.

## Getting Started

Before beginning substantial work, consider opening a GitHub issue to discuss the proposed change.

This is especially encouraged for:

* major architectural changes;
* new optical surface models;
* changes to the ray-tracing model;
* diffraction or physical-optics features;
* serialization or project-format changes;
* major user-interface changes; and
* new external dependencies.

Small bug fixes and documentation improvements generally do not require prior discussion.

## Building OpticForge

OpticForge is written in C++ and built with CMake.

A typical build is:

```bash
git clone https://github.com/fullmetalcoding/opticforge.git
cd opticforge

cmake -S . -B build
cmake --build build --config Release
```

Platform-specific requirements and dependency configuration may change as the project develops. Refer to the repository's CMake configuration and README for current requirements.

## Pull Requests

Pull requests should be focused on a specific change whenever practical.

Please ensure that:

* the project builds successfully on the platforms affected by your change;
* existing functionality is not intentionally broken without prior discussion;
* new behavior is tested where practical;
* new optical calculations are documented sufficiently to explain the underlying model;
* serialization changes maintain appropriate compatibility or include corresponding schema changes;
* unrelated formatting or refactoring is kept separate from functional changes when practical; and
* third-party code is not added unless its license is compatible with OpticForge's licensing model.

For changes involving optical calculations, references to relevant textbooks, papers, standards, or mathematical derivations are appreciated.

## Coding Style

Please follow the existing style and organization of the surrounding OpticForge code.

In general:

* prefer clear, explicit C++ over unnecessarily clever implementations;
* keep rendering, optical modeling, ray tracing, project state, and UI responsibilities appropriately separated;
* avoid introducing global state where practical;
* use RAII and standard C++ ownership patterns;
* document non-obvious numerical algorithms and optical assumptions; and
* avoid unnecessary external dependencies.

Existing code should generally be used as the style reference.

# Contribution License Agreement

The following terms apply to any contribution submitted to OpticForge.

By intentionally submitting a contribution to the OpticForge project, including through a pull request, patch, commit, issue attachment, or other contribution mechanism intended for incorporation into the project, you agree to the following terms.

## 1. Ownership

You retain ownership of the copyright in your contribution.

Nothing in this agreement transfers ownership of your contribution to the OpticForge project maintainer.

You remain free to use, license, publish, or distribute your contribution independently, subject to any rights belonging to third parties.

## 2. License Grant to the OpticForge Project

You grant the OpticForge project maintainer a perpetual, worldwide, non-exclusive, royalty-free, irrevocable license to:

* use your contribution;
* reproduce your contribution;
* modify your contribution;
* prepare derivative works based on your contribution;
* combine your contribution with other software;
* publicly display and publicly perform your contribution where applicable;
* distribute your contribution and derivative works;
* sublicense your contribution; and
* relicense your contribution, in original or modified form, under other licenses.

This grant specifically includes the right to distribute your contribution as part of OpticForge under:

1. the PolyForm Noncommercial License 1.0.0 or another future public license used by the OpticForge project;

2. commercial, proprietary, or other separately negotiated licenses; and

3. future versions or successor licensing arrangements for OpticForge.

This license grant does not prevent you from exercising any rights you otherwise have in your contribution.

## 3. Commercial Relicensing

You understand that OpticForge may be offered under separate commercial licenses.

You expressly grant the OpticForge project maintainer permission to include your contribution in commercially licensed versions of OpticForge without requiring additional permission from you and without requiring payment of royalties or other compensation.

This permission applies only to rights you have authority to grant.

## 4. Patent License

To the extent that your contribution is covered by patent claims that you own or control and that would necessarily be infringed by use of your contribution as submitted, you grant the OpticForge project maintainer and recipients of OpticForge a perpetual, worldwide, non-exclusive, royalty-free patent license to make, use, sell, offer for sale, import, and otherwise exercise those patent claims as necessary to use your contribution.

No patent rights are granted beyond claims necessarily infringed by your contribution itself or by its combination with OpticForge as submitted by you.

## 5. Authority to Contribute

By submitting a contribution, you represent that you have the legal authority to grant the rights described in this agreement.

If your contribution was created as part of your employment, under contract, or on behalf of another person or organization, you are responsible for ensuring that you have permission to submit the contribution under these terms.

Do not submit code owned by your employer, client, university, or another third party unless you have authority to do so.

## 6. Original and Third-Party Material

You represent that, to the best of your knowledge, your contribution is either:

* your original work; or
* material that you have the legal right to submit under terms compatible with this agreement.

If your contribution includes third-party code, data, documentation, algorithms expressed in copyrighted source code, or other copyrighted material, you must clearly identify that material and its applicable license.

Do not submit code copied from projects whose licenses are incompatible with OpticForge's licensing model.

In particular, code subject to licensing requirements that would prevent OpticForge from being separately commercially licensed should not be submitted without prior approval from the project maintainer.

## 7. No Obligation to Accept Contributions

Submission of a contribution does not require the OpticForge project maintainer to accept, merge, publish, maintain, or distribute that contribution.

Accepted contributions may subsequently be modified, reorganized, replaced, or removed as the project evolves.

## 8. No Warranty

Unless separately agreed in writing, contributions are provided without warranties or conditions of any kind.

You are not responsible for future modifications made to your contribution by the OpticForge project or other contributors.

## 9. Attribution

The project may preserve authorship information through Git history, commit metadata, release notes, acknowledgements, or similar mechanisms.

This agreement does not require that individual source files retain contributor-specific copyright notices unless separately required by applicable law or an authorized third-party license.

## 10. Future Public Licensing

The ability of the OpticForge project maintainer to relicense contributions is intended to allow the project to evolve its licensing model while remaining able to distribute the complete codebase consistently.

For example, the project may in the future adopt a different source-available license, a more permissive public license, a standardized small-business or artisan license, or other licensing arrangements.

The rights granted under this agreement allow accepted contributions to continue to be distributed as part of OpticForge under those arrangements.

# Contributor Certification

By submitting a contribution intended for incorporation into OpticForge, you certify that:

1. you have the right to submit the contribution;

2. you understand that your contribution may be publicly distributed under the OpticForge public license;

3. you understand that your contribution may also be distributed as part of commercially licensed versions of OpticForge;

4. you grant the rights described in this CONTRIBUTING.md; and

5. you understand that you retain copyright ownership of your contribution while granting these rights to the OpticForge project maintainer.

If you cannot agree to these terms, please do not submit the contribution for incorporation into the OpticForge codebase.

Questions concerning contribution or licensing terms may be raised through the project's GitHub repository.
