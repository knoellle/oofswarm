#include <stdlib.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#if defined(WIN32) || defined(_WIN32)
	#define PATH_SEPARATOR "\\"
#else
	#define PATH_SEPARATOR "/"
#endif

struct Texture
{
	GLuint handle;
	char name[100];
};

Texture loadTexture(const char *filename, const char *name)
{
	Texture tex;
	SDL_Surface* surface = IMG_Load(filename);
	strcpy(tex.name, name);
	tex.handle = 0;
	if (!surface)
		return tex;

	glGenTextures(1, &tex.handle);
	glBindTexture(GL_TEXTURE_2D, tex.handle);
	//                          mipmap level                        border
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	SDL_FreeSurface(surface);

	return tex;
}

void unloadTexture(Texture* tex)
{
	glDeleteTextures(1, &tex->handle);
}

void bindTexture(Texture* tex)
{
	if (!tex)
		return;
	glBindTexture(GL_TEXTURE_2D, tex->handle);
	glEnable(GL_TEXTURE_2D);
}

void unbindTexture(Texture* tex)
{
	if (!tex)
		return;
	glDisable(GL_TEXTURE_2D);
}