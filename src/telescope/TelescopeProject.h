// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include "TelescopePrimitives.h"

#include <algorithm>
#include <vector>

namespace opticforge::project
{
	class ProjectSerializer;
}

namespace opticforge::telescope
{
	class TelescopeProject
	{
	public:

		const std::vector<PrimitiveRecord>& primitives() const
		{
			return m_primitives;
		}
		void clearProject();

		TelescopeProject() {
			clearProject();
		}

		TelescopePrimitive*
			findPrimitive(PrimitiveId id)
		{
			for (auto& record : m_primitives)
			{
				if (record.id == id)
					return &record.primitive;
			}

			return nullptr;
		}

		const TelescopePrimitive*
			findPrimitive(
				PrimitiveId id) const
		{
			for (const auto& record : m_primitives)
			{
				if (record.id == id)
					return &record.primitive;
			}

			return nullptr;
		}
		bool removePrimitive(
			PrimitiveId id)
		{
			auto it = std::find_if(
				m_primitives.begin(),
				m_primitives.end(),
				[id](const PrimitiveRecord& entry)
				{
					return entry.id == id;
				});

			if (it == m_primitives.end())
				return false;

			m_primitives.erase(it);

			return true;
		}
		std::string getNameForId(PrimitiveId id) {
			for (const auto& record : m_primitives)
			{
				if (record.id == id)
					return record.name;
			}
			return ""; 
		}
		PrimitiveId addPrimitive(TelescopePrimitive primitive, const std::string name = "" )
		{
			PrimitiveRecord newRecord{ ++m_nextPrimitive, std::move(primitive), name};
			m_primitives.push_back(std::move( newRecord));
			return m_primitives.back().id;

		}
		const LaunchPupil& getLaunchPupil() const {
			return m_launchPupil;
		}
		// Call TraceController::invalidate() after changing the pupil.
		void setLaunchPupil(const LaunchPupil& pupil) {
			m_launchPupil = pupil;
		}
		const ObservationPlane& getObservationPlane() const {
			return m_observationPlane;
		}

		// Call TraceController::invalidate() after changing the observation plane.
		void setObservationPlane(const ObservationPlane& obsPlane) {
			m_observationPlane = obsPlane;
		}

	protected:
		std::vector<PrimitiveRecord> m_primitives;
		// ID 0 is reserved for the atomic observation plane.
		// addPrimitive() pre-increments this counter, so the first
		// ordinary primitive gets ID 1.
		PrimitiveId m_nextPrimitive = 0;

		LaunchPupil m_launchPupil;
		ObservationPlane m_observationPlane;
	private:
		friend class opticforge::project::ProjectSerializer;

	};
}
