#include "CameraComponent.h"
#include "GameObject.h"
#include "DebugUtil.h"

#include "GraphicsContext.h"
#include "Geometry.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm\glm.hpp>
#include <glm\gtc\type_ptr.hpp>
#include "BasicIO.h"

namespace acqua
{
	bool	CameraComponent::skyboxShaderLoaded = false;
	u32		CameraComponent::skyboxShader = 0;

	CameraComponent::CameraComponent(void) :
		Component(CT_CAMERA)
		, fieldOfView(60.0f)
		, dirtyFlag(false)
		, nearPlane(1.0f)
		, farPlane(10000.0f)
		, hasSkybox(false)
		, skyboxCubemap(0)
		, skyboxGeometry(NULL)
	{
	}


	CameraComponent::~CameraComponent(void)
	{
		if(owner != NULL)
			owner->GetScene().UnregisterCamera(this); // Play nicely and let Scene know you are out of the way.
	}

	bool CameraComponent::Init(GameObject* o)
	{
		Component::Init(o);

		componentType = CT_CAMERA;

		owner->GetScene().RegisterCamera(this);
		
		RecomputeViewMatrix();
		RecomputeProjectionMatrix();

		return true;
	}

	void CameraComponent::FixedUpdate( float fixed_delta_time )
	{
	}

	void CameraComponent::Update( float delta_time )
	{
		ASSERT(owner != NULL, "Owner cannot be null.");
		
		RecomputeViewMatrix();

		if (dirtyFlag)
		{
			RecomputeProjectionMatrix();
		}
	}

	void CameraComponent::Draw( float delta_time )
	{
	}

	const glm::mat4& CameraComponent::GetViewProjectionMatrix() const
	{
		return viewProjectionMatrix;
	}

	void CameraComponent::RecomputeViewMatrix()
	{
		Transform transform = owner->GetTransform();
		
		const glm::vec3 position = transform.GetPosition();
		glm::vec3 at = position + transform.GetForward();
		const glm::vec3& up = transform.GetUp();

		viewMatrix = glm::lookAt(position, at, up);
		viewProjectionMatrix = projectionMatrix * viewMatrix;
	}

	void CameraComponent::RecomputeProjectionMatrix()
	{
		float aspect = viewport.width / static_cast<float>(viewport.height); // TODO: Make this better (ie. cache things etc.).
		projectionMatrix = glm::perspective(fieldOfView, aspect, nearPlane, farPlane );
		viewProjectionMatrix = projectionMatrix * viewMatrix;
	}

	void CameraComponent::GenerateSkybox( u32 cubemap_handle )
	{
		if(!hasSkybox)
		{
			LoadSkyboxShader(owner->GetScene().GetGraphicsContext());
			// Generate skybox geometry.
			VertexLayoutAttrib vertex_attribs[] = { {3, 0} /* Position */ };
			const u32 num_attribs = sizeof(vertex_attribs) / sizeof(VertexLayoutAttrib);

			float vertices[] = 
			{
				-10.0f,  10.0f, -10.0f,
				-10.0f, -10.0f, -10.0f,
				10.0f, -10.0f, -10.0f,
				10.0f, -10.0f, -10.0f,
				10.0f,  10.0f, -10.0f,
				-10.0f,  10.0f, -10.0f,

				-10.0f, -10.0f,  10.0f,
				-10.0f, -10.0f, -10.0f,
				-10.0f,  10.0f, -10.0f,
				-10.0f,  10.0f, -10.0f,
				-10.0f,  10.0f,  10.0f,
				-10.0f, -10.0f,  10.0f,

				10.0f, -10.0f, -10.0f,
				10.0f, -10.0f,  10.0f,
				10.0f,  10.0f,  10.0f,
				10.0f,  10.0f,  10.0f,
				10.0f,  10.0f, -10.0f,
				10.0f, -10.0f, -10.0f,

				-10.0f, -10.0f,  10.0f,
				-10.0f,  10.0f,  10.0f,
				10.0f,  10.0f,  10.0f,
				10.0f,  10.0f,  10.0f,
				10.0f, -10.0f,  10.0f,
				-10.0f, -10.0f,  10.0f,

				-10.0f,  10.0f, -10.0f,
				10.0f,  10.0f, -10.0f,
				10.0f,  10.0f,  10.0f,
				10.0f,  10.0f,  10.0f,
				-10.0f,  10.0f,  10.0f,
				-10.0f,  10.0f, -10.0f,

				-10.0f, -10.0f, -10.0f,
				-10.0f, -10.0f,  10.0f,
				10.0f, -10.0f, -10.0f,
				10.0f, -10.0f, -10.0f,
				-10.0f, -10.0f,  10.0f,
				10.0f, -10.0f,  10.0f
			};

			const u32 num_vertices = sizeof(vertices) / sizeof(float);

			u32 indices[num_vertices];

			for(u32 i = 0; i < num_vertices; ++i)
				indices[i] = i;

			skyboxGeometry = std::make_shared<Geometry>();
			skyboxGeometry->Load(owner->GetScene().GetGraphicsContext(), vertices, num_vertices, indices, num_vertices, vertex_attribs, num_attribs);
		}

		skyboxCubemap = cubemap_handle;
		hasSkybox = true;
	}

	void CameraComponent::DrawSkybox( GraphicsContext* graphics_context )
	{
		if(graphics_context == NULL || !hasSkybox || skyboxShader == 0)
			return;

		bool depth_mask = graphics_context->GetDepthMask();

		graphics_context->SetDepthMask(false);

		u32 shader = graphics_context->GetCurrentShaderProgram();

		graphics_context->UseShaderProgram(skyboxShader);
		
		graphics_context->UseTexture(1, skyboxCubemap);

		ShaderProgram skybox_program = graphics_context->GetShaderProgram(skyboxShader);

		glm::mat4 view = viewMatrix;
		view[3][0] = 0.0f; view[3][1] = 0.0f; view[3][2] = 0.0f;

		glm::mat4 view_proj = projectionMatrix * view;

		float scale = (farPlane) / 20.0f;
		glm::mat4 scale_matrix = glm::scale(glm::mat4(1.0f), glm::vec3(scale, scale, scale));
		// Set common uniforms.
		skybox_program.SetUniformFromArray("modelMatrix", (void*)glm::value_ptr(scale_matrix), 1, false);
		skybox_program.SetUniformFromArray("viewProjectionMatrix", (void*)glm::value_ptr(view_proj), 1, false);

		const Geometry* g = skyboxGeometry.get();
		graphics_context->DrawGeometry(g);

		graphics_context->UseShaderProgram(shader);
		graphics_context->SetDepthMask(depth_mask);
	}

	void CameraComponent::LoadSkyboxShader(GraphicsContext* graphics_context)
	{
		if(graphics_context == NULL || skyboxShaderLoaded)
			return;

		std::string vs_source	= StringFromFile("Shaders/skybox_vs.glsl");
		u32 vertex_shader		= graphics_context->CreateShader(VERTEX_SHADER, vs_source.c_str());

		std::string fs_source	= StringFromFile("Shaders/skybox_fs.glsl");
		u32 fragment_shader		= graphics_context->CreateShader(FRAGMENT_SHADER, fs_source.c_str());

		u32 shaders[] = { vertex_shader, fragment_shader };
		u32 num_shaders = sizeof(shaders) / sizeof(u32);

		skyboxShader = graphics_context->CreateShaderProgram(shaders, num_shaders);

		skyboxShaderLoaded = true;
	}

}

