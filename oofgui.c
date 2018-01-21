#include <stdio.h>
#include <stdbool.h> 
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <GL/glu.h>
#include <stdlib.h>
#include <math.h>

struct UIElement
{
	char name[100];
	Vectorf position;
	Vectorf size;
	void (*onClick)(UIElement* source);
	int data;

	UIElement *parent;
	UIElement *children;
	int numChildren;

	Vectorf borderColor;
	Vectorf faceColor;
	Texture* texture;

	bool visible;
};

UIElement* addChild(UIElement* root)
{
	printf("spawning child element\n");
	root->numChildren += 1;
	UIElement* newarray = (UIElement*) realloc(root->children, root->numChildren * sizeof(UIElement));
	if (newarray)
	{
		root->children = newarray;
	}
	else
	{
		printf("Couldn't increase array size, aborting element creation.\n");
		return NULL;
	}
	UIElement* elem = &root->children[root->numChildren - 1];
	strcpy(elem->name, "");
	elem->parent = root;
	elem->numChildren = 0;
	elem->children = NULL;
	elem->position = vecf(0.f, 0.f);
	elem->size = vecf(0.f, 0.f);
	elem->texture = NULL;
	elem->onClick = NULL;
	elem->visible = true;
	elem->borderColor = vecf(0.f, 0.f, 0.f, 0.f);
	elem->faceColor = vecf(0.f, 0.f, 0.f, 0.f);
	return elem;
}

UIElement* getElementAt(UIElement* root, float x, float y)
{
	if (!root->visible)
		return NULL;
	x -= root->position.x;
	y -= root->position.y;
	x /= root->size.x;
	y /= root->size.y;
	printf("%f %f\n", x, y);
	if (x < 0.f || x > 1.f || y < 0.f || y > 1.f)
	{
		return NULL;
	}
	for (int i = 0; i < root->numChildren; i++)
	{
		UIElement* elem = getElementAt(&root->children[i], x, y);
		if (elem)
			return elem;
	}
	return root;
}

UIElement* getElementByName(UIElement* root, char* name)
{
	// printf("comparing %s and %s %d\n", root->name, name, strcmp(root->name, name));
	if (strcmp(root->name, name) == 0)
		return root;
	for (int i = 0; i < root->numChildren; i++)
	{
		UIElement* elem = getElementByName(&root->children[i], name);
		if (elem)
			return elem;
	}
	return NULL;
}

void renderElement(UIElement* root)
{
	if (!root->visible)
		return;
	glPushMatrix();
	glTranslatef(root->position.x, root->position.y, 0.f);
	glScalef(root->size.x, root->size.y, 1.f);

	// render element
	glColor4fv(&root->borderColor.x);
	glBegin(GL_LINE_LOOP);
		glVertex2f(0.f, 0.f);
		glVertex2f(1.f, 0.f);
		glVertex2f(1.f, 1.f);
		glVertex2f(0.f, 1.f);
	glEnd();
	
	bindTexture(root->texture);
	glColor4fv(&root->faceColor.x);
	glBegin(GL_QUADS);
		glVertex2f(0.f, 0.f); glTexCoord2f(1.f, 0.f);
		glVertex2f(1.f, 0.f); glTexCoord2f(1.f, 1.f);
		glVertex2f(1.f, 1.f); glTexCoord2f(0.f, 1.f);
		glVertex2f(0.f, 1.f); glTexCoord2f(0.f, 0.f);
	glEnd();
	unbindTexture(root->texture);
	
	// render child elements
	for (int i = 0; i < root->numChildren; i++)
	{
		renderElement(&root->children[i]);
	}

	glPopMatrix();
}