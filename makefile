CFLAGS = -lSDL2 -lGLU -lGL

.PHONY: oofswarm
oofswarm: main.c
	gcc -o oofswarm main.c $(CFLAGS)
	./oofswarm