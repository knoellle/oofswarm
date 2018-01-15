#include <stdio.h>
#include <stdbool.h> 
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <GL/glu.h>
#include <stdlib.h>
#include <math.h>

struct Tile
{
	// float bonus[3];
	int   buildingType;
	int   buildingLevel;
};

struct Planet
{
	float x;
	float y;
	float radius;
	int   seed;
	Tile  *tiles;
	int numTiles;
};

struct Ship
{
	float x;  // position
	float y;
	float vx; // veloctiy
	float vy;
	// type 0 = none/dead
	//      1 = fighter
	//      2 = cruiser
	//      3 = bomber
	int type;
	// team 0 = player
	//      1 = enemy
	int team;
	float health;
};

struct Game
{
	float gameAge;
	int seed;
	int galaxyRadius;

	Planet *planets;
	int numPlanets;

	Ship *ships;
	int numShips;
	int lenShips;

	// ratio of height and width of the window
	float aspectRatio;
};

Game game;

void clearGame()
{
	game.seed = 0;
	game.galaxyRadius = 0;
	if (game.numPlanets > 0)
	{
		for (int i = 0; i < game.numPlanets; i++)
		{
			free(game.planets[i].tiles);
		}
		free(game.planets);
	}
	game.numPlanets = 0;
}

void newGame(int seed, float galaxyRadius, int planets)
{
	// make sure no old data survives
	clearGame();
	
	// set basic values
	game.seed = seed;
	game.galaxyRadius = galaxyRadius;
	game.numPlanets = planets;
	
	// generate planets
	// it is important to seed the planets individually first,
	// so the seeds can be used without interference
	srand(game.seed);
	game.planets = (Planet*) malloc(sizeof(Planet) * planets);
	for (int i = 0; i < game.numPlanets; i++)
	{
		game.planets[i].seed = rand();
	}

	// actually generate planet values
	float r = 0.f;
	float a = 0.f;
	for (int i = 0; i < game.numPlanets; i++)
	{
		Planet* planet = &game.planets[i];
		srand(game.planets[i].seed);
		// positioning
		planet->x = cos(a) * r;
		planet->y = sin(a) * r;
		r = r + game.galaxyRadius / game.numPlanets;
		a = a + planet->seed;
		// size
		planet->radius = (float) 5 + (rand() % 15);
		planet->numTiles = round(planet->radius);
		planet->tiles = (Tile*) malloc(sizeof(Tile) * planet->numTiles);
		for (int t = 0; t < planet->numTiles; t++)
		{
			Tile* tile = &planet->tiles[i];
			// tile->bonus[0] = rand() % 100 > 75 ? rand() % 3 : 0;
			// tile->bonus[1] = rand() % 100 > 75 ? rand() % 3 : 0;
			// tile->bonus[2] = rand() % 100 > 75 ? rand() % 3 : 0;
		}
	}	
}

void tickGame(float step)
{
	
}

void renderGame()
{
	
	// Set up projection matrix for game world
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	glOrtho(-100.f * game.aspectRatio, 100.f * game.aspectRatio, -100.f, 100.f, -1.f, 100.f);
	printf("%f\n", game.aspectRatio);
	// Reset camera
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();

	// Clear color buffer
	glClear( GL_COLOR_BUFFER_BIT );
	
	// Render background
	glColor3f(0.3f, 0.3f, 0.3f);
	glBegin( GL_QUADS );
		glVertex2f( -100.f * game.aspectRatio, -100.f );
		glVertex2f( 100.f * game.aspectRatio, -100.f );
		glVertex2f( 100.f * game.aspectRatio, 100.f );
		glVertex2f( -100.f * game.aspectRatio, 100.f );
	glEnd();
	glColor3f(1.f, 1.f, 1.f);

	glClear( GL_DEPTH_BUFFER_BIT );

	// Render world
	for (int i = 0; i < game.numPlanets; i++)
	{
		float r = game.planets[i].radius;
		glPushMatrix();
			glTranslatef(game.planets[i].x, game.planets[i].y, 0);
			glBegin( GL_QUADS );
				glVertex2f( -r, -r );
				glVertex2f(  r, -r );
				glVertex2f(  r,  r );
				glVertex2f( -r,  r );
			glEnd();
		glPopMatrix();
	}

	// Render UI
}