#ifndef APPMAIN_H
#define APPMAIN_H


#include "World.h"
#include "Raytracer.h"

#if defined(__APPLE__)
    #include <GLUT/glut.h>
#else
    #include <GL/glut.h>
#endif

#define RESOLUTIONX 400
#define RESOLUTIONY 400

Raytracer g_raytracer( RESOLUTIONX , RESOLUTIONY );
World w;

#endif