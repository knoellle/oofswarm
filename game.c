#include <stdio.h>
#include <stdbool.h> 
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <GL/glu.h>
#include <stdlib.h>
#include <math.h>
#include "vectors.c"
#include "textures.c"
#include "oofgui.c"

#define PI 3.14159265358979323846

#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)>(b))?(a):(b))

#define rsc_energy 0
#define rsc_sbm 1
#define rsc_food 2

struct Tile
{
	// type 0 = none
	//      1 = HQ(not buildable)
	//      2 = mine
	//      3 = power plant
	//      4 = farm
	//      5 = shipyard(fighter)
	//      6 = shipyard(bomber)
	//      7 = shipyard(cruiser)
	int   buildingType;
	int   buildingLevel;
};

struct Planet
{
	Vectorf position;
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
	float energyUsage;
	float buildCost;
};

struct Ship
{
	Vectorf position;
	Vectorf velocity;
	Vectorf force;
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
	float shipsToSpawn[3]; // float to make it easily scalable, i.e. x * 1.1
	float countdown;
	Vectorf spawnAreaP1;
	Vectorf spawnAreaP2;

	int waveNumber;
};

struct Game
{
	float gameAge;
	int seed;
	int galaxyRadius;

	Planet *planets;
	int numPlanets;

	ShipClass shipClasses[3];
	int buildingPrices[8];

	Ship *ships;
	int numShips;
	int lenShips;

	Wave nextWave;
	Wave currentWave;

	float resources[3];

	// ratio of height and width of the window
	float aspectRatio;
	int window_width;
	int window_height;

	Vectorf cameraShift;
	float cameraZoom;

	UIElement gui;

	Texture *textures;
	int numTextures;

	// 0: none
	// 1: visual indicators
	// 2: console outputs
	int debuglevel;

	float speedModifier;
	float leftoverStep;
	bool steplimiting;
};

Game game;

void clearGame()
{
	game.speedModifier = 1.f;
	game.leftoverStep = 0.0f;
	game.steplimiting = false;
	game.seed = 0;
	game.galaxyRadius = 0;
	game.cameraShift = vecf(0.f, 0.f);
	game.cameraZoom = 1.f;
	if (game.numPlanets > 0)
	{
		for (int i = 0; i < game.numPlanets; i++)
		{
			free(game.planets[i].tiles);
		}
		free(game.planets);
	}
	game.numShips = 0;
	game.numPlanets = 0;
}

Ship* spawnShip(Vectorf position, int type, int team)
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
	s->velocity = vecf(0.f, 0.f);
	s->type = type;
	s->values = &game.shipClasses[type];
	s->team = team;
	s->health = s->values->baseHealth;
	s->target = NULL;
	s->weaponTimer = 0.f;
	// printf("spawning ship #%d\n", game.numShips);
	return s;
}

Texture* findTexture(char* name)
{
	for (int i = 0; i < game.numTextures; i++)
	{
		if(strcmp(name, game.textures[i].name) == 0)
			return &game.textures[i];
	}
	return NULL;
}

void buttonBuildingSelect(UIElement* source)
{
	UIElement* buildingSelector = getElementByName(&game.gui, "buildingSelector");
	buildingSelector->data = source->data;
	for (int i = 0; i < buildingSelector->numChildren; i++)
	{
		buildingSelector->children[i].borderColor = vecf(0.f, 0.f, 0.f, 1.f);
		// buildingSelector->children[i].faceColor = vecf(0.1f, 0.1f, 0.1f, 0.f);
	}
	source->borderColor.z = 0.9;
	// source->faceColor.w = 1.f;
}

void buttonTileClick(UIElement* source)
{
	UIElement* buildingSelector = getElementByName(&game.gui, "buildingSelector");
	UIElement* planetPopup = getElementByName(&game.gui, "planetPopup");

	if (game.resources[rsc_sbm] >= game.buildingPrices[buildingSelector->data])
	{
		game.resources[rsc_sbm] -= game.buildingPrices[buildingSelector->data];
		Planet* p = &game.planets[planetPopup->data];
		p->team = 0;
		p->tiles[source->data].buildingType = buildingSelector->data;
		source->texture = &game.textures[buildingSelector->data];
	}
}

