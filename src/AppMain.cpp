// Application entry point.
//
// Two modes:
//   Preview (default): opens a GLUT window and shows the image line by line
//                      while it renders, then saves it.
//   Headless:          renders without opening a window and saves the image.
//                      Needed on machines with no display, such as CI.
//
// This is the only file that uses OpenGL/GLUT; the renderer core does not.

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

#include "World.h"
#include "Raytracer.h"

#if defined(__APPLE__)
    #include <GLUT/glut.h>
#else
    #include <GL/glut.h>
#endif

namespace
{
	const int DefaultResolution = 1000;
	const int EscapeKey         = 27;

	struct Options
	{
		std::string scene    = RAYTRACER_DEFAULT_SCENE;	// Set by CMake: absolute path of assets/scene.sdf.
		std::string output;								// Empty: results/Result<timestamp>.ppm
		int         width    = DefaultResolution;
		int         height   = DefaultResolution;
		bool        headless = false;
	};

	// State shared with the GLUT callbacks, which take no arguments.
	World      g_world;
	Raytracer *g_raytracer = nullptr;
	Options    g_options;

	void PrintUsage( const char *program )
	{
		std::cout
			<< "Usage: " << program << " [options] [scene.sdf]\n"
			<< "\n"
			<< "Options:\n"
			<< "  -s, --scene <file>    Scene to render (default: assets/scene.sdf)\n"
			<< "  -o, --output <file>   Where to save the image, as .ppm\n"
			<< "                        (default: results/Result<date>_<time>.ppm)\n"
			<< "  -w, --width <pixels>  Image width  (default: " << DefaultResolution << ")\n"
			<< "  -h, --height <pixels> Image height (default: " << DefaultResolution << ")\n"
			<< "      --headless        Render without opening a window\n"
			<< "      --help            Show this help\n";
	}

	bool ParsePositiveInt( const char *text, int &value )
	{
		char *end = nullptr;
		const long parsed = std::strtol( text, &end, 10 );
		if( end == text || *end != '\0' || parsed <= 0 || parsed > 16384 ) return false;
		value = static_cast<int>( parsed );
		return true;
	}

	enum class ParseResult { Ok, Help, Error };

	ParseResult ParseArguments( int argc, char **argv, Options &options )
	{
		for( int i = 1; i < argc; i++ )
		{
			const std::string arg = argv[i];
			const bool hasValue = i + 1 < argc;

			if( arg == "--help" ) return ParseResult::Help;
			if( arg == "--headless" ) { options.headless = true; continue; }

			if( arg == "-s" || arg == "--scene" || arg == "-o" || arg == "--output" ||
				arg == "-w" || arg == "--width" || arg == "-h" || arg == "--height" )
			{
				if( !hasValue )
				{
					std::cerr << "Missing value after " << arg << std::endl;
					return ParseResult::Error;
				}
				const char *value = argv[++i];
				if( arg == "-s" || arg == "--scene" )  { options.scene  = value; continue; }
				if( arg == "-o" || arg == "--output" ) { options.output = value; continue; }

				int &size = ( arg == "-w" || arg == "--width" ) ? options.width : options.height;
				if( !ParsePositiveInt( value, size ) )
				{
					std::cerr << "Invalid size for " << arg << ": " << value << std::endl;
					return ParseResult::Error;
				}
				continue;
			}

			if( !arg.empty() && arg[0] == '-' )
			{
				std::cerr << "Unknown option: " << arg << std::endl;
				return ParseResult::Error;
			}

			options.scene = arg;	// A bare argument is the scene file.
		}
		return ParseResult::Ok;
	}

	// results/Result<YYYY_MM_DD_HH_MM>.ppm, relative to the working directory.
	std::string TimestampedOutputPath()
	{
		const std::time_t now = std::chrono::system_clock::to_time_t( std::chrono::system_clock::now() );
		const std::tm localTime = *std::localtime( &now );
		std::ostringstream path;
		path << "results/Result" << std::put_time( &localTime, "%Y_%m_%d_%H_%M" ) << ".ppm";
		return path.str();
	}

