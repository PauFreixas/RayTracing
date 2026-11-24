// Unit tests for the scene file reader.

#include <doctest/doctest.h>
#include <filesystem>
#include <fstream>
#include <string>

#include "World.h"

namespace
{
	// Writes `contents` to a file in the test output folder and returns its path.
	// Files are opened in binary mode so the line endings are exactly as given.
	std::string WriteSceneFile( const std::string &name, const std::string &contents )
	{
		const std::filesystem::path dir = std::filesystem::path( RAYTRACER_TEST_OUTPUT_DIR ) / "scenes";
		std::filesystem::create_directories( dir );
		const std::filesystem::path path = dir / name;
		std::ofstream( path, std::ios::binary ) << contents;
		return path.string();
	}

	int CountObjects( const Scene &scene )
	{
		int count = 0;
		for( const Object *object = scene.first; object != nullptr; object = object->next ) count++;
		return count;
	}

	const std::string ValidScene =
		"# A comment\n"
		"\n"
		"eye     (0, 1, 20)\n"
		"lookat  (0, 0, 0)\n"
		"up      (0, 1, 0)\n"
		"vpdist  2.5\n"
		"lightpos   (1, 2, 3)\n"
		"lightcolor [1, 0.5, 0.25]\n"
		"lightpos   (-1, 2, 3)\n"
		"lightcolor [0, 0, 1]\n"
		"amblight   [0.1, 0.1, 0.1]\n"
		"bgcolor    [0.2, 0.3, 0.4]\n"
		"sphere   (0, 0, 0) 2\n"
		"diffuse  [0.9, 0.1, 0.1]\n"
		"Phong_exp 20\n"
		"cube     (-1, -1, -1) (1, 1, 1)\n"
		"opacity  0.5\n"
		"refractive_index 1.5\n";
}

TEST_SUITE( "reader" )
{
	TEST_CASE( "A valid scene is read completely" )
	{
		World world;
		REQUIRE( world.readScene( WriteSceneFile( "valid.sdf", ValidScene ).c_str() ) );

		const Camera &camera = world.getCamera();
		CHECK( camera.eye.y == doctest::Approx( 1.0 ) );
		CHECK( camera.eye.z == doctest::Approx( 20.0 ) );
		CHECK( camera.vpdist == doctest::Approx( 2.5 ) );

		const Scene &scene = world.getScene();
		REQUIRE( scene.num_lights == 2 );
		CHECK( scene.light[0].m_Position.x == doctest::Approx( 1.0 ) );
		CHECK( scene.light[0].m_Color.green == doctest::Approx( 0.5 ) );
		CHECK( scene.light[1].m_Color.blue == doctest::Approx( 1.0 ) );
		CHECK( scene.ambient.red == doctest::Approx( 0.1 ) );
		CHECK( scene.bgcolor.blue == doctest::Approx( 0.4 ) );

		REQUIRE( CountObjects( scene ) == 2 );

		// Objects are stored newest first: the cube, then the sphere.
		const Material &cube = scene.first->material;
		CHECK( cube.m_Opacity == doctest::Approx( 0.5 ) );
		CHECK( cube.m_RefractiveIndex == doctest::Approx( 1.5 ) );
		CHECK( cube.m_Reflectivity == doctest::Approx( 0.0 ) );	// Not in the file: default.

		const Material &sphere = scene.first->next->material;
		CHECK( sphere.m_Diffuse.red == doctest::Approx( 0.9 ) );
		CHECK( sphere.m_Phong_exp == doctest::Approx( 20.0 ) );
		CHECK( sphere.m_Opacity == doctest::Approx( 1.0 ) );		// Not in the file: default.
	}

	TEST_CASE( "Windows line endings and a missing final newline are accepted" )
	{
		std::string crlf;
		for( char c : ValidScene )
		{
			if( c == '\n' ) crlf += '\r';
			crlf += c;
		}
		crlf += "   ";	// Last line: only spaces, no newline.

		World world;
		REQUIRE( world.readScene( WriteSceneFile( "crlf.sdf", crlf ).c_str() ) );
		CHECK( world.getScene().num_lights == 2 );
		CHECK( CountObjects( world.getScene() ) == 2 );
	}

	TEST_CASE( "A malformed line is rejected" )
	{
		World world;
		// The sphere is missing its radius.
		CHECK_FALSE( world.readScene( WriteSceneFile( "malformed.sdf", "eye (0, 0, 10)\nsphere (0, 0, 0)\n" ).c_str() ) );
	}

	TEST_CASE( "An unknown keyword is rejected" )
	{
		World world;
		CHECK_FALSE( world.readScene( WriteSceneFile( "unknown.sdf", "eye (0, 0, 10)\nteapot (0, 0, 0)\n" ).c_str() ) );
	}

	TEST_CASE( "A material line before any object is rejected instead of crashing" )
	{
		World world;
		CHECK_FALSE( world.readScene( WriteSceneFile( "material_first.sdf", "diffuse [1, 0, 0]\nsphere (0, 0, 0) 1\n" ).c_str() ) );
	}

	TEST_CASE( "More lights than Scene::MaxLights are rejected instead of overflowing" )
	{
		std::string contents;
		for( int i = 0; i <= Scene::MaxLights; i++ )
			contents += "lightpos (0, 0, 0)\nlightcolor [1, 1, 1]\n";

		World world;
		CHECK_FALSE( world.readScene( WriteSceneFile( "too_many_lights.sdf", contents ).c_str() ) );
	}

	TEST_CASE( "A missing file is reported" )
	{
		World world;
		CHECK_FALSE( world.readScene( "this/file/does/not/exist.sdf" ) );
	}

	TEST_CASE( "Every scene in assets/ can be read" )
	{
		for( const char *name : { "scene.sdf", "cornell_mirror_glass.sdf", "showcase.sdf" } )
		{
			CAPTURE( name );
			World world;
			const std::string path = std::string( RAYTRACER_ASSETS_DIR ) + "/" + name;
			CHECK( world.readScene( path.c_str() ) );
			CHECK( world.getScene().num_lights > 0 );
			CHECK( world.getScene().first != nullptr );
		}
	}
}