void createUI()
{
	// root element fills the whole ui area
	game.gui.position = vecf(0.f, 0.f);
	game.gui.size = vecf(1.f, 1.f);
	game.gui.visible = true;
	game.gui.enabled = true;

	UIElement* squareCenter = addChild(&game.gui);
	strcpy(squareCenter->name, "squareCenter");

	UIElement* planetPopup = addChild(squareCenter);
	strcpy(planetPopup->name, "planetPopup");
	planetPopup->position = vecf(0.1f, 0.1f);
	planetPopup->size = vecf(0.8f, 0.8f);
	planetPopup->borderColor = vecf(0.f, 0.8f, 0.f, 1.f);
	planetPopup->faceColor = vecf(0.f, 0.f, 0.f, 0.4f);
	planetPopup->visible = false;

	// planet tiles
	for (int i = 0; i < 20; i++)
	{
		UIElement* tile = addChild(planetPopup);
		int x = i % 5;
		int y = i / 5;
		tile->data = i;
		tile->size = vecf(0.15f, 0.15f);
		tile->position = vecf(0.025f + x * 0.2f, 0.025f + y * 0.2f);
		tile->borderColor = vecf(0.f, 0.f, 0.f, 1.f);
		tile->faceColor = vecf(1.f, 1.f, 1.f, 1.f);
		tile->texture = findTexture("powerplant");
		tile->onClick = &buttonTileClick;
	}

	UIElement* buildingSelector = addChild(planetPopup);
	strcpy(buildingSelector->name, "buildingSelector");
	buildingSelector->position = vecf(0.025f, 0.83f);
	buildingSelector->size = vecf(0.95f, 0.15f);
	buildingSelector->faceColor = vecf(1.f, 0.7, 0.f, 0.8f);

	// building selectors
	for (int i = 2; i < 8; i++)
	{
		UIElement* tile = addChild(buildingSelector);
		tile->data = i;
		int x = i - 2;
		tile->size = vecf(0.15f, 0.8f);
		tile->position = vecf(0.0251f + x * 0.16f, 0.1f);
		tile->borderColor = vecf(0.f, 0.f, 0.f, 1.f);
		tile->faceColor = vecf(1.f, 1.f, 1.f, 1.f);
		tile->texture = &game.textures[i];
		tile->onClick = &buttonBuildingSelect;
	}
}

void resizeWindow(SDL_Event event)
{
	game.window_width = event.window.data1;
	game.window_height = event.window.data2;
	if (game.window_height == 0)
		game.window_height = 1;
	game.aspectRatio = (float) game.window_width / game.window_height;
	glViewport(0, 0, game.window_width, game.window_height);

	float xpadding = (float) (game.window_width - min(game.window_width, game.window_height)) / game.window_width;
	float ypadding = (float) (game.window_height - min(game.window_width, game.window_height)) / game.window_height;

	UIElement* squareCenter = getElementByName(&game.gui, "squareCenter");
	if (!squareCenter)
	{
		printf("Couldn't find squareCenter!\n");
		return;
	}

	//printf("1st name %s %s\n", game.gui.children[0].name, squareCenter->name);

	squareCenter->position = vecf(xpadding / 2.f, ypadding / 2.f);
	squareCenter->size = vecf(1.f - xpadding, 1.f - ypadding);
	//printf("%s %f %f - %f %f\n", squareCenter->name,
	//	squareCenter->position.x, squareCenter->position.y, 
	//	squareCenter->size.x, squareCenter->size.y);

	printf("Window resized %d %d\n", game.window_width, game.window_height);
}

