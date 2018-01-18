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
	int team;

	float shipPresence[3];
};

struct ShipClass
{
	float acceleration;
	float speed;
	float damageModifiers[3];
	float sensorRange;
	float weaponRange;
	float fireSpeed;
	float baseHealth;
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
	ShipClass* values;
	// team 0 = player
	//      1 = enemy
	int team;
	float health;
	Ship* target;
	float weaponTimer;
};

struct Wave
{
	int shipsToSpawn[3];
	float countdown;
	Vector2f spawnAreaP1;
	Vector2f spawnAreaP2;
};

struct Game
{
	float gameAge;
	int seed;
	int galaxyRadius;

	Planet *planets;
	int numPlanets;

	ShipClass shipClasses[3];

	Ship *ships;
	int numShips;
	int lenShips;

	Wave currentWave;

	// ratio of height and width of the window
	float aspectRatio;

	Vector2f cameraShift;
	float cameraZoom;

	// 0: none
	// 1: visual indicators
	// 2: console outputs
	int debuglevel;
	float speedModifier;
};

Game game;

void clearGame()
{
	game.speedModifier = 1.f;
	game.seed = 0;
	game.galaxyRadius = 0;
	game.cameraShift = vec2f(0.f, 0.f);
	game.cameraZoom = 1.f;
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
		printf("Increasing array size from %d to %d\n", game.lenShips, game.lenShips + 100);
		game.lenShips += 100;
		Ship* newarray = (Ship*) realloc(game.ships, game.lenShips * sizeof(Ship));
		if (newarray)
		{
			game.ships = newarray;
		}
		else
		{
			printf("Couldn't increase array size, aborting spawn.\n");
			return NULL;
		}

	}

	// printf("%d\n", game.ships);

	Ship* s = &game.ships[game.numShips];
	game.numShips++;

	s->position = position;
	s->velocity = vec2f(0.f, 0.f);
	s->type = type;
	s->values = &game.shipClasses[type];
	s->team = team;
	s->health = s->values->baseHealth;
	s->target = NULL;
	s->weaponTimer = 0.f;
	// printf("spawning ship #%d\n", game.numShips);
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
		planet->team = i < planets/2 ? 0 : 1;
		srand(game.planets[i].seed);
		// positioning
		planet->position = vecscale(vec2f(cos(a), sin(a)), sqrt(r / game.galaxyRadius) * game.galaxyRadius);
		r = r + game.galaxyRadius / game.numPlanets;
		a = a + planet->seed;
		// size
		planet->radius = (float) 5 + (rand() % 5);
		planet->numTiles = round(planet->radius);
		planet->tiles = (Tile*) malloc(sizeof(Tile) * planet->numTiles);
		for (int t = 0; t < planet->numTiles; t++)
		{
			Tile* tile = &planet->tiles[i];
		}
	}

	game.currentWave.spawnAreaP1 = vec2f(-50, 100);
	game.currentWave.spawnAreaP2 = vec2f( 50, 100);
	game.currentWave.shipsToSpawn[0] = 5;
	game.currentWave.countdown = 3.f;

	spawnShip(vec2f(25.f,-25.f),0,0);
}

