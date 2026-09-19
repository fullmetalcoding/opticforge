#pragma once
#include "telescope/TelescopeProject.h"
#include "ShaderProgram.h"
#include "camera.h"
#include "MeshGpu.h"
#include "LensMeshGenerator.h"
#include "PickingFrameBuffer.h"
#include "MirrorMeshGenerator.h"

#include <optional>

namespace opticforge
{
	struct RenderObject
	{
		MeshGpu mesh;
		std::uint64_t geometryRevision = 0;
	};
	class RenderSystem
	{
	public:
		void clearProjectCache()
		{
			m_renderObjects.clear();
			m_hoveredPrimitive.reset();
		}
		void setAspectRatio(float aspect)
		{
			m_aspectRatio = aspect;
		}
		RenderObject& getOrCreateMirrorRenderObject(
			telescope::PrimitiveId id,
			const telescope::Mirror& mirror
		);
		RenderObject& getOrCreateLensRenderObject(
			telescope::PrimitiveId id,
			const telescope::Lens& lens);
		RenderSystem(float aspect); 
		void drawProject(const telescope::TelescopeProject & project,
			const Camera & camera);

		void drawPrimitive(telescope::PrimitiveId id, const telescope::Lens & lens, const Camera& camera); 
		void drawPrimitive(telescope::PrimitiveId id, const telescope::Mirror & mirror, const Camera& camera);
		void drawPrimitive(telescope::PrimitiveId id, const telescope::Detector & detector, const Camera& camera);

		std::optional<telescope::PrimitiveId> pickPrimitive(
			const telescope::TelescopeProject& project,
			const Camera& camera,
			int pixelX,
			int pixelYFromTop,
			int framebufferWidth,
			int framebufferHeight);

		void setHoveredPrimitive(
			std::optional<telescope::PrimitiveId> id)
		{
			m_hoveredPrimitive = id;
		}

		[[nodiscard]]
		std::optional<telescope::PrimitiveId>
			hoveredPrimitive() const noexcept
		{
			return m_hoveredPrimitive;
		}

		void removePrimitiveFromCache(
			telescope::PrimitiveId id)
		{
			m_renderObjects.erase(id);

			if (
				m_hoveredPrimitive &&
				*m_hoveredPrimitive == id)
			{
				m_hoveredPrimitive.reset();
			}
		}

	protected:
		void drawPrimitiveForPicking(
			telescope::PrimitiveId id,
			const telescope::Lens& lens,
			const Camera& camera);

		void drawPrimitiveForPicking(
			telescope::PrimitiveId id,
			const telescope::Mirror& mirror,
			const Camera& camera);

		void drawPrimitiveForPicking(
			telescope::PrimitiveId id,
			const telescope::Detector& detector,
			const Camera& camera);

		void setPickingPrimitiveId(
			telescope::PrimitiveId id);

		[[nodiscard]]
		bool isHovered(
			telescope::PrimitiveId id) const noexcept
		{
			return
				m_hoveredPrimitive &&
				*m_hoveredPrimitive == id;
		}
		void primitiveSetup(ShaderProgram & shader, const optics::Transform& transform, const Camera& camera);
		MeshGpu createMesh(const MeshData& meshData);

		ShaderProgram m_meshShader;
		ShaderProgram m_planeShader;
		ShaderProgram m_planeCircShader;
		ShaderProgram m_pickingShader;

		GLuint m_emptyVao = 0; 

		float m_aspectRatio; 
		std::unordered_map<
			telescope::PrimitiveId,
			RenderObject> m_renderObjects;
		
		PickingFramebuffer m_pickingFramebuffer;

		std::optional<telescope::PrimitiveId>
			m_hoveredPrimitive;
	};


}