void handleKeys( unsigned char key, int x, int y )
{
	if (key == SDL_SCANCODE_KP_PLUS)
	{
		game.speedModifier *= 2.f;
		if (game.speedModifier > 32.f)
			game.speedModifier = 32.f;
		printf("speedModifier is now %f\n", game.speedModifier);
	}
	if (key == SDL_SCANCODE_KP_MINUS)
	{
		game.speedModifier /= 2.f;
		printf("speedModifier is now %f\n", game.speedModifier);
	}
	if (key == SDL_SCANCODE_S)
	{
		game.currentWave.shipsToSpawn[0] = 50;
		game.currentWave.shipsToSpawn[1] = 50;
		game.currentWave.shipsToSpawn[2] = 50;
	}
	if (key == SDL_SCANCODE_D)
	{
		// for (int i = 0; i < game.numShips; ++i)
		// {
		// 	if (game.ships[i].target != NULL)
		// 	{
		// 		printf("p%d h%f l%f %f\n", game.ships[i].target, game.ships[i].target->health,
		// 			game.ships[i].target->position.x, game.ships[i].target->position.y);
		// 	}
		// }
		for (int i = 0; i < game.numPlanets; ++i)
		{
			printf("%d %f %f %f\n", game.planets[i].team, game.planets[i].shipPresence[0], game.planets[i].shipPresence[1], game.planets[i].shipPresence[2]);
		}
	}
	if (key == SDL_SCANCODE_SPACE)
	{
		game.speedModifier *= -1.f;
	}
	if (key == SDL_SCANCODE_ESCAPE)
	{
		UIElement* planetPopup = getElementByName(&game.gui, "planetPopup");
		planetPopup->visible = false;
	}
	if (key == SDL_SCANCODE_F5)
	{
		game.steplimiting = !game.steplimiting;
	}
}

void handleMouseButtons(uint8_t button, int32_t x, int32_t y)
{
	float fx = (float) x / game.window_width;
	float fy = (float) y / game.window_height;
	//printf("%f %f\n", fx, fy);
	UIElement* elem = getElementAt(&game.gui, fx, fy);
	if (elem)
	{
		if (elem->onClick)
		{
			if (elem->enabled)
			{
				elem->onClick(elem);
			}
			return;
		}
	}

	UIElement* planetPopup = getElementByName(&game.gui, "planetPopup");
	planetPopup->visible = false;
	fx -= 0.5f;
	fy -= 0.5f;
	fx *= 200 * game.aspectRatio / game.cameraZoom;
	fy *= -200 / game.cameraZoom;
	for (int i = 0; i < game.numPlanets; i++)
	{
		//printf("%f %f - %f %f = %f?\n", fx, fy, game.planets[i].position.x, game.planets[i].position.y, game.planets[i].radius);
		if (veclen(vecsub(game.planets[i].position, vecf(fx, fy))) <= game.planets[i].radius)
		{
			for (int j = 0; j < 20; j++)
			{
				if (j < game.planets[i].numTiles)
				{
					planetPopup->children[j].texture = &game.textures[game.planets[i].tiles[j].buildingType];
					planetPopup->children[j].visible = true;
				}
				else
					planetPopup->children[j].visible = false;
			}
			planetPopup->visible = true;
			planetPopup->data = i;
			break;
		}
	}
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
		planet->team = 1;
		srand(game.planets[i].seed);
		// positioning
		planet->position = vecscale(vecf(cos(a), sin(a)), sqrt(r / game.galaxyRadius) * game.galaxyRadius);
		r = r + game.galaxyRadius / game.numPlanets;
		a = a + PI * (1 + r/game.galaxyRadius/game.numPlanets*2); // 2 arm spiral galaxy
		printf("Planet seed: %d\n", planet->seed);
		// size
		planet->radius = (float) 5 + (rand() % 15);
		planet->numTiles = round(planet->radius);
		planet->tiles = (Tile*) malloc(sizeof(Tile) * planet->numTiles);
		for (int t = 0; t < planet->numTiles; t++)
		{
			Tile* tile = &planet->tiles[t];
			tile->buildingType = 0;
			tile->buildingLevel = 0;
		}
	}

	// set up starting planet
	Planet* p = &game.planets[0];
	p->team = 0;
	p->tiles[p->numTiles/2].buildingType = 1;
	p->tiles[p->numTiles/2].buildingLevel = 1;

	game.nextWave.spawnAreaP1 = vecf(-galaxyRadius/10.f, galaxyRadius);
	game.nextWave.spawnAreaP2 = vecf( galaxyRadius/10.f, galaxyRadius);
	game.nextWave.shipsToSpawn[0] = 15;
	game.nextWave.shipsToSpawn[1] = 5;
	game.nextWave.shipsToSpawn[2] = 1;
	game.nextWave.countdown = 5.f;

	game.resources[rsc_energy] = 100;
	game.resources[rsc_sbm] = 450;
	game.resources[rsc_food] = 300;

	spawnShip(vecf(25.f,-25.f),0,0)->velocity=vecf(0.1,0.0);
}

