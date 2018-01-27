#include <stdio.h>
#include <stdbool.h> 
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <GL/glu.h>
#include <sys/time.h>
#include <stdint.h>

#include <fenv.h>
#include "game.c"

unsigned GetTickCount()
{
        struct timeval tv;
        if(gettimeofday(&tv, NULL) != 0)
                return 0;

        return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

//The window we'll be rendering to
SDL_Window* gWindow = NULL;

//OpenGL context
SDL_GLContext gContext;

unsigned lastTick;

bool initGL()
{
	bool success = true;
	GLenum error = GL_NO_ERROR;

	//Initialize Projection Matrix
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	
	//Check for error
	error = glGetError();
	if( error != GL_NO_ERROR )
	{
		printf( "Error initializing OpenGL! %s\n", gluErrorString( error ) );
		success = false;
	}

	//Initialize Modelview Matrix
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();

	//Check for error
	error = glGetError();
	if( error != GL_NO_ERROR )
	{
		printf( "Error initializing OpenGL! %s\n", gluErrorString( error ) );
		success = false;
	}
	
	//Initialize clear color
	glClearColor( 0.f, 0.f, 0.f, 1.f );
	
	//Check for error
	error = glGetError();
	if( error != GL_NO_ERROR )
	{
		printf( "Error initializing OpenGL! %s\n", gluErrorString( error ) );
		success = false;
	}
	
	return success;
}

bool initLibs(int SCREEN_WIDTH, int SCREEN_HEIGHT)
{
	bool success = true;

	if( SDL_Init( SDL_INIT_VIDEO ) < 0 )
	{
		printf( "SDL could not initialize! SDL Error: %s\n", SDL_GetError() );
		success = false;
	}
	else
	{
		//Use OpenGL 2.1
		SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 2 );
		SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 1 );

		
		gWindow = SDL_CreateWindow( "oofswarm", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
		if( gWindow == NULL )
		{
			printf( "Window could not be created! SDL Error: %s\n", SDL_GetError() );
			success = false;
		}
		else
		{
			gContext = SDL_GL_CreateContext( gWindow );
			if( gContext == NULL )
			{
				printf( "OpenGL context could not be created! SDL Error: %s\n", SDL_GetError() );
				success = false;
			}
			else
			{
				if( SDL_GL_SetSwapInterval( 1 ) < 0 )
				{
					printf( "Warning: Unable to set VSync! SDL Error: %s\n", SDL_GetError() );
				}

				if( !initGL() )
				{
					printf( "Unable to initialize OpenGL!\n" );
					success = false;
				}

				if (!IMG_Init(IMG_INIT_PNG))
				{
					printf("Unable to initialize SDL_Image\n");
					success = false;
				}
			}
		}
	}

	return success;
}

void close()
{
	SDL_DestroyWindow( gWindow );
	gWindow = NULL;

	SDL_Quit();
}

int main(int argc, char const *argv[])
{
	// make sure to catch sources of NAN and INF
	feenableexcept(FE_INVALID | FE_OVERFLOW);
	game.window_width = 640;
	game.window_height = 420;
	if (!initLibs(game.window_width, game.window_height))
	{
		close();
		return 1;
	}

		// Main loop flag
		bool quit = false;

		// Event handler
		SDL_Event e;
		
		// Enable text input
		SDL_StartTextInput();

		// define ship classes
		ShipClass fighter;
		fighter.acceleration = 0.5f;
		fighter.speed = 5.f;
		fighter.damageModifiers[0] = 50.f;
		fighter.damageModifiers[1] = 100.f;
		fighter.damageModifiers[2] = 5.f;
		fighter.sensorRange = 15.f;
		fighter.weaponRange = 7.5f;
		fighter.fireSpeed = 10.f;
		fighter.baseHealth = 100.f;
		game.shipClasses[0] = fighter;

		ShipClass bomber;
		bomber.acceleration = 0.1f;
		bomber.speed = 3.f;
		bomber.damageModifiers[0] = 5.f;
		bomber.damageModifiers[1] = 50.f;
		bomber.damageModifiers[2] = 100.f;
		bomber.sensorRange = 30.f;
		bomber.weaponRange = 7.5f;
		bomber.fireSpeed = 1.f;
		bomber.baseHealth = 100.f;
		game.shipClasses[1] = bomber;

		ShipClass cruiser;
		cruiser.acceleration = 0.2f;
		cruiser.speed = 2.f;
		cruiser.damageModifiers[0] = 100.f;
		cruiser.damageModifiers[1] = 100.f;
		cruiser.damageModifiers[2] = 25.f;
		cruiser.sensorRange = 50.f;
		cruiser.weaponRange = 7.5f;
		cruiser.fireSpeed = 0.5f;
		cruiser.baseHealth = 1000.f;
		game.shipClasses[2] = cruiser;

		// for (int i = 0; i < 10; ++i)
		// {
		// 	printf("%f\n", randomFloat());
		// }

		newGame(GetTickCount(), 250, 15);
		loadAssets();
		createUI();
		game.window_width = 640;
		game.window_height = 420;
		game.aspectRatio = 640.f / 420.f;
		game.debuglevel = 0;

		//While game not terminating
		while( !quit )
		{
			while( SDL_PollEvent( &e ) != 0 )
			{
				if( e.type == SDL_QUIT )
				{
					quit = true;
				}

				// Handle keypress with current mouse position
				else if( e.type == SDL_KEYDOWN )
				{
					int x = 0, y = 0;
					SDL_GetMouseState( &x, &y );
					handleKeys( e.key.keysym.scancode, x, y );
				}

				// process resizing
				else if (e.window.event == SDL_WINDOWEVENT_RESIZED)
				{
					resizeWindow(e);
				}

				// zoom
				else if (e.type == SDL_MOUSEWHEEL)
				{
					int numScrolls = e.wheel.y;
					// printf("%d\n", numScrolls);
					if (numScrolls == 1)
					{
						game.cameraZoom *= 1.1f;
					}
					if (numScrolls == -1)
					{
						game.cameraZoom /= 1.1f;
					}
				} else if (e.type == SDL_MOUSEBUTTONDOWN)
				{
					if (e.button.button == SDL_BUTTON_LEFT || e.button.button == SDL_BUTTON_RIGHT)
					{
						handleMouseButtons(e.button.button, e.button.x, e.button.y);
					}
				}
			}

			float step = (GetTickCount() - lastTick) / 1000.0;
			lastTick = GetTickCount();

			if (step > 0.1f)
			{
				step = 0.1f;
			}

			tickGame(step);

			//Render quad
			renderGame();
			
			//Update screen
			SDL_GL_SwapWindow( gWindow );
		}
		
		//Disable text input
		SDL_StopTextInput();

	return 0;
}