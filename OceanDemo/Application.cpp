#include "Application.h"
#include "GraphicsContext.h"
#include "Geometry.h"

#include "Scene.h"
#include "GameObject.h"

#include "BasicIO.h" // TODO: Testing, so remove later.

#include "CameraComponent.h"
#include "OceanComponent.h"
#include "TerrainComponent.h"

#include <GL\glew.h>
#include <SFML\Window.hpp>
#include <SFML\Graphics.hpp>
#include <math.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace acqua
{
	// Global Settings for the ocean.
	// Ocean specific - TODO: Remove when taking the engine bit.
	OceanSettings gOceanSettings;
	
	OceanComponent* gCurrentOcean = NULL;

	// Ocean specific - TODO: Remove when taking the engine bit.
	void TW_CALL ApplyOceanSettings( void* )
	{
		if(gCurrentOcean == NULL)
			return;

		gCurrentOcean->ResetOcean(gOceanSettings);
	}


	Application::Application(void) :
		  window(nullptr)
		, running(false)
		, graphicsContext(nullptr)
		, GUISystem(NULL)
	{
	}


	Application::~Application(void)
	{
		delete graphicsContext;
		delete window;

		// Terminate GUI System.
		//TwTerminate();
	}

	bool Application::Init(const AppSettings& app_settings, const std::string& window_title)
	{
		bool result = false;

		// Initialize SFML Window and OpenGL Context
		sf::ContextSettings settings;
		settings.depthBits = 24;
		settings.stencilBits = 8;
		settings.antialiasingLevel = 4;

		sf::Uint32 window_style = sf::Style::Close;
		
		appSettings = app_settings;
		if(appSettings.fullscreen)
			window_style |= sf::Style::Fullscreen;

		appSettings.title = window_title;
		window = new sf::Window(sf::VideoMode(appSettings.width, appSettings.height), window_title, window_style, settings);

		// Initialize the OpenGL graphics context
		graphicsContext = new GraphicsContext();
		bool gfx_context_result = graphicsContext->Init();

		result = (window != nullptr) && gfx_context_result;

		// Initialize GUI System.
#pragma region TWEAK_BAR_INITIALIZATION
		TwInit(TW_OPENGL_CORE, NULL);
		TwWindowSize(appSettings.width, appSettings.height);

		GUISystem = TwNewBar("Ocean Settings");
		TwAddVarRW(GUISystem, "Wave speed", TW_TYPE_FLOAT, &gOceanSettings.V, NULL);
		TwAddVarRW(GUISystem, "Shortest Wavelength", TW_TYPE_FLOAT, &gOceanSettings.l, NULL);
		TwAddVarRW(GUISystem, "Amplitude", TW_TYPE_FLOAT, &gOceanSettings.A, NULL);
		TwAddVarRW(GUISystem, "Wind Direction (Rad.)", TW_TYPE_FLOAT, &gOceanSettings.W, NULL);
		TwAddVarRW(GUISystem, "Wind Alignment", TW_TYPE_FLOAT, &gOceanSettings.windAlignment, NULL);
		TwAddVarRW(GUISystem, "Reflections Damping", TW_TYPE_FLOAT, &gOceanSettings.dampReflections, NULL);
		TwAddVarRW(GUISystem, "Depth", TW_TYPE_FLOAT, &gOceanSettings.depth, NULL);
		TwAddVarRW(GUISystem, "Choppiness", TW_TYPE_FLOAT, &gOceanSettings.chopAmount, NULL);

		TwAddVarRW(GUISystem, "Foam Slope Start", TW_TYPE_FLOAT, &gOceanSettings.foamSlopeRatio, NULL);
		TwAddVarRW(GUISystem, "Foam Fader", TW_TYPE_FLOAT, &gOceanSettings.foamFader, NULL);

		TwAddButton(GUISystem, "Apply", ApplyOceanSettings, NULL, NULL);

#pragma endregion

		return result;
	}

	
	u32 terrain_normal_texture_handle;
	u32 terrain_diffuse_texture_handle;
	u32 normal_texture_handle;
	u32 foam_texture_handle;

	void Application::Run()
	{
		running = true;

		// TODO: REMOVE!
		/***** TEST CODE *****/

		VertexLayoutAttrib vl_attribs[] =
		{
			{3, 0} // Position
		};

		float vertices[]	= { 
								0.0f,  0.7f, 0.0f, // Vertex 1 (X, Y)
								0.5f, -0.5f, 0.0f,// Vertex 2 (X, Y)
								-0.5f, -0.5f, 0.0f  // Vertex 3 (X, Y)
							  };

		u32 indices[]	= {0, 1, 2};

		std::shared_ptr<Geometry> geometry = std::make_shared<Geometry>();
		geometry->Load(graphicsContext, vertices, 3, indices, 3, vl_attribs, 1);

		u32 ocean_shader_program = 0;
		{
			std::string vs_source	= StringFromFile("Shaders/TessellationTest/test_vs.glsl");
			u32 vertex_shader		= graphicsContext->CreateShader(VERTEX_SHADER, vs_source.c_str());

			std::string fs_source	= StringFromFile("Shaders/TessellationTest/test_fs.glsl");
			u32 fragment_shader		= graphicsContext->CreateShader(FRAGMENT_SHADER, fs_source.c_str());

			std::string tcs_source	= StringFromFile("Shaders/TessellationTest/test_tcs.glsl");
			u32 tess_control_shader		= graphicsContext->CreateShader(TESSELATION_CONTROL_SHADER, tcs_source.c_str());
			std::string tes_source	= StringFromFile("Shaders/TessellationTest/test_tes.glsl");
			u32 tess_eval_shader		= graphicsContext->CreateShader(TESSELLATION_EVALUATION_SHADER, tes_source.c_str());

			std::string gs_source	= StringFromFile("Shaders/TessellationTest/gs.glsl");//StringFromFile("Shaders/test_tes.glsl");
			u32 geometry_shader		= graphicsContext->CreateShader(GEOMETRY_SHADER, gs_source.c_str());

			u32 shaders[] = { vertex_shader, tess_control_shader, tess_eval_shader,  fragment_shader };
			u32 num_shaders = sizeof(shaders) / sizeof(u32);

			ocean_shader_program = graphicsContext->CreateShaderProgram(shaders, num_shaders);
		}

		u32 terrain_shader_program = 0;
		{
			std::string vs_source	= StringFromFile("Shaders/terrain_vs.glsl");
			u32 vertex_shader		= graphicsContext->CreateShader(VERTEX_SHADER, vs_source.c_str());

			std::string fs_source	= StringFromFile("Shaders/terrain_fs.glsl");
			u32 fragment_shader		= graphicsContext->CreateShader(FRAGMENT_SHADER, fs_source.c_str());

			u32 shaders[] = { vertex_shader, fragment_shader };
			u32 num_shaders = sizeof(shaders) / sizeof(u32);

			terrain_shader_program = graphicsContext->CreateShaderProgram(shaders, num_shaders);
		}

		/**** END OF TEST ****/
		
		/***** TEXTURE TEST *****/
		sf::Image terrain_normal_texture;
		if(!terrain_normal_texture.loadFromFile("Textures/Island_N.png"))
		{
			ASSERT(false, "Can't load texture!")
		}

		sf::Image terrain_diffuse_texture;
		if(!terrain_diffuse_texture.loadFromFile("Textures/Island_D.png"))
		{
			ASSERT(false, "Can't load texture!")
		}

		sf::Image normal_texture;
		if(!normal_texture.loadFromFile("Textures/ocean_normalmap.png"))
		{
			ASSERT(false, "Can't load texture!")
		}

		sf::Image foam_texture;
		if(!foam_texture.loadFromFile("Textures/Foam_D.png"))
		{
			ASSERT(false, "Can't load texture!")
			std::cout << "Can't load foam texture\n";
		}

		//Load skybox textures;
		std::string skybox_filenames[] = {"1.png", "2.png", "3.png", "4.png", "5.png", "6.png"};
		sf::Image skybox_textures[6];
		for(int i = 0; i < 6; ++i)
		{
			std::string path = "Textures/skybox/";
			path += skybox_filenames[i];
			if(!skybox_textures[i].loadFromFile(path))
			{
				ASSERT(false, "Can't load texture!")
			}
		}

		terrain_normal_texture_handle = graphicsContext->CreateTexture(TextureTypes::List::Tex2D, terrain_normal_texture.getSize().x, terrain_normal_texture.getSize().y, 32, TextureFormats::RGBA, false, true, false, false);
		graphicsContext->UploadTextureData(terrain_normal_texture_handle, 0, 0, static_cast<const void*>(terrain_normal_texture.getPixelsPtr()));

		terrain_diffuse_texture_handle = graphicsContext->CreateTexture(TextureTypes::List::Tex2D, terrain_diffuse_texture.getSize().x, terrain_diffuse_texture.getSize().y, 32, TextureFormats::RGBA, false, true, false, false);
		graphicsContext->UploadTextureData(terrain_diffuse_texture_handle, 0, 0, static_cast<const void*>(terrain_diffuse_texture.getPixelsPtr()));


		normal_texture_handle = graphicsContext->CreateTexture(TextureTypes::List::Tex2D, normal_texture.getSize().x, normal_texture.getSize().y, 32, TextureFormats::RGBA, false, false, false, false);
		graphicsContext->UploadTextureData(normal_texture_handle, 0, 0, static_cast<const void*>(normal_texture.getPixelsPtr()));

		foam_texture_handle = graphicsContext->CreateTexture(TextureTypes::List::Tex2D, foam_texture.getSize().x, foam_texture.getSize().y, 32, TextureFormats::RGBA, false, true, false, false);
		graphicsContext->UploadTextureData(foam_texture_handle, 0, 0, static_cast<const void*>(foam_texture.getPixelsPtr()));

		u32 skybox_texture_handle = graphicsContext->CreateTexture(TextureTypes::List::TexCube, skybox_textures[0].getSize().x, skybox_textures[0].getSize().y, 32, TextureFormats::RGBA, false, false, false, false);
		for(int s = 0; s < 6; ++s)
		{
			graphicsContext->UploadTextureData(skybox_texture_handle, s, 0, static_cast<const void*>(skybox_textures[s].getPixelsPtr()));
		}

		graphicsContext->UseTexture(0, skybox_texture_handle);

		/***** END TEXTURE  *****/
		
		/* SCENE TEST */

		Scene scene;
		scene.Init(graphicsContext);

		GameObject& camera = scene.CreateGameObject();
		camera.AddComponent("CameraComponent");
		camera.GetTransform().SetPosition(glm::vec3(10.0f, 10.0f, 5.0f));
		camera.GetComponent<CameraComponent>()->GenerateSkybox(skybox_texture_handle);

		/*GameObject& triangle = scene.CreateGameObject();
		triangle.AddComponent("GeometryRenderer");
		GeometryRenderer* triangle_gr = triangle.GetComponent<GeometryRenderer>();
		if(triangle_gr)
		{
			triangle_gr->SetGeometry(geometry);
		}*/

		GameObject& ocean = scene.CreateGameObject();
		ocean.AddComponent("OceanComponent");
		gCurrentOcean = ocean.GetComponent<OceanComponent>();
		GeometryRenderer* ocean_renderer = ocean.GetComponent<GeometryRenderer>();
		if(ocean_renderer != NULL)
			ocean_renderer->SetShaderProgram(ocean_shader_program);

		graphicsContext->SetScreenSize(appSettings.width, appSettings.height);
		scene.ResolutionChanged(appSettings.width, appSettings.height);

		GameObject& island = scene.CreateGameObject();
		island.AddComponent("TerrainComponent");
		TerrainComponent* terrain_component = island.GetComponent<TerrainComponent>();
		if(terrain_component != NULL)
			terrain_component->LoadFromHeightMap("Textures/Island_H.png");
		island.GetTransform().SetPosition(glm::vec3(0.0f, -150.0f * 3.0f, 0.0f));
		GeometryRenderer* terrain_renderer = island.GetComponent<GeometryRenderer>();
		if(terrain_renderer != NULL)
			terrain_renderer->SetShaderProgram(terrain_shader_program);

		
		graphicsContext->UseShaderProgram(ocean_shader_program);
		graphicsContext->SetScreenSize(appSettings.width, appSettings.height);
		scene.ResolutionChanged(appSettings.width, appSettings.height);

		/**************/
		sf::Clock timer;
		float current_time = timer.getElapsedTime().asSeconds();
		float last_time = current_time;

		float time_accumulator = 0.0f;

		float yaw = 0.f; 
		float pitch = 0.f;

		glm::vec3 camera_direction = glm::vec3(0.0f);
		while(running)
		{
			float delta_time = current_time - last_time;
			last_time = current_time;

			SFMLEventLoop();

		/*	graphicsContext->SetColourMask(true, true, true, true);
			graphicsContext->SetDepthMask(true);
			graphicsContext->SetClearColour(glm::vec3(0.776f, 0.325f, 0.321f));
			graphicsContext->SetClearDepth(1.0f);

			graphicsContext->Clear();*/

			/*TODO REMOVE THIS CODE*/
			/*JUST FOR TESTING     */
			float forward_mult = (float)sf::Keyboard::isKeyPressed(sf::Keyboard::W) - (float)sf::Keyboard::isKeyPressed(sf::Keyboard::S);
			float strife_mult =  (float)sf::Keyboard::isKeyPressed(sf::Keyboard::D) - (float)sf::Keyboard::isKeyPressed(sf::Keyboard::A);
			float up_mult = (float)sf::Keyboard::isKeyPressed(sf::Keyboard::Q) - (float)sf::Keyboard::isKeyPressed(sf::Keyboard::Z);
			
			static bool dragging_mouse = false;
			static sf::Vector2i mouse_pos;

			if(sf::Mouse::isButtonPressed(sf::Mouse::Right))
			{

				if(!dragging_mouse)
				{
					dragging_mouse = true;
					mouse_pos = sf::Mouse::getPosition(*window);
				}

				float yaw_mult = (float)sf::Keyboard::isKeyPressed(sf::Keyboard::Right) - (float)sf::Keyboard::isKeyPressed(sf::Keyboard::Left);
				float pitch_mult = (float)sf::Keyboard::isKeyPressed(sf::Keyboard::S) - (float)sf::Keyboard::isKeyPressed(sf::Keyboard::W);
				
				pitch_mult = ((float)sf::Mouse::getPosition(*window).y - mouse_pos.y) / 50.0f;
				yaw_mult = ((float)sf::Mouse::getPosition(*window).x - mouse_pos.x) / 50.0f;
				std::cout << "MouseY = " << (float)sf::Mouse::getPosition(*window).y << std::endl;

				pitch += pitch_mult * 25.0f * delta_time;// * 0.0174532925f;
				yaw += yaw_mult * 25.0f * delta_time;// * 0.0174532925f;
				pitch = pitch > 60.0f ? 60.0f : (pitch < -60.0f ? -60.0f : pitch);

				glm::vec3 up(0.0f, 1.0f, 0.0f);
				float sin_yaw_half = sin(yaw * 0.5f);
			
				glm::quat result = glm::angleAxis(yaw, up);
				camera.GetTransform().SetOrientation(result);
				result = result * glm::angleAxis(pitch, camera.GetTransform().GetRight());

				camera.GetTransform().SetOrientation(result);
			}
			else
			{
				dragging_mouse = false;
			}

			glm::vec3 camera_pos = camera.GetTransform().GetPosition() + camera.GetTransform().GetForward() * 375.0f * forward_mult * delta_time;
			camera_pos = camera_pos + camera.GetTransform().GetUp() * 375.0f * up_mult * delta_time;
			camera_pos = camera_pos + camera.GetTransform().GetRight() * 375.0f * strife_mult * delta_time;
			camera.GetTransform().SetPosition(camera_pos);

			{
				if(sf::Keyboard::isKeyPressed(sf::Keyboard::T))
				{
					graphicsContext->ToggleWireframeDrawing();
				}
			}
			/*************************/

			scene.Update(delta_time);
			scene.Draw(delta_time);

#pragma region TWEAK_BAR GUI DRAW

			bool need_wireframe_off = graphicsContext->IsWireframeEnabled();
			if(need_wireframe_off)
				graphicsContext->ToggleWireframeDrawing();

			TwDraw();

			if(need_wireframe_off)
				graphicsContext->ToggleWireframeDrawing();

#pragma endregion


			// Swap buffers
			window->display();
			
			current_time = timer.getElapsedTime().asSeconds();

			time_accumulator += delta_time;
			if(time_accumulator >= 0.5f)
			{
				time_accumulator = 0.0f;
				std::ostringstream str;
				str << appSettings.title << " - FPS: " << (1.0f / delta_time) << " (" << delta_time * 1000.0f << " ms)";
				window->setTitle(str.str());
			}
			//printf("Delta Time: %f - FPS: %f\n", delta_time, 1.0f / delta_time);
		}
	}

	void Application::SFMLEventLoop()
	{
		if(window == nullptr)
			return; // Nothing to do here

		sf::Event window_event;
		while(window->pollEvent(window_event))
		{
			// Send event to AntTweakBar
			int handled = TwEventSFML(&window_event, 2, 1); 

			// If event has not been handled by AntTweakBar, process it
			if(handled)
				continue;

			switch(window_event.type)
			{
			case sf::Event::Closed :
				{
					running = false;
					break;
				}

			default:
				break;
			}
		}
	}
}
