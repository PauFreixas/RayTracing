// Image regression tests.
//
// Each scene in assets/ is rendered at a low resolution and compared with a
// reference image in tests/reference/. Different compilers and processors
// round floating-point maths slightly differently, so the comparison allows
// small differences: a few pixels may change a lot (for example along the
// edge of an object), and the rest only a little.
//
// To accept new renders as the references (after a deliberate visual change):
//   set RAYTRACER_UPDATE_REFERENCES=1, run the image tests, check the new
//   images in tests/reference/ by eye, and commit them.
//
// Every run also writes the render and a difference image to
// <build>/test-output/, so a failure can be inspected (CI uploads them).

#include <doctest/doctest.h>
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "World.h"
#include "Raytracer.h"

namespace
{
	const int TestResolution = 128;

	// A pixel "differs" when any channel changes by more than this (out of 255).
	const int    PixelTolerance       = 16;
	// At most this share of pixels may differ, e.g. along object edges.
	const double MaxDifferentFraction = 0.01;
	// Average change per channel over the whole image, out of 255.
	const double MaxMeanDifference    = 1.0;

	struct RgbImage
	{
		int width = 0;
		int height = 0;
		std::vector<unsigned char> rgb;	// Top row first, 3 bytes per pixel.
	};

	// Reads a binary PPM (P6) with maxval 255, as written by Image::Write.
	bool ReadPpm( const std::filesystem::path &path, RgbImage &image )
	{
		std::ifstream file( path, std::ios::binary );
		std::string magic;
		int maxval = 0;
		if( !( file >> magic >> image.width >> image.height >> maxval ) ) return false;
		if( magic != "P6" || maxval != 255 || image.width <= 0 || image.height <= 0 ) return false;
		file.get();	// The single whitespace character after the header.

		image.rgb.resize( static_cast<size_t>( image.width ) * image.height * 3 );
		file.read( reinterpret_cast<char *>( image.rgb.data() ), static_cast<std::streamsize>( image.rgb.size() ) );
		return static_cast<size_t>( file.gcount() ) == image.rgb.size();
	}

	bool WritePpm( const std::filesystem::path &path, const RgbImage &image )
	{
		std::ofstream file( path, std::ios::binary );
		file << "P6\n" << image.width << " " << image.height << "\n255\n";
		file.write( reinterpret_cast<const char *>( image.rgb.data() ), static_cast<std::streamsize>( image.rgb.size() ) );
		return static_cast<bool>( file );
	}

	bool UpdateReferencesRequested()
	{
		const char *value = std::getenv( "RAYTRACER_UPDATE_REFERENCES" );
		return value != nullptr && std::string( value ) == "1";
	}

	void RenderAndCompare( const std::string &sceneName )
	{
		namespace fs = std::filesystem;
		const fs::path scenePath     = fs::path( RAYTRACER_ASSETS_DIR ) / ( sceneName + ".sdf" );
		const fs::path referencePath = fs::path( RAYTRACER_REFERENCE_DIR ) / ( sceneName + ".ppm" );
		const fs::path outputDir     = fs::path( RAYTRACER_TEST_OUTPUT_DIR );
		const fs::path actualPath    = outputDir / ( sceneName + ".ppm" );
		const fs::path diffPath      = outputDir / ( sceneName + "_diff.ppm" );
		fs::create_directories( outputDir );

		World world;
		REQUIRE( world.readScene( scenePath.string().c_str() ) );
		Raytracer raytracer( TestResolution, TestResolution );
		raytracer.Render( world );
		REQUIRE( raytracer.GetImage().Write( actualPath.string().c_str() ) );

		if( UpdateReferencesRequested() )
		{
			fs::create_directories( referencePath.parent_path() );
			fs::copy_file( actualPath, referencePath, fs::copy_options::overwrite_existing );
			MESSAGE( "Updated reference " << referencePath.string() );
			return;
		}

		RgbImage actual, reference;
		REQUIRE( ReadPpm( actualPath, actual ) );
		INFO( "Reference: " << referencePath.string()
			  << " (create it by running the image tests with RAYTRACER_UPDATE_REFERENCES=1)" );
		REQUIRE( ReadPpm( referencePath, reference ) );
		REQUIRE( actual.width == reference.width );
		REQUIRE( actual.height == reference.height );

		// Compare, and build a difference image (brighter = bigger difference).
		RgbImage diff = actual;
		size_t differentPixels = 0;
		double totalDifference = 0.0;
		const size_t pixelCount = static_cast<size_t>( actual.width ) * actual.height;
		for( size_t p = 0; p < pixelCount; p++ )
		{
			int largest = 0;
			for( int c = 0; c < 3; c++ )
			{
				const int d = std::abs( int( actual.rgb[p * 3 + c] ) - int( reference.rgb[p * 3 + c] ) );
				largest = std::max( largest, d );
				totalDifference += d;
				diff.rgb[p * 3 + c] = static_cast<unsigned char>( std::min( 255, d * 8 ) );
			}
			if( largest > PixelTolerance ) differentPixels++;
		}
		WritePpm( diffPath, diff );

		const double differentFraction = double( differentPixels ) / double( pixelCount );
		const double meanDifference    = totalDifference / double( pixelCount * 3 );

		INFO( "Render: " << actualPath.string() << ", difference image: " << diffPath.string() );
		INFO( "Pixels differing by more than " << PixelTolerance << ": " << differentPixels
			  << " (" << 100.0 * differentFraction << "%)" );
		INFO( "Mean difference per channel: " << meanDifference );
		CHECK( differentFraction <= MaxDifferentFraction );
		CHECK( meanDifference <= MaxMeanDifference );
	}
}

TEST_SUITE( "image" )
{
	TEST_CASE( "Default scene matches its reference" )        { RenderAndCompare( "scene" ); }
	TEST_CASE( "Mirror and glass scene matches its reference" ) { RenderAndCompare( "cornell_mirror_glass" ); }
	TEST_CASE( "Showcase scene matches its reference" )       { RenderAndCompare( "showcase" ); }
}
