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

#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)>(b))?(a):(b))

struct Tile
{
	// type 0 = none
	//      1 = HQ(not buildable)
	//      2 = mine
	//      3 = power plant
	//      4 = farm
	//      5 = shipyard(fighter)
	//      6 = shipyard(cruiser)
	//      7 = shipyard(bomber)
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
	int shipsToSpawn[3];
	float countdown;
	Vectorf spawnAreaP1;
	Vectorf spawnAreaP2;
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
};

Game game;

void clearGame()
{
	game.speedModifier = 1.f;
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

	Planet* p = &game.planets[planetPopup->data];
	p->tiles[source->data].buildingType = buildingSelector->data;
	source->texture = &game.textures[buildingSelector->data];
}

void createUI()
{
	// root element fills the whole ui area
	game.gui.position = vecf(0.f, 0.f);
	game.gui.size = vecf(1.f, 1.f);
	game.gui.visible = true;

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

	printf("1st name %s %s\n", game.gui.children[0].name, squareCenter->name);

	squareCenter->position = vecf(xpadding / 2.f, ypadding / 2.f);
	squareCenter->size = vecf(1.f - xpadding, 1.f - ypadding);
	printf("%s %f %f - %f %f\n", squareCenter->name,
		squareCenter->position.x, squareCenter->position.y, 
		squareCenter->size.x, squareCenter->size.y);

	printf("Window resized %d %d\n", game.window_width, game.window_height);
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
}

void handleMouseButtons(uint8_t button, int32_t x, int32_t y)
{
	float fx = (float) x / game.window_width;
	float fy = (float) y / game.window_height;
	printf("%f %f\n", fx, fy);
	UIElement* elem = getElementAt(&game.gui, fx, fy);
	if (elem)
	{
		if (elem->onClick)
		{
			elem->onClick(elem);
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
		printf("%f %f - %f %f = %f?\n", fx, fy, game.planets[i].position.x, game.planets[i].position.y, game.planets[i].radius);
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
		planet->team = i < 1 ? 0 : 1;
		srand(game.planets[i].seed);
		// positioning
		planet->position = vecscale(vecf(cos(a), sin(a)), sqrt(r / game.galaxyRadius) * game.galaxyRadius);
		r = r + game.galaxyRadius / game.numPlanets;
		a = a + planet->seed;
		// size
		planet->radius = (float) 5 + (rand() % 5);
		planet->numTiles = round(planet->radius);
		planet->tiles = (Tile*) malloc(sizeof(Tile) * planet->numTiles);
		for (int t = 0; t < planet->numTiles; t++)
		{
			Tile* tile = &planet->tiles[t];
			tile->buildingType = 0;
			tile->buildingLevel = 0;
		}
	}

	game.currentWave.spawnAreaP1 = vecf(-galaxyRadius, galaxyRadius);
	game.currentWave.spawnAreaP2 = vecf( galaxyRadius, galaxyRadius);
	game.currentWave.shipsToSpawn[0] = 5;
	game.currentWave.countdown = 3.f;

	spawnShip(vecf(25.f,-25.f),0,0);
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
		Ship* s = spawnShip(vecf(cos(a)*game.planets[0].radius, sin(a)* game.planets[0].radius), 0, 0);
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
			if (game.ships[s].team != p->team) continue;
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

			Vectorf boid = vecf(0.f, 0.f);
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
				force = vecadd(force, normalize(vecsub(vecscale(positionSum, 1.f/numPeers), s->position)));

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

		s->force = force;
		s->velocity = normalize(vecadd(s->velocity, vecscale(force, step * 0.5f)));
		s->position = vecadd(s->position, vecscale(s->velocity, step * 5.f));
	}
	// normalize velocity and adjust it as per type
}

void loadAssets()
{
	game.numTextures = 8;
	game.textures = (Texture*) malloc(game.numTextures * sizeof(Texture));

	game.textures[0] = loadTexture("assets" PATH_SEPARATOR "empty.png", "empy");
	game.textures[1] = loadTexture("assets" PATH_SEPARATOR "hq.png", "headquarter");
	game.textures[2] = loadTexture("assets" PATH_SEPARATOR "plant.png", "powerplant");
	game.textures[3] = loadTexture("assets" PATH_SEPARATOR "farm.png", "farm");
	game.textures[4] = loadTexture("assets" PATH_SEPARATOR "mine.png", "mine");
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