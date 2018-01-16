#include <stdio.h>
#include <stdbool.h> 
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <GL/glu.h>
#include <stdlib.h>
#include <math.h>
#include "vectors.c"

#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)>(b))?(a):(b))

struct Tile
{
	// float bonus[3];
	int   buildingType;
	int   buildingLevel;
};

struct Planet
{
	Vector2f position;
	float radius;
	int   seed;
	Tile  *tiles;
	int numTiles;

	float shipPresence[3];
};

struct Ship
{
	Vector2f position;
	Vector2f velocity;
	Vector2f force;
	// type 0 = fighter
	//      1 = cruiser
	//      2 = bomber
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

	// 0: none
	// 1: visual indicators
	// 2: console outputs
	int debuglevel;
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

Ship* spawnShip(Vector2f position, int type, int team)
{
	// expand array if necessary
	if (game.numShips == game.lenShips)
	{
		game.lenShips += 100;
		game.ships = (Ship*) realloc(game.ships, game.lenShips * sizeof(Ship));
	}

	Ship* s = &game.ships[game.numShips];
	game.numShips++;
	
	s->position = position;
	s->velocity = vec2f(0.f, 0.f);
	s->type = type;
	s->team = team;
	s->health = 100;
	return s;
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
	// it is important to seed the planets individually first, so the seeds can be used without interference
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
		planet->position = vec2f(cos(a) * r, sin(a) * r);
		r = r + game.galaxyRadius / game.numPlanets;
		a = a + planet->seed;
		// size
		planet->radius = (float) 5 + (rand() % 5);
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

	spawnShip(vec2f(25.f,-25.f),0,0);
}

void tickGame(float step)
{
	// advance timers and process production values
	game.gameAge += step;

	if ((int)game.gameAge != (int)(game.gameAge+step))
	{
		spawnShip(vec2f(rand()%50, rand()%50), 0, 0);
	}

	// accumulate ship presence
	float presenceSum[3];
	presenceSum[0] = 0;
	presenceSum[1] = 0;
	presenceSum[2] = 0;
	for (int i = 0; i < game.numPlanets; i++)
	{
		Planet* p = &(game.planets[i]);
		p->shipPresence[0] = 0;
		p->shipPresence[1] = 0;
		p->shipPresence[2] = 0;
		for (int s = 0; s < game.numShips; s++)
		{
			float r = veclen(vecsub(game.ships[s].position, p->position));
			p->shipPresence[game.ships[s].type] += min(1.f / r, 1.f); //r < p->radius * 4.f ? 1.f : 0.f;
			presenceSum[game.ships[s].type] += p->shipPresence[game.ships[s].type];
		}
	}

	for (int i = 0; i < game.numShips; i++)
	{
		Ship* s = &game.ships[i];
		Vector2f force;
		force = vec2f(0.f, 0.f);

		Vector2f planetAttraction = vec2f(0.f, 0.f);
		Planet bestPlanet;
		float bestFactor = 10000.f;
		for (int j = 0; j < game.numPlanets; j++)
		{
			float r = veclen(vecsub(game.planets[j].position, s->position));
			float f = sqrt(sqrt(r)) * game.planets[j].shipPresence[s->type];
			if (f < bestFactor)
			{
				bestPlanet = game.planets[j];
				bestFactor = f;
			}

			// planet evasion
			// if (r < game.planets[j].radius * 1.1f)
			// {
				// force = vecadd(force, vecscale(normalize(vecsub(s->position, game.planets[j].position)), 
				// 	max(min(2.f * game.planets[j].radius - r, 10), 0)));
			// }
		}
		planetAttraction = normalize(vecsub(bestPlanet.position, s->position));

		Vector2f boid = vec2f(0.f, 0.f);
		Vector2f positionSum = vec2f(0.f, 0.f);
		int numPeers;
		for (int j = 0; j < game.numShips; j++)
		{
			float r = veclen(vecsub(s->position, game.ships[j].position));
			
			// seperation
			if (r < 2.f)
			{
				force = vecadd(force, normalize(vecsub(s->position, game.ships[j].position)));
			}

			// alignment
			if (r < 1.f)
			{
				force = vecadd(force, vecscale(normalize(vecsub(game.ships[j].velocity, s->velocity)), 2.f));
			}

			// cohesion pt. 1
			if (r < 5.f)
			{
				numPeers++;
				positionSum = vecadd(positionSum, game.ships[j].position);
			}
		}

		// cohesion pt. 2
		force = vecadd(force, normalize(vecsub(vecscale(positionSum, 1.f/numPeers), s->position)));


		force = vecadd(force, vecscale(normalize(planetAttraction), 3.f));

		// force = vecadd(force, vec2f(sin(game.gameAge),cos(game.gameAge)));

		printf("planetAttraction = (%f, %f)\n", planetAttraction.x, planetAttraction.y);
		s->force = force;
		s->velocity = normalize(vecadd(s->velocity, vecscale(force, step)));
		s->position = vecadd(s->position, vecscale(s->velocity, step * 5.f));
	}
	// normalize velocity and adjust it as per type
}

void renderGame()
{
	
	// Set up projection matrix for game world
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	glOrtho(-100.f * game.aspectRatio, 100.f * game.aspectRatio, -100.f, 100.f, -1.f, 100.f);
	// Reset camera
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();

	// Clear color buffer
	glClear( GL_COLOR_BUFFER_BIT );
	
	// Render background
	glColor3f(0.1f, 0.1f, 0.1f);
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
		glColor3f(game.planets[i].shipPresence[1]/1.f, game.planets[i].shipPresence[0]/100.f, game.planets[i].shipPresence[2]);
		glPushMatrix();
			glTranslatef(game.planets[i].position.x, game.planets[i].position.y, 0);
			glBegin( GL_QUADS );
				glVertex2f( -r, -r );
				glVertex2f(  r, -r );
				glVertex2f(  r,  r );
				glVertex2f( -r,  r );
			glEnd();
		glPopMatrix();
	}

	for (int i = 0; i < game.numShips; i++)
	{
		glColor3f(1.f, 0.f, 0.f);
		glPushMatrix();
			glTranslatef(game.ships[i].position.x, game.ships[i].position.y, 0);
			glBegin( GL_POINTS );
				glVertex2f(0, 0);
			glEnd();
			glBegin( GL_LINES );
				// glVertex2f(0, 0);
				// glVertex2f(game.ships[i].velocity.x*2, game.ships[i].velocity.y*2);
			glEnd();
		glPopMatrix();
	}

	// Render UI
}