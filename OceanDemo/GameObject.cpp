#include "GameObject.h"
#include "DebugUtil.h"
#include "GraphicsContext.h"
#include "CameraComponent.h"

#include <glm/gtc/matrix_transform.hpp>


namespace acqua
{

	// Transform class implementation.
	Transform::Transform( void ) :
		dirtyFlags(TDC_NONE)
		, transformMatrix(1.0f)
		, position(0.0f)
		, scale(1.0f)
		, forward(0.0f, 0.0f, -1.0f)
		, up(0.0f, 1.0f, 0.0f)
		, right(1.0f, 0.0f, 0.0f)
	{
		int i = 0;
	}

	Transform::~Transform( void )
	{

	}
	
	void Transform::MarkDirty()
	{
		dirtyFlags = TDC_ALL;
	}

	void Transform::MarkDirty(TransformDirtyFlags dirty_flag)
	{
		dirtyFlags |= dirty_flag;
	}

	const glm::mat4& Transform::GetMatrix() const
	{
		if(dirtyFlags != TDC_NONE)
		{
			RecomputeMatrix();
		}

		return transformMatrix;
	}

	void Transform::SetPosition( glm::vec3 pos )
	{
		position = pos;
		MarkDirty(TDC_POSITION);
	}

	void Transform::SetOrientation( glm::quat or )
	{
		MarkDirty(TDC_ORIENTATION);

		orientation = or;
		glm::vec4 f = glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
		forward = forward * orientation;

		up = up * orientation;

		right = right * orientation;

		forward = glm::normalize(glm::vec3(0.0f, 0.0f, -1.0f) * orientation);
		up		= glm::normalize(glm::vec3(0.0f, 1.0f,  0.0f) * orientation);
		right	= glm::normalize(glm::vec3(1.0f, 0.0f,  0.0f) * orientation);
		//forward = glm::vec3( 2 * (orientation.x * orientation.z + orientation.w * orientation.y), 2 * (orientation.y * orientation.x - orientation.w * orientation.x), 1 - 2 * (orientation.x * orientation.x + orientation.y * orientation.y));
		//up = glm::vec3( 2 * (orientation.x * orientation.y + orientation.w * orientation.z), 1 - 2 * (orientation.x * orientation.x - orientation.z * orientation.z), 2 * (orientation.y * orientation.z + orientation.w * orientation.x));
		//right = glm::vec3( 1 - 2 * (orientation.y * orientation.y + orientation.z * orientation.z), 2 * (orientation.x * orientation.y - orientation.w * orientation.z), 2 * (orientation.x * orientation.z + orientation.w * orientation.y));
	}

	void Transform::SetEulerAngles(float x, float y, float z)
	{
		MarkDirty(TDC_ORIENTATION);
		orientation  = glm::quat(glm::vec3(x, y, z));

		forward = glm::vec3( 2 * (orientation.x * orientation.z + orientation.w * orientation.y), 2 * (orientation.y * orientation.x - orientation.w * orientation.x), 1 - 2 * (orientation.x * orientation.x + orientation.y * orientation.y));
		up = glm::vec3( 2 * (orientation.x * orientation.y + orientation.w * orientation.z), 1 - 2 * (orientation.x * orientation.x - orientation.z * orientation.z), 2 * (orientation.y * orientation.z + orientation.w * orientation.x));
		right = glm::vec3( 1 - 2 * (orientation.y * orientation.y + orientation.z * orientation.z), 2 * (orientation.x * orientation.y - orientation.w * orientation.z), 2 * (orientation.x * orientation.z + orientation.w * orientation.y));

		//forward = glm::normalize(glm::vec3(0.0f, 0.0f, -1.0f) * orientation);
		//up		= glm::normalize(glm::vec3(0.0f, 1.0f,  0.0f) * orientation);
		//right	= glm::normalize(glm::vec3(1.0f, 0.0f,  0.0f) * orientation);
	}

