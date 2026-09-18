#pragma once
#include "telescope/TelescopeProject.h"
#include "ShaderProgram.h"
#include "camera.h"
#include "MeshGpu.h"
#include "LensMeshGenerator.h"

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

	protected:
		void primitiveSetup(ShaderProgram & shader, const optics::Transform& transform, const Camera& camera);
		MeshGpu createMesh(const MeshData& meshData);

		ShaderProgram m_meshShader;
		ShaderProgram m_planeShader;
		ShaderProgram m_planeCircShader;

		GLuint m_emptyVao = 0; 

		float m_aspectRatio; 
		std::unordered_map<
			telescope::PrimitiveId,
			RenderObject> m_renderObjects;
	};


}