void tickGame(float step, bool fixedStepSize = false, float stepsize = 0.016f) // 1/0.016 = 60 fps
{
	if (game.speedModifier <= 0.f)
	{
		return;
	}
	step *= game.speedModifier;

	if (fixedStepSize)
	{
		step += game.leftoverStep;
		if (step/stepsize > 5.f) // don't do more than 5 steps per frame
		{
			printf("Overstepping! Dropping some steps!\n");
			step = stepsize * 5.f;
		}
		while (step >= stepsize)
		{
			tickGame(stepsize);
			step -= stepsize;
		}
		game.leftoverStep = step;
		return;
	}

	// "remove" dead ships(and count ships)
	int enemy_shipcount = 0;
	int player_shipcount = 0;
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
		} else {
			if (game.ships[i].team == 1)
				enemy_shipcount++;
			if (game.ships[i].team == 0)
				player_shipcount++;
		}
	}

	if (player_shipcount == 0)
	{
		printf("\n\n\n==================\n\nYou lost...\n\n==================\n\n\n\n");
		game.speedModifier = 0.f;
	}

	// advance wave
	if (enemy_shipcount == 0 && game.currentWave.countdown < 0.f)
	{
		game.currentWave = game.nextWave;
		for (int i = 0; i < 3; ++i)
		{
			game.nextWave.shipsToSpawn[i] *= 1.5;
		}
	}

	// tick wave
	game.currentWave.countdown -= step;
	if (game.currentWave.countdown < 0.f)
	{
		for (int i = 0; i < 3; i++)
		{
			if (game.currentWave.shipsToSpawn[i] > 0)
			{
				game.currentWave.shipsToSpawn[i] -= 1;
				Ship* s = spawnShip(randomBetween(game.currentWave.spawnAreaP1, game.currentWave.spawnAreaP2), i, 1);
				// printf("%f %f\n", s->position.x, s->position.y);
			}
		}
	}

	Vectorf resource_delta = vecf(0, 0, 0);

	// tick planets
	bool tick = false; // this is only true during one cycle per second and used to determine when to spawn ships
	if ((int)game.gameAge != (int)(game.gameAge+step))
	{
		tick = true;
		//printf("tick! %d %d %f\n", (int)game.gameAge, (int)(game.gameAge+step), game.gameAge+step);
	}
	for (int i = 0; i < game.numPlanets; i++)
	{
		for (int j = 0; j < game.planets[i].numTiles; j++)
		{
			switch (game.planets[i].tiles[j].buildingType)
			{
				case 0: // ignore, empty tile
					break;
				case 1: // HQ
					resource_delta.x += 3;
					resource_delta.y += 3;
					resource_delta.z += 3;
					break;
				case 2: // mine
					// deactivate building if there's not enough energy and its not producing energy
					if (game.resources[rsc_energy] <= 0)
							continue;
					resource_delta.y += 3;
					resource_delta.x -= 2;
					break;
				case 3: // power plant
					resource_delta.x += 3;
					break;
				case 4: // farm
					// deactivate building if there's not enough energy and its not producing energy
					if (game.resources[rsc_energy] <= 0)
							continue;
					resource_delta.z += 3;
					resource_delta.x -= 1;
					break;
				case 5: // shipyard(fighter), produces 1 ship every 5 seconds
					// deactivate building if there's not enough energy and its not producing energy
					if (game.resources[rsc_energy] <= 0)
							continue;
					if (tick && ((int)game.gameAge) % 2 == 0 && game.planets[i].shipPresence[0] < game.planets[i].radius)
					{
						if (game.resources[rsc_sbm] < game.shipClasses[0].buildCost)
							break;
						game.resources[rsc_sbm] -= game.shipClasses[0].buildCost;
						float a = (rand() % 360) / (180.f/3.41f);
						Ship* s = spawnShip(vecadd(vecf(cos(a)*game.planets[i].radius, sin(a)* game.planets[i].radius), game.planets[i].position), 0, 0);
						s->velocity = normalize(vecadd(vecsub(s->position, game.planets[i].position), randomBetween(vecf(-0.1f, -0.1f), vecf(0.1f, 0.1f))));
					}
					break;
				case 6: // shipyard(bomber), produces 1 ship every 15 seconds
					// deactivate building if there's not enough energy and its not producing energy
					if (game.resources[rsc_energy] <= 0)
							continue;
					if (tick && ((int)game.gameAge) % 10 == 0 && game.planets[i].shipPresence[1] < game.planets[i].radius)
					{
						if (game.resources[rsc_sbm] < game.shipClasses[1].buildCost)
							break;
						game.resources[rsc_sbm] -= game.shipClasses[1].buildCost;
						float a = (rand() % 360) / (180.f/3.41f);
						Ship* s = spawnShip(vecadd(vecf(cos(a)*game.planets[i].radius, sin(a)* game.planets[i].radius), game.planets[i].position), 1, 0);
						s->velocity = normalize(vecadd(vecsub(s->position, game.planets[i].position), randomBetween(vecf(-0.1f, -0.1f), vecf(0.1f, 0.1f))));
					}
					break;
				case 7: // shipyard(cruiser), produces 1 ship every 60 seconds
					// deactivate building if there's not enough energy and its not producing energy
					if (game.resources[rsc_energy] <= 0)
							continue;
					if (tick && ((int)game.gameAge) % 30 == 0 && game.planets[i].shipPresence[2] < game.planets[i].radius)
					{
						if (game.resources[rsc_sbm] < game.shipClasses[2].buildCost)
							break;
						game.resources[rsc_sbm] -= game.shipClasses[2].buildCost;
						float a = (rand() % 360) / (180.f/3.41f);
						Ship* s = spawnShip(vecadd(vecf(cos(a)*game.planets[i].radius, sin(a)* game.planets[i].radius), game.planets[i].position), 2, 0);
						s->velocity = normalize(vecadd(vecsub(s->position, game.planets[i].position), randomBetween(vecf(-0.1f, -0.1f), vecf(0.1f, 0.1f))));
					}
					break;
			}
		}
	}

	// advance timers and process production values
	game.gameAge += step;

	// accumulate ship presence
	float presenceSum[3];
	presenceSum[0] = 0;
	presenceSum[1] = 0;
	presenceSum[2] = 0;
	for (int i = 0; i < game.numPlanets; i++)
	{
		Planet* p = &(game.planets[i]);
		if (p->team == 0)
		{
			resource_delta.z -= sqrt(p->radius); // subtract some food
		}
		p->shipPresence[0] = 0;
		p->shipPresence[1] = 0;
		p->shipPresence[2] = 0;
		for (int s = 0; s < game.numShips; s++)
		{
			// if (game.ships[s].team != p->team) continue;
			float r = veclensqr(vecsub(game.ships[s].position, p->position));
			if (r > 0.f)
				p->shipPresence[game.ships[s].type] += min(1.f / r, 1.f) * (2 - game.ships[s].team * 3); //r < p->radius * 4.f ? 1.f : 0.f;
		}
		presenceSum[0] += p->shipPresence[0];
		presenceSum[1] += p->shipPresence[1];
		presenceSum[2] += p->shipPresence[2];
	}

	// move ships
	for (int i = 0; i < game.numShips; i++)
	{
		Ship* s = &game.ships[i];
		if (s->team == 0)
			resource_delta.x -= s->values->energyUsage; // subtract some energy
		Vectorf force;
		force = vecf(0.f, 0.f);
		if (s->target == NULL) // cruise mode
		{
			Vectorf planetAttraction = vecf(0.f, 0.f);
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

			Vectorf positionSum = vecf(0.f, 0.f);
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
				force = vecadd(force, vecscale(normalize(vecsub(vecscale(positionSum, 1.f/numPeers), s->position)), 0.5f));

			force = vecadd(force, vecscale(normalize(planetAttraction), 3.f));
		} else { // target mode
			// continue;
			Vectorf relpos = vecsub(s->target->position, s->position);

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

		s->force = force; // copy for debugging during the render cycle
		s->velocity = normalize(vecadd(s->velocity, vecscale(force, step * s->values->acceleration)));
		// normalize velocity and adjust it as per type
		s->position = vecadd(s->position, vecscale(s->velocity, step * s->values->speed));
	}

	Vectorf resource_delta_scaled = vecscale(resource_delta, step); // make sure to advance the counters only by a fraction based on the time passeds

	game.resources[rsc_energy] += resource_delta_scaled.x;
	game.resources[rsc_sbm] += resource_delta_scaled.y;
	game.resources[rsc_food] += resource_delta_scaled.z;

	// update building selectors to reflect whether they can be purchased with the current amount of funds
	UIElement* buildingSelector = getElementByName(&game.gui, "buildingSelector");
	for (int i = 0; i < buildingSelector->numChildren; i++)
	{
		UIElement* elem = &buildingSelector->children[i];
		if (game.resources[rsc_sbm] >= game.buildingPrices[elem->data])
		{
			elem->enabled = true;
			elem->faceColor = vecf(1.f, 1.f, 1.f, 1.f);
		} else {
			elem->enabled = false;
			elem->faceColor = vecf(0.5f, 0.5f, 0.5f, 1.f);
		}
	}

	printf("Resources: %f %f %f, delta %f %f %f\n", game.resources[0], game.resources[1], game.resources[2], resource_delta.x, resource_delta.y, resource_delta.z);
}