void tickGame(float step)
{
	if (game.speedModifier <= 0.f)
	{
		return;
	}
	step *= game.speedModifier;
	// advance timers and process production values
	game.gameAge += step;

	// "remove" dead ships
	for (int i = game.numShips; i >= 0; i--)
	{
		if (game.ships[i].health < 0.f)
		{
			for (int j = 0; j < game.numShips; j++)
			{
				if (game.ships[j].target == &game.ships[i])
				{
					game.ships[j].target = NULL;
				}
			}
			game.ships[i] = game.ships[game.numShips - 1];
			game.ships[i].target = NULL;
			game.numShips--;
		}
	}

	// tick wave
	game.currentWave.countdown -= step;
	if (game.currentWave.countdown < 0.f)
	{
		for (int i = 0; i < 3; i++)
		{
			while (game.currentWave.shipsToSpawn[i] > 0)
			{
				game.currentWave.shipsToSpawn[i] -= 1;
				Ship* s = spawnShip(randomBetween(game.currentWave.spawnAreaP1, game.currentWave.spawnAreaP2), i, 1);
				// printf("%f %f\n", s->position.x, s->position.y);
			}
		}
	}

	if ((int)game.gameAge != (int)(game.gameAge+step))
	{
		float a = (rand() % 360) / (180.f/3.41f);
		Ship* s = spawnShip(vec2f(cos(a)*game.planets[0].radius, sin(a)* game.planets[0].radius), 0, 0);
		s->velocity = normalize(vecsub(s->position, game.planets[0].position));
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
			if (game.ships[i].team != p->team) continue;
			float r = veclen(vecsub(game.ships[s].position, p->position));
			if (r > 0.f)
				p->shipPresence[game.ships[s].type] += min(1.f / r, 1.f); //r < p->radius * 4.f ? 1.f : 0.f;
		}
		presenceSum[0] += p->shipPresence[0];
		presenceSum[1] += p->shipPresence[1];
		presenceSum[2] += p->shipPresence[2];
	}

	// move ships
	for (int i = 0; i < game.numShips; i++)
	{
		Ship* s = &game.ships[i];
		Vector2f force;
		force = vec2f(0.f, 0.f);
		if (s->target == NULL) // cruise mode
		{
			Vector2f planetAttraction = vec2f(0.f, 0.f);
			Planet bestPlanet;
			float bestFactor = 10000.f;
			for (int j = 0; j < game.numPlanets; j++)
			{
				float r = veclen(vecsub(game.planets[j].position, s->position));
				float f;
				if (game.planets[j].team == 0)
				{
					f = sqrt(sqrt(r)) * game.planets[j].shipPresence[s->type];
				} else {
					f = r * 1000;
				}
				if (f < bestFactor)
				{
					bestPlanet = game.planets[j];
					bestFactor = f;
				}

				// planet evasion
				if (r < game.planets[j].radius * 1.1f)
				{
					force = vecadd(force, vecscale(normalize(vecsub(s->position, game.planets[j].position)), 
						max(min(2.f * game.planets[j].radius - r, 10), 0)));
				}
			}
			planetAttraction = normalize(vecsub(bestPlanet.position, s->position));

			Vector2f boid = vec2f(0.f, 0.f);
			Vector2f positionSum = vec2f(0.f, 0.f);
			int numPeers;
			for (int j = 0; j < game.numShips; j++)
			{
				float r = veclen(vecsub(s->position, game.ships[j].position));
				if (s->team != game.ships[j].team)
				{
					if (r < s->values->sensorRange)
					{
						if (random() % 100 > 90)
						{
							s->target = &game.ships[j];
							break;
						}
					}
					continue;
				}

				// seperation
				if (r < 5.f)
				{
					force = vecadd(force, normalize(vecsub(s->position, game.ships[j].position)));
				}

				// alignment
				if (r < 2.f)
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
			if (numPeers > 0)
				force = vecadd(force, normalize(vecsub(vecscale(positionSum, 1.f/numPeers), s->position)));

			force = vecadd(force, vecscale(normalize(planetAttraction), 3.f));
		} else { // target mode
			// continue;
			Vector2f relpos = vecsub(s->target->position, s->position);

			float r = veclen(relpos);
			if (r < s->values->weaponRange)
			{
				// weapon animation
				s->weaponTimer += step * s->values->fireSpeed;
				if (s->weaponTimer > 1000.f)
					s->weaponTimer = 0.f;
				// impart damage
				s->target->health -= step * s->values->damageModifiers[s->target->type];
			}
			if (r > s->values->weaponRange || s->target->health <= 0.f)
			{
				s->target = NULL;
			}
			force = vecadd(force, normalize(relpos));
		}

		s->force = force;
		s->velocity = normalize(vecadd(s->velocity, vecscale(force, step * 0.5f)));
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

	// Render background before setting camera values (skybox)
	glColor3f(0.1f, 0.1f, 0.1f);
	glBegin( GL_QUADS );
		glVertex2f( -100.f * game.aspectRatio, -100.f );
		glVertex2f(  100.f * game.aspectRatio, -100.f );
		glVertex2f(  100.f * game.aspectRatio,  100.f );
		glVertex2f( -100.f * game.aspectRatio,  100.f );
	glEnd();

	// set camera values
	glTranslatef(game.cameraShift.x, game.cameraShift.y, 0.f);
	glScalef(game.cameraZoom, game.cameraZoom, 1.f);

	glClear( GL_DEPTH_BUFFER_BIT );
	// Render world
	glColor3f(1.f, 1.f, 1.f);
	for (int i = 0; i < game.numPlanets; i++)
	{
		float r = game.planets[i].radius;

		// Todo: set color to white and use pixelshader instead
		glColor3f(game.planets[i].team/3.f, game.planets[i].shipPresence[0]/100.f, game.planets[i].shipPresence[2]);

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
		Ship* s = &game.ships[i];
		if (s->team == 0)
		{
			glColor3f(0.f, 0.f, 1.f);
		} else {
			glColor3f(1.f, 0.f, 0.f);
		}
		glPushMatrix();
			glTranslatef(s->position.x, s->position.y, 0);
			glBegin( GL_POINTS );
				glVertex2f(0, 0);
			glEnd();
			glBegin( GL_LINES );
				if (game.debuglevel >= 1)
				{
					glColor3f(1.f, 0.f, 0.f);
					glVertex2f(0, 0);
					glVertex2f(s->velocity.x*2, s->velocity.y*2);
					glColor3f(0.f, 1.f, 0.f);
					glVertex2f(0, 0);
					glVertex2f(s->force.x*2, s->force.y*2);
				}
				if (s->target != NULL)
				{
					if ((int)(s->weaponTimer*2) % 2 > 0)
					{
						Vector2f relpos = vecsub(s->target->position, s->position);
						if (veclen(relpos) < s->values->weaponRange)
						{
							glVertex2f(0, 0);
							glVertex2f(relpos.x, relpos.y);
						}
					}
				}
			glEnd();
		glPopMatrix();
	}

	// Render UI
}