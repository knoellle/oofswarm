CFLAGS = -lSDL2 -lGLU -lGL

.PHONY: oofswarm
oofswarm: main.c
	g++ -o oofswarm main.c $(CFLAGS)
	./oofswarm
debug:
	g++ -g -o oofswarm main.c $(CFLAGS)
	gdb oofswarm