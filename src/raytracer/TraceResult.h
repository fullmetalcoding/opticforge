#pragma once

#include "RayPath.h"
#include <vector>

namespace opticforge::raytracer {
	struct TraceResult {
		std::vector<RayPath> paths; 
	};
}