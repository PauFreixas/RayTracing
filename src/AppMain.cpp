#include <chrono>
#include <thread>

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
		std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
		glFlush(); 
		glutPostRedisplay();
	}
}

// Usage: Raytracer [scene.sdf]
// Without an argument, the Cornell box in assets/scene.sdf is rendered.
int main(int argc, char** argv)
{
	//Initialize glut (this also strips GLUT-specific options from argv)
	glutInit(&argc, argv);

	//Pick the scene file: first command-line argument, or the default scene.
	//RAYTRACER_DEFAULT_SCENE is set by CMake to the absolute path of assets/scene.sdf.
	const char* scenePath = ( argc > 1 ) ? argv[1] : RAYTRACER_DEFAULT_SCENE;

	//Read the scene before opening a window. If it can't be read, output an error.
	if ( !w.readScene( scenePath ) )
	{
		std::cerr << "No Scene Found: could not read \"" << scenePath << "\"" << std::endl;
		std::cerr << "Usage: " << argv[0] << " [scene.sdf]" << std::endl;
		return 1;
	}

	//Create the window
	glutInitDisplayMode( GLUT_SINGLE | GLUT_RGBA);
	glutInitWindowPosition(200,100);
	glutInitWindowSize( RESOLUTIONX , RESOLUTIONY );
	glutCreateWindow("Raytracer");
	glClearColor (0.0, 0.0, 0.0, 0.0);

	//Set up keyboard, idle and draw functions and run main GLUT loop
	glutKeyboardFunc( Keyboard );
	glutIdleFunc( Idle );
	glutDisplayFunc( Draw );
	glutMainLoop();

	return 0;
}