	void Transform::SetAxisRotation( const glm::vec3& axis, float angle )
	{
		MarkDirty(TDC_ORIENTATION);
		orientation = glm::angleAxis(angle, axis);

		forward = glm::vec3( 2 * (orientation.x * orientation.z + orientation.w * orientation.y), 2 * (orientation.y * orientation.x - orientation.w * orientation.x), 1 - 2 * (orientation.x * orientation.x + orientation.y * orientation.y));
		up = glm::vec3( 2 * (orientation.x * orientation.y + orientation.w * orientation.z), 1 - 2 * (orientation.x * orientation.x - orientation.z * orientation.z), 2 * (orientation.y * orientation.z + orientation.w * orientation.x));
		right = glm::vec3( 1 - 2 * (orientation.y * orientation.y + orientation.z * orientation.z), 2 * (orientation.x * orientation.y - orientation.w * orientation.z), 2 * (orientation.x * orientation.z + orientation.w * orientation.y));

		//forward = glm::normalize(glm::vec3(0.0f, 0.0f, -1.0f) * orientation);
		//up		= glm::normalize(glm::vec3(0.0f, 1.0f,  0.0f) * orientation);
		//right	= glm::normalize(glm::vec3(1.0f, 0.0f,  0.0f) * orientation);
	}

	void Transform::RecomputeMatrix() const
	{
		// TODO: Maybe optimize based on which flag was set.
		transformMatrix = glm::translate(glm::mat4(1.0f), position) * glm::mat4_cast(orientation) * glm::scale(glm::mat4(1.0f), scale);

		ResetDirtyFlag();
	}
	
	glm::vec3 Transform::GetEulerAngles() const
	{
		return glm::eulerAngles(orientation);
	}

	// GeometryRenderer class implementation
	GeometryRenderer::~GeometryRenderer( void )
	{
		if(owner != NULL)
			owner->GetScene().UnregisterGeometryRenderer(this); // Play nicely and let Scene know you are out of the way.
	}
	
	void GeometryRenderer::Draw(float delta_time)
	{
		// DONE: Consider registering the component into the scene when initialized.
		// (REMEMBER TO UNREGISTER IT ON DESTRUCTION/CLEANUP!!)
		// Doing so, the scene can use it to render the geometry instead of delegate the rendering to this function.
	}

	bool GeometryRenderer::Init( GameObject* o )
	{
		Component::Init(o); 

		componentType = CT_GEOMTRYRENDERER;

		owner->GetScene().RegisterGeometryRenderer(this);
		
		return true;
	}

	// GameObject class implementation.
	GameObject::GameObject(void) :
		owner(NULL)
	{
	}

	GameObject::~GameObject(void)
	{
		std::map<ComponentType, Component*>::iterator it;
		for(it = components.begin(); it != components.end(); ++it)
		{
			delete it->second;
		}
	}

	Scene& GameObject::GetScene()
	{
		ASSERT(owner != NULL, "Owning scene is NULL. Cannot be possible."); 
		
		return *owner;
	}

	void GameObject::FixedUpdate( float fixed_delta_time )
	{
		std::map<ComponentType, Component*>::iterator it;
		for(it = components.begin(); it != components.end(); ++it)
		{
			it->second->FixedUpdate(fixed_delta_time);
		}
	}

	void GameObject::Update( float delta_time )
	{
		std::map<ComponentType, Component*>::iterator it;
		for(it = components.begin(); it != components.end(); ++it)
		{
			it->second->Update(delta_time);
		}
	}

	void GameObject::Draw( float delta_time )
	{
		std::map<ComponentType, Component*>::iterator it;
		for(it = components.begin(); it != components.end(); ++it)
		{
			it->second->Draw(delta_time);
		}
	}

	bool GameObject::AddComponent( const char* name )
	{
		PComponent c = Component::Create(name);
		bool result = (c != NULL) && c->Init(this);

		if(result)
			components[c->GetType()] = c;

		return result;
	}
}
