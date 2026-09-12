#pragma once

#include "TelescopePrimitives.h"

namespace opticforge::telescope
{
	class TelescopeProject
	{
	public:
		const std::vector<PrimitiveRecord>& primitives() const
		{
			return m_primitives; 
		}
		void clearProject() {
			m_primitives.clear(); 
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
		PrimitiveId addPrimitive(TelescopePrimitive primitive) 
		{
			PrimitiveRecord newRecord{ ++m_nextPrimitive, primitive };
			m_primitives.push_back(newRecord);
			return newRecord.id;

		}

	protected:
		std::vector<PrimitiveRecord> m_primitives;
		PrimitiveId m_nextPrimitive = 1; 

	};
}