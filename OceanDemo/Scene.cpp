#include "Scene.h"
#include "GraphicsContext.h"
#include "GameObject.h"
#include "CameraComponent.h"
#include "OceanComponent.h"
#include "DebugUtil.h"

#include <algorithm>
#include <glm\glm.hpp>
#include <glm\gtc\type_ptr.hpp>
namespace acqua
{
	Scene::Scene( void ) :
		graphicsContext(NULL)
		, renderTime(0.0f)
		, reflectionBuffer(0)
		, refractionBuffer(0)
		, foamBuffer(0)
	{

	}

	Scene::~Scene( void )
	{
		// Delete game objects.
		std::list<GameObject*>::iterator it;
		for(it = gameObjects.begin(); it != gameObjects.end(); ++it)
		{
			delete (*it);
		}
	}

	bool Scene::Init(GraphicsContext* graphics_context)
	{
		// I do not assert on the graphics_context pointer, because it might be null in some situations.
		// Imagine needing the Scene class in a command prompt tool for example.

		if(graphics_context != NULL)
		{
			graphicsContext = graphics_context;

			return true;
		}

		return false;
	}

	GameObject& Scene::CreateGameObject()
	{
		// TODO: All these new one day might be replaced by a memory manager.
		GameObject* g = new GameObject();
		ASSERT(g != NULL, "Out of memory.");
		
		g->owner = this;

		gameObjects.push_back(g);

		return *g;
	}

	void Scene::FixedUpdate( float fixed_delta_time )
	{
		std::list<GameObject*>::iterator it;
		for(it = gameObjects.begin(); it != gameObjects.end(); ++it)
		{
			(*it)->FixedUpdate(fixed_delta_time);
		}
	}

	void Scene::Update( float delta_time )
	{
		std::list<GameObject*>::iterator it;
		for(it = gameObjects.begin(); it != gameObjects.end(); ++it)
		{
			(*it)->Update(delta_time);
		}
	}

	extern u32 terrain_normal_texture_handle;
	extern u32 terrain_diffuse_texture_handle;
	extern u32 normal_texture_handle;
	extern u32 foam_texture_handle;
	extern OceanSettings gOceanSettings;
	
