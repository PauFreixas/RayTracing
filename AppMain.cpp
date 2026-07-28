#include "windows.h"
#include "AppMain.h"

// GLUT keyboard callback: handle key presses, e.g. ESC to quit the application.
void Keyboard(unsigned char tecla, int x, int y)
{
	if (tecla == ESCAPE_KEY) 
		exit(0);
}

// GLUT display callback: draw with RayTracer when GLUT requests a redraw.
void Draw( void )
{
	g_raytracer.draw();
}

// GLUT idle callback: called when event queue is empty.
// Used here to advance the ray tracer one scanline at a time and request redisplay.
void Idle( void )
{
	if (!g_raytracer.IsDone())
	{
		g_raytracer.cast_line( w );
		glFlush(); 
		glutPostRedisplay();
	}
	else
	{
		Sleep (100);
		glFlush(); 
		glutPostRedisplay();
	}
}

int main(int argc, char** argv)
{
	//Initialize glut window
	glutInit(&argc, argv);
	glutInitDisplayMode( GLUT_SINGLE | GLUT_RGBA);
	glutInitWindowPosition(200,100);
	glutInitWindowSize( RESOLUTIONX , RESOLUTIONY );
	glutCreateWindow("Raytracer");
	glClearColor (0.0, 0.0, 0.0, 0.0);

	//Read Cornell Box from scene.sdf. If no scene is found output error.
	if ( w.readScene("../assets/scene.sdf") )
	{
		glutKeyboardFunc( Keyboard );
		glutIdleFunc( Idle );
		glutDisplayFunc( Draw );	
		glutMainLoop();
	} else {
		std::cout<<"No Scene Found"<<std::endl;
	}

	return 0;
}