void loadAssets()
{
	game.numTextures = 8;
	game.textures = (Texture*) malloc(game.numTextures * sizeof(Texture));

	game.textures[0] = loadTexture("assets" PATH_SEPARATOR "empty.png", "empty");
	game.textures[1] = loadTexture("assets" PATH_SEPARATOR "hq.png", "headquarter");
	game.textures[2] = loadTexture("assets" PATH_SEPARATOR "mine.png", "mine");
	game.textures[3] = loadTexture("assets" PATH_SEPARATOR "plant.png", "powerplant");
	game.textures[4] = loadTexture("assets" PATH_SEPARATOR "farm.png", "farm");
	game.textures[5] = loadTexture("assets" PATH_SEPARATOR "Shipyard1.png", "headquarter");
	game.textures[6] = loadTexture("assets" PATH_SEPARATOR "Shipyard2.png", "powerplant");
	game.textures[7] = loadTexture("assets" PATH_SEPARATOR "Shipyard3.png", "farm");
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
	glDisable(GL_CULL_FACE);

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
		float r = game.planets[i].radius/2;

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
			glColor3f(0.f, 0.f, 1.f * (s->health/s->values->baseHealth));
		} else {
			glColor3f(1.f * (s->health/s->values->baseHealth), 0.f, 0.f);
		}
		glPushMatrix();
			glTranslatef(s->position.x, s->position.y, 0);
			switch (s->type)
			{
				case 0:
					glBegin( GL_POINTS );
						glVertex2f(0, 0);
					glEnd();
					break;
				case 1:
					glBegin( GL_QUADS );
						glVertex2f(-0.2f, -0.2f);
						glVertex2f( 0.2f, -0.2f);
						glVertex2f( 0.2f,  0.2f);
						glVertex2f(-0.2f,  0.2f);
					glEnd();
					break;
				case 2:
					glBegin( GL_POLYGON );
						for (int a = 0; a < 360; a+=36)
							glVertex2f(cos(a / (180.f/3.41f)), sin(a / (180.f/3.41f)));
					glEnd();
					break;
			}

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
						Vectorf relpos = vecsub(s->target->position, s->position);
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

	// Set up projection matrix for the ui
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	glOrtho(0.f, 1.f, 1.f, 0.f, -1.f, 1.f); // * game.aspectRatio

	// Reset camera
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	renderElement(&game.gui);
	glDisable(GL_BLEND);
}