	void Scene::Draw( float delta_time )
	{
		glEnable(GL_CLIP_DISTANCE0);
		renderTime += delta_time;

		{
			std::list<GameObject*>::iterator it;
			for(it = gameObjects.begin(); it != gameObjects.end(); ++it)
			{
				(*it)->Draw(delta_time);
			}
		}

		//TODO: Draw geometryRenderers possibly using a renderer. For now just use the graphics context.
		
		if(graphicsContext == NULL)
			return;

		graphicsContext->SetColourMask(true, true, true, true);
		graphicsContext->SetDepthMask(true);
		graphicsContext->SetClearColour(glm::vec4(0.0f));
		graphicsContext->SetClearDepth(1.0f);

		graphicsContext->Clear();

		{
			for(CameraListIterator camera = cameras.begin(); camera != cameras.end(); ++camera)
			{
				// Draw Camera's skybox.
				(*camera)->DrawSkybox(graphicsContext);

				graphicsContext->UseTexture(0, terrain_normal_texture_handle);
				graphicsContext->UseTexture(4, terrain_diffuse_texture_handle);

				std::list<const GeometryRenderer*>::iterator it;
				
				// Reflection pass.
				glEnable(GL_CLIP_DISTANCE0); // TODO: DO THIS IN GRAPHICS CONTEXT!
				graphicsContext->SetRenderBuffer(reflectionBuffer);
				graphicsContext->Clear();
				
				glm::mat4 reflection_matrix = glm::mat4(1.0f);
				reflection_matrix[1][1] = -1;
				
				for(it = geometryRenderers.begin(); it != geometryRenderers.end(); ++it)
				{
					//Don't render the ocean.
					if((*it)->GetGameObject().HasComponent<OceanComponent>())
						continue;

					graphicsContext->UseShaderProgram((*it)->GetShaderProgram());

					// Set General Uniforms.
					SetGeneralUniforms(*camera, renderTime);

					// Set transform matrix.
					const glm::mat4& model_matrix = (*it)->GetGameObject().GetTransform().GetMatrix();

					ShaderProgram& shader_prog = graphicsContext->GetShaderProgram(graphicsContext->GetCurrentShaderProgram());
					shader_prog.SetUniformFromArray("modelMatrix", (void*)glm::value_ptr(model_matrix), 1, false);
					shader_prog.SetUniformFromArray("reflectionMatrix", (void*)glm::value_ptr(reflection_matrix), 1, false);
					shader_prog.SetUniform("refractionPass", 0.0f);

					// Draw Geometry.
					const Geometry* g = (*it)->geometry.get();
					graphicsContext->DrawGeometry(g);
				}
				glDisable(GL_CLIP_DISTANCE0);
				graphicsContext->SetRenderBuffer(0);
				reflection_matrix = glm::mat4(1.0f);

				// Refraction pass.
				glEnable(GL_CLIP_DISTANCE1); // TODO: DO THIS IN GRAPHICS CONTEXT!
				graphicsContext->SetRenderBuffer(refractionBuffer);
				graphicsContext->SetClearColour(glm::vec4(1.0f));
				graphicsContext->Clear();

				for(it = geometryRenderers.begin(); it != geometryRenderers.end(); ++it)
				{
					//Don't render the ocean.
					if((*it)->GetGameObject().HasComponent<OceanComponent>())
						continue;

					graphicsContext->UseShaderProgram((*it)->GetShaderProgram());

					// Set General Uniforms.
					SetGeneralUniforms(*camera, renderTime);

					// Set transform matrix.
					const glm::mat4& model_matrix = (*it)->GetGameObject().GetTransform().GetMatrix();

					ShaderProgram& shader_prog = graphicsContext->GetShaderProgram(graphicsContext->GetCurrentShaderProgram());
					shader_prog.SetUniformFromArray("modelMatrix", (void*)glm::value_ptr(model_matrix), 1, false);
					shader_prog.SetUniformFromArray("reflectionMatrix", (void*)glm::value_ptr(reflection_matrix), 1, false);
					shader_prog.SetUniform("refractionPass", 1.0f);

					// Draw Geometry.
					const Geometry* g = (*it)->geometry.get();
					graphicsContext->DrawGeometry(g);
				}
				glDisable(GL_CLIP_DISTANCE1);
				graphicsContext->SetRenderBuffer(0);
				reflection_matrix = glm::mat4(1.0f);

				// Render geometry (normal pass - terrain -).
				graphicsContext->UseTexture(2, graphicsContext->GetRenderbufferTexture(reflectionBuffer, 0));
				graphicsContext->UseTexture(3, graphicsContext->GetRenderbufferTexture(refractionBuffer, 0));

				const GeometryRenderer* ocean_renderer = NULL;

				for(it = geometryRenderers.begin(); it != geometryRenderers.end(); ++it)
				{
					//Don't render the ocean.
					if((*it)->GetGameObject().HasComponent<OceanComponent>())
					{
						ocean_renderer = (*it);
						continue;
					}
					// TODO: 
					// Even better:
					// - Perform frustum culling.
					// - Create PVS from Spatial structure.
					// - Order by material and instance.
					// - Create render queue in the HIGH LEVEL rendering system.
					// - It will then render.
					// The above algorithm should be performed into a PreRender function.

					// TODO: Set Material Properties.
					graphicsContext->UseShaderProgram((*it)->GetShaderProgram());

					// Set General Uniforms.
					SetGeneralUniforms(*camera, renderTime);

					// Set transform matrix.
					const glm::mat4& model_matrix = (*it)->GetGameObject().GetTransform().GetMatrix();

					ShaderProgram& shader_prog = graphicsContext->GetShaderProgram(graphicsContext->GetCurrentShaderProgram());
					shader_prog.SetUniformFromArray("modelMatrix", (void*)glm::value_ptr(model_matrix), 1, false);
					shader_prog.SetUniformFromArray("reflectionMatrix", (void*)glm::value_ptr(reflection_matrix), 1, false);
					shader_prog.SetUniform("refractionPass", 0.0f);

					// Draw Geometry.
					const Geometry* g = (*it)->geometry.get();
					graphicsContext->DrawGeometry(g);
				}

				// Render the ocean .
				graphicsContext->UseTexture(0, normal_texture_handle);
				graphicsContext->UseTexture(5, foam_texture_handle);
				if(ocean_renderer != NULL)
				{
					graphicsContext->UseShaderProgram(ocean_renderer->GetShaderProgram());

					// Set General Uniforms.
					SetGeneralUniforms(*camera, renderTime);

					// Set transform matrix.
					const glm::mat4& model_matrix = ocean_renderer->GetGameObject().GetTransform().GetMatrix();

					ShaderProgram& shader_prog = graphicsContext->GetShaderProgram(graphicsContext->GetCurrentShaderProgram());
					shader_prog.SetUniformFromArray("modelMatrix", (void*)glm::value_ptr(model_matrix), 1, false);

					// Render foam buffer first.
					graphicsContext->SetRenderBuffer(foamBuffer);
					graphicsContext->SetClearColour(glm::vec4(0.0f));
					graphicsContext->Clear();
					glViewport(0, 0, graphicsContext->GetScreenWidth() / 2 , graphicsContext->GetScreenHeight() / 2);
					shader_prog.SetUniform("renderFoamIntensity", 1.0f);
					// Draw Geometry.
					const Geometry* g = ocean_renderer->geometry.get();
					graphicsContext->DrawGeometry(g, 3);

					// Render normally.
					graphicsContext->SetRenderBuffer(0);
					shader_prog.SetUniform("renderFoamIntensity", 0.0f);

					graphicsContext->UseTexture(4, graphicsContext->GetRenderbufferTexture(foamBuffer, 0));
					// Draw Geometry.

					glViewport(0, 0, graphicsContext->GetScreenWidth(), graphicsContext->GetScreenHeight());
					graphicsContext->DrawGeometry(g, 3);

				}
			}
		}
	}
	
