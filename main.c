#include <stdio.h>
#include <stdbool.h> 
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <GL/glu.h>
#include <sys/time.h>

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

int window_width;
int window_height;

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
			}
		}
	}

	return success;
}

void resizeWindow(SDL_Event event)
{
	window_width = event.window.data1;
	window_height = event.window.data2;
	game.aspectRatio = (float) window_width / window_height;
	glViewport(0, 0, window_width, window_height);
	printf("Window resized %d %d\n", window_width, window_height);
}

void close()
{
	SDL_DestroyWindow( gWindow );
	gWindow = NULL;

	SDL_Quit();
}

void handleKeys( unsigned char key, int x, int y )
{
	if (key == SDL_SCANCODE_KP_PLUS)
	{
		game.speedModifier *= 2.f;
	}
	if (key == SDL_SCANCODE_KP_MINUS)
	{
		game.speedModifier /= 2.f;
	}
	if (key == SDL_SCANCODE_S)
	{
		game.currentWave.shipsToSpawn[0] = 50;
	}
	if (key == SDL_SCANCODE_D)
	{
		for (int i = 0; i < game.numShips; ++i)
		{
			if (game.ships[i].target != NULL)
			{
				printf("p%d h%f l%f %f\n", game.ships[i].target, game.ships[i].target->health,
					game.ships[i].target->position.x, game.ships[i].target->position.y);
			}
		}
	}
}

int main(int argc, char const *argv[])
{
	feenableexcept(FE_INVALID | FE_OVERFLOW);
	window_width = 640;
	window_height = 420;
	if (!initLibs(window_width, window_height))
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
		fighter.acceleration = 2.f;
		fighter.speed = 2.f;
		fighter.damageModifiers[0] = 50;
		fighter.sensorRange = 50.f;
		fighter.weaponRange = 7.5f;
		fighter.fireSpeed = 10.f;
		fighter.baseHealth = 100.f;
		game.shipClasses[0] = fighter;

		// for (int i = 0; i < 10; ++i)
		// {
		// 	printf("%f\n", randomFloat());
		// }

		newGame(0, 500, 50);
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
				}
			}

			float step = (GetTickCount() - lastTick) / 1000.0;
			lastTick = GetTickCount();

			if (step > 0.1f)
			{
				step = 0.1f;
			}

			tickGame(step);

			glViewport(0,0,window_width,window_height);

			//Render quad
			renderGame();
			
			//Update screen
			SDL_GL_SwapWindow( gWindow );
		}
		
		//Disable text input
		SDL_StopTextInput();

	return 0;
}