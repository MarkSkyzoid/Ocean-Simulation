#include "Application.h"

#include <iostream>

int main()
{
	// Get settings from the user.
	acqua::AppSettings app_settings;
	
	std::cout << "Please insert the desired resolution width: ";
	std::cin >> app_settings.width;

	std::cout << "Please insert the desired resolution height: ";
	std::cin >> app_settings.height;

	char fullscreen = 'n';
	std::cout << "Fullscreen? (y/n) ";
	std::cin >> fullscreen;

	app_settings.fullscreen = (fullscreen == 'y');


	// Create and run the application.
	acqua::Application app;

	bool app_initialized = app.Init(app_settings, "Real-Time Rendering of Deep Water Volumes : I Sat By The Ocean");

	if(app_initialized)
		app.Run();

	return 0;
}