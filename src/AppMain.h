#ifndef APPMAIN_H
#define APPMAIN_H


#include "World.h"
#include "Raytracer.h"

#if defined(__APPLE__)
    #include <GLUT/glut.h>
#else
    #include <GL/glut.h>
#endif

#define RESOLUTIONX 1000
#define RESOLUTIONY 1000
#define ESCAPE_KEY 27

Raytracer g_raytracer( RESOLUTIONX , RESOLUTIONY );
World w;

#endif