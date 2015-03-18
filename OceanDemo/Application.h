#pragma once

#include <string>

#include <AntTweakBar.h>

// Forward declarations
namespace sf
{
	class Window;
}

namespace acqua
{
	class GraphicsContext;
}

// Yep, it's named acqua (water).
namespace acqua
{

	struct AppSettings
	{
		int			width;
		int			height;
		bool		fullscreen;
		std::string title;

		AppSettings() : width(1920), height(1080), fullscreen(false), title("")
		{
		}
	};

	class Application
	{
	public:
		Application(void);
		~Application(void);

		bool Init(const AppSettings& app_settings, const std::string& window_title);

		void Run();

	private:

		void SFMLEventLoop();

	private:
		sf::Window*		window;
		bool			running;

		AppSettings		appSettings;

		// Subsystems.
		GraphicsContext* graphicsContext;

		// KEEP THESE AT THE BOTTOM
		//GUI - TODO: Ocean specific remove it when taking the engine
		TwBar* GUISystem;
	};
}