	bool SaveImage( const Image &image, const std::string &requestedPath )
	{
		const std::string path = requestedPath.empty() ? TimestampedOutputPath() : requestedPath;

		std::error_code error;
		const std::filesystem::path parent = std::filesystem::path( path ).parent_path();
		if( !parent.empty() ) std::filesystem::create_directories( parent, error );

		if( !image.Write( path.c_str() ) )
		{
			std::cerr << "Could not write " << path << std::endl;
			return false;
		}
		std::cout << "Saved " << std::filesystem::absolute( path ).string() << std::endl;
		return true;
	}

	void PrintProgress( const Raytracer &raytracer, int height )
	{
		const int step = height >= 10 ? height / 10 : 1;
		const int line = raytracer.CurrentLine();
		if( line % step == 0 || line == height )
			std::cout << "Rendering... " << ( 100 * line / height ) << "%" << std::endl;
	}

	// Headless mode: render everything, save, exit.
	int RunHeadless()
	{
		Raytracer raytracer( g_options.width, g_options.height );
		const auto start = std::chrono::steady_clock::now();
		raytracer.Render( g_world );
		const double seconds = std::chrono::duration<double>( std::chrono::steady_clock::now() - start ).count();

		std::cout << "Rendered " << g_options.width << "x" << g_options.height
				  << " in " << std::fixed << std::setprecision( 2 ) << seconds << " s" << std::endl;
		return SaveImage( raytracer.GetImage(), g_options.output ) ? 0 : 1;
	}

	// GLUT keyboard callback: ESC quits.
	void Keyboard( unsigned char key, int, int )
	{
		if( key == EscapeKey ) std::exit( 0 );
	}

	// GLUT display callback: copy the image (rendered so far) to the window.
	void Draw()
	{
		const Image &image = g_raytracer->GetImage();
		glDrawPixels( image.width, image.height, GL_RGB, GL_UNSIGNED_BYTE, image.pixels );
		glFlush();
	}

	// GLUT idle callback: called when the event queue is empty.
	// Renders one line at a time so the window keeps updating, then saves the image.
	void Idle()
	{
		g_raytracer->cast_line( g_world );
		PrintProgress( *g_raytracer, g_options.height );
		glutPostRedisplay();

		if( g_raytracer->IsDone() )
		{
			SaveImage( g_raytracer->GetImage(), g_options.output );
			glutIdleFunc( nullptr );	// Done: stop using the CPU. The window stays open until ESC.
		}
	}

	int RunPreview( int argc, char **argv )
	{
		glutInit( &argc, argv );
		glutInitDisplayMode( GLUT_SINGLE | GLUT_RGBA );
		glutInitWindowPosition( 200, 100 );
		glutInitWindowSize( g_options.width, g_options.height );
		glutCreateWindow( "Raytracer" );
		glClearColor( 0.0, 0.0, 0.0, 0.0 );
		glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );	// Image rows are tightly packed RGB bytes.

		static Raytracer raytracer( g_options.width, g_options.height );
		g_raytracer = &raytracer;

		glutKeyboardFunc( Keyboard );
		glutIdleFunc( Idle );
		glutDisplayFunc( Draw );
		glutMainLoop();
		return 0;
	}
}

int main( int argc, char **argv )
{
	switch( ParseArguments( argc, argv, g_options ) )
	{
		case ParseResult::Help:  PrintUsage( argv[0] ); return 0;
		case ParseResult::Error: PrintUsage( argv[0] ); return 1;
		case ParseResult::Ok:    break;
	}

	// Read the scene before opening a window, so a bad path fails fast.
	if( !g_world.readScene( g_options.scene.c_str() ) )
	{
		std::cerr << "No Scene Found: could not read \"" << g_options.scene << "\"" << std::endl;
		return 1;
	}

	return g_options.headless ? RunHeadless() : RunPreview( argc, argv );
}