	void Scene::RegisterGeometryRenderer( GeometryRenderer* geometry_renderer )
	{
		// If the element has already been added or the pointer is null, skip it.
		if(geometry_renderer == NULL)
			return;

		if(std::find(geometryRenderers.begin(), geometryRenderers.end(), geometry_renderer) != geometryRenderers.end())
			return;

		geometryRenderers.push_back(geometry_renderer);
	}

	void Scene::UnregisterGeometryRenderer( GeometryRenderer* geometry_renderer )
	{
		std::list<const GeometryRenderer*>::iterator it = std::find(geometryRenderers.begin(), geometryRenderers.end(), geometry_renderer);

		if(it == geometryRenderers.end())
			return; // Nothing to do here.

		geometryRenderers.erase(it);
	}

	void Scene::RegisterCamera(CameraComponent* camera)
	{
		// If the element has already been added or the pointer is null, skip it.
		if(camera == NULL)
			return;

		if(std::find(cameras.begin(), cameras.end(), camera) != cameras.end())
			return;

		cameras.push_back(camera);
	}

	void Scene::UnregisterCamera(CameraComponent* camera)
	{
		CameraListIterator it = std::find(cameras.begin(), cameras.end(), camera);

		if(it == cameras.end())
			return; // Nothing to do here.

		cameras.erase(it);
	}

	void Scene::ResolutionChanged( int width, int height )
	{
		CameraComponent::CameraViewport viewport = {0, 0, width, height};

		CameraListIterator camera;
		for(camera = cameras.begin(); camera != cameras.end(); ++camera)
		{
			(*camera)->SetViewport(viewport);
		}

		// Recreate Render Buffers.
		if(reflectionBuffer != 0) graphicsContext->DestroyRenderBuffer(reflectionBuffer);
		if(refractionBuffer != 0) graphicsContext->DestroyRenderBuffer(refractionBuffer);
		if(foamBuffer != 0) graphicsContext->DestroyRenderBuffer(foamBuffer);

		reflectionBuffer = graphicsContext->CreateRenderbuffer(width, height, TextureFormats::RGBA, true, 1, 0);
		refractionBuffer = graphicsContext->CreateRenderbuffer(width, height, TextureFormats::RGBA, true, 1, 0);
		foamBuffer = graphicsContext->CreateRenderbuffer(width / 2, height / 2, TextureFormats::RGBA, true, 1, 0);
	}

	void Scene::SetGeneralUniforms(const CameraComponent* camera, float time)
	{
		ASSERT(camera !=  NULL, "Camera cannot be NULL. Does not make sense.")
		if(graphicsContext == NULL)
			return;

		// Get current shader.
		ShaderProgram& shader_prog = graphicsContext->GetShaderProgram(graphicsContext->GetCurrentShaderProgram());

		// Set common uniforms.
		shader_prog.SetUniformFromArray("viewProjectionMatrix", (void*)glm::value_ptr(camera->GetViewProjectionMatrix()), 1, false);
		shader_prog.SetUniformFromArray("viewMatrix", (void*)glm::value_ptr(camera->GetViewMatrix()), 1, false);
		shader_prog.SetUniformFromArray("cameraPosition", (void*)glm::value_ptr(camera->GetGameObject().GetTransform().GetPosition()), 1, false);
		shader_prog.SetUniform("time", time);

		// Set Wind
		float W = gOceanSettings.W * 0.0174532925f; // Convert to radians.
		glm::vec2 Wv(cos(W), -sin(W));
		shader_prog.SetUniformFromArray("windDir", (void*)glm::value_ptr(Wv), 1, false);
	}
}
