// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include "RayPath.h"
#include <vector>

namespace opticforge::raytracer {
	struct TraceResult {
		std::vector<RayPath> paths; 
	};
}