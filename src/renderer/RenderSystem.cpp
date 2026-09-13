#include "RenderSystem.h"
#include "LensMeshGenerator.h"
#include "MirrorMeshGenerator.h"
#include <iostream>

namespace opticforge {
	RenderSystem::RenderSystem(float aspect) :
		m_aspectRatio(aspect),
		m_meshShader("shaders/mesh.vert", "shaders/mesh.frag"),
		m_planeShader("shaders/plane.vert", "shaders/plane.frag"),
		m_planeCircShader("shaders/planeCirc.vert", "shaders/planeCirc.frag")
	{
		GLuint emptyVao = 0;

		glGenVertexArrays(1, &emptyVao);
		glBindVertexArray(emptyVao);
		m_emptyVao = emptyVao;
	}
	RenderObject& RenderSystem::getOrCreateMirrorRenderObject(
		telescope::PrimitiveId id,
		const telescope::Mirror& mirror)
	{
		uint64_t geometryRevision = 1;
		auto it =
			m_renderObjects.find(id);

		if (it == m_renderObjects.end())
		{
			MeshData meshData =
				MirrorMeshGenerator::generate(
					mirror.surface,
					mirror.thickness);

			RenderObject renderObject;

			renderObject.mesh =
				createMesh(meshData);

		

			renderObject.geometryRevision =	geometryRevision;

			it =
				m_renderObjects.emplace(
					id,
					std::move(renderObject))
				.first;

			return it->second;
		}

		if (it->second.geometryRevision !=
			geometryRevision)
		{
			MeshData meshData =
				MirrorMeshGenerator::generate(
					mirror.surface,
					mirror.thickness);

			it->second.mesh =
				createMesh(meshData);

			it->second.geometryRevision =
				geometryRevision;
		}

		return it->second;
	}
	void RenderSystem::drawProject(const telescope::TelescopeProject& project,
		const Camera& camera)
	{

		//First render the project's observation plane
		auto& obsPlane = project.getObservationPlane();
		const glm::mat4 model = obsPlane.transform.matrix(); 
		const glm::vec3 center = glm::vec3(model * glm::vec4(0, 0, 0, 1)); 
		
		const glm::dmat3 normalMatrix = 
			glm::transpose(glm::inverse(glm::dmat3(model)));

		const glm::dvec3 obsNormal = glm::normalize(
			normalMatrix * glm::dvec3(0.0, 0.0, 1.0));
	
		glm::vec2 obsSize {obsPlane.displaySize.x / 1000.0, obsPlane.displaySize.y / 1000.0 };

		m_planeShader.bind();

		m_planeShader.setMat4("uView", camera.viewMatrix());
		m_planeShader.setMat4("uProjection", camera.projectionMatrix(m_aspectRatio));

			
		m_planeShader.setVec3("uCenter", glm::vec3( center * (1.0f/1000.0f)));
		m_planeShader.setVec3("uNormal", obsNormal);
		m_planeShader.setVec2("uSize", obsSize);

		m_planeShader.setVec3("uColor", { 1.0, 0.0, 0.0 });
		m_planeShader.setFloat("uOpacity", 1.0);

		glBindVertexArray(m_emptyVao);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		ShaderProgram::unbind();


		//Then render all primitives
		auto drawList = project.primitives();
		for (const auto& record : project.primitives())
		{
			std::visit(
				[&](const auto& primitive)
				{
					drawPrimitive(
						record.id,
						primitive,
						camera);
				},
				record.primitive);
		}

		//Finally, since it's treated as transparent, render the launch pupil
		auto& launchPupil = project.getLaunchPupil();
		glm::vec3 pupilCenter = launchPupil.transform.position(); 
		glm::vec3 pupilNorm = glm::normalize(launchPupil.transform.rotation() * launchPupil.transform.forward());
		float pupilSize = 100.0;

		if (const auto* circle =
			std::get_if<opticforge::optics::CircularAperture>(
				&launchPupil.aperture.geometry()))
		{
			const float radius =
				static_cast<float>(circle->radius);

			pupilSize = radius; 
		}

		m_planeCircShader.bind(); 

		m_planeCircShader.setMat4("uView", camera.viewMatrix());
		m_planeCircShader.setMat4("uProjection", camera.projectionMatrix(m_aspectRatio));

		m_planeCircShader.setVec3("uCenter", pupilCenter * (1.0f/1000.0f));
		m_planeCircShader.setVec3("uNormal", pupilNorm);
		m_planeCircShader.setFloat("uRadius", pupilSize * (1.0f/1000.0f));

		m_planeCircShader.setVec3("uColor", glm::vec3{ 0,0,1 });
		m_planeCircShader.setFloat("uOpacity", 0.75);

		glBindVertexArray(m_emptyVao);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		ShaderProgram::unbind();


	}

	void RenderSystem::drawPrimitive(telescope::PrimitiveId id, 
									 const telescope::Lens & lens, 
									 const Camera& camera)
	{
		//std::cout << "Drawing a lens..." << std::endl;
		auto& renderObj = getOrCreateLensRenderObject(id, lens);
		//Load lens shader program
		m_meshShader.bind(); 

		primitiveSetup(m_meshShader, lens.transform, camera);
		m_meshShader.setVec3("uBaseColor", { 0.25, 1.0, 1.0 });
		m_meshShader.setFloat("uOpacity", 0.5);
		m_meshShader.setFloat("uNormalExaggeration", 1.0);

		m_meshShader.setFloat("uAmbientStrength", 0.25);
		m_meshShader.setFloat("uDiffuseStrength", 0.8);
		m_meshShader.setFloat("uSpecularStrength", 0.5);
		m_meshShader.setFloat("uShininess", 0.9); 
		m_meshShader.setVec3("uLightDirection", { 0.4, 0.7, -0.6 });
		m_meshShader.setVec3("uLightColor", { 1.0, 1.0, 1.0 }); 

		m_meshShader.setFloat("uFresnelStrength", 0.9);
		m_meshShader.setFloat("uFresnelPower", 1.0); 



		//Bind lens VAO
		renderObj.mesh.bind(); 
		

		// Render
		glDrawElements(
			GL_TRIANGLES,
			renderObj.mesh.indexCount(),
			GL_UNSIGNED_INT,
			nullptr);
		
		//Unbind 
		renderObj.mesh.unbind(); 
		ShaderProgram::unbind(); 
	}
	void RenderSystem::drawPrimitive(telescope::PrimitiveId id,
									 const telescope::Mirror & mirror,
									 const Camera& camera)
	{
		//std::cout << "Drawing a lens..." << std::endl;
		auto& renderObj = getOrCreateMirrorRenderObject(id, mirror);
		//Load lens shader program
		m_meshShader.bind();

		primitiveSetup(m_meshShader, mirror.transform, camera);
		m_meshShader.setVec3("uBaseColor", { 0.45, 0.48, 0.55});
		m_meshShader.setFloat("uOpacity", 1.0);

		m_meshShader.setFloat("uAmbientStrength", 0.08);
		m_meshShader.setFloat("uDiffuseStrength", 0.55);
		m_meshShader.setFloat("uSpecularStrength", 0.85);
		m_meshShader.setFloat("uShininess", 64.0);
		m_meshShader.setVec3("uLightDirection", { 0.0, 0.0, -1.0 });
		m_meshShader.setVec3("uLightColor", { 1.0, 1.0, 1.0 });
		m_meshShader.setFloat("uNormalExaggeration", 6.0);

		m_meshShader.setFloat("uFresnelStrength", 0.15);
		m_meshShader.setFloat("uFresnelPower", 4.0);



		//Bind lens VAO
		renderObj.mesh.bind();


		// Render
		glDrawElements(
			GL_TRIANGLES,
			renderObj.mesh.indexCount(),
			GL_UNSIGNED_INT,
			nullptr);

		//Unbind 
		renderObj.mesh.unbind();
		ShaderProgram::unbind();
	}
	void RenderSystem::drawPrimitive(telescope::PrimitiveId id,
									 const telescope::Detector & detector,
									 const Camera& camera)
	{
		//Load lens shader program
		m_meshShader.bind();
		primitiveSetup(m_meshShader, detector.transform, camera);


		//Bind lens VAO
		// Render
		//Unbind 
		ShaderProgram::unbind();
	}
	void RenderSystem::primitiveSetup(ShaderProgram & shader, const optics::Transform& transform, const Camera& camera)
	{
		constexpr double MM_TO_M = 0.001;

		optics::Transform renderTransform =
			transform;

		renderTransform.setPosition(
			renderTransform.position() *
			MM_TO_M);

		glm::dmat4 model =
			renderTransform.matrix() *
			glm::scale(
				glm::dmat4(1.0),
				glm::dvec3(MM_TO_M));
	
		shader.setMat4(
			"uView",
			camera.viewMatrix());

		shader.setMat4("uModel",
			glm::mat4(model));

		shader.setMat4(
			"uProjection",
			camera.projectionMatrix(m_aspectRatio));

	}
	RenderObject& RenderSystem::getOrCreateLensRenderObject(
		telescope::PrimitiveId id,
		const telescope::Lens& lens)
	{
		auto it = m_renderObjects.find(id);

		if (it == m_renderObjects.end())
		{
			MeshData data =
				LensMeshGenerator::generate(lens.frontSurface, lens.rearSurface, lens.centerThickness);

			RenderObject object;
			object.mesh = createMesh(data);
			//object.geometryRevision = lens.geometryRevision;

			it = m_renderObjects.emplace(
				id,
				std::move(object)).first;
		}

		return it->second;
	}
	MeshGpu RenderSystem::createMesh(const MeshData& meshData)
	{
		
		MeshGpu mesh;

		glGenVertexArrays(
			1,
			&mesh.m_vao);

		glGenBuffers(
			1,
			&mesh.m_vbo);

		glGenBuffers(
			1,
			&mesh.m_ebo);

		glBindVertexArray(
			mesh.m_vao);

		glBindBuffer(
			GL_ARRAY_BUFFER,
			mesh.m_vbo);

		glBufferData(
			GL_ARRAY_BUFFER,
			meshData.vertices.size() *
			sizeof(MeshVertex),
			meshData.vertices.data(),
			GL_STATIC_DRAW);

		glBindBuffer(
			GL_ELEMENT_ARRAY_BUFFER,
			mesh.m_ebo);

		glBufferData(
			GL_ELEMENT_ARRAY_BUFFER,
			meshData.indices.size() *
			sizeof(std::uint32_t),
			meshData.indices.data(),
			GL_STATIC_DRAW);

		glVertexAttribPointer(
			0,
			3,
			GL_FLOAT,
			GL_FALSE,
			sizeof(MeshVertex),
			reinterpret_cast<void*>(
				offsetof(
					MeshVertex,
					position)));

		glEnableVertexAttribArray(0);

		glVertexAttribPointer(
			1,
			3,
			GL_FLOAT,
			GL_FALSE,
			sizeof(MeshVertex),
			reinterpret_cast<void*>(
				offsetof(
					MeshVertex,
					normal)));

		glEnableVertexAttribArray(1);

		glBindVertexArray(0);

		mesh.m_indexCount =
			static_cast<GLsizei>(
				meshData.indices.size());

		return mesh;
	}
}