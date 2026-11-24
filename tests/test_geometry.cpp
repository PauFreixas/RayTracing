// Unit tests for ray-object intersection: hit, miss, grazing hit and hit from inside.

#include <doctest/doctest.h>
#include <cmath>

#include "Sphere.h"
#include "Cube.h"
#include "Triangle.h"
#include "Polygon.h"

namespace
{
	Ray MakeRay( const Vec3 &origin, const Vec3 &direction )
	{
		Ray ray;
		ray.origin      = origin;
		ray.direction   = Unit( direction );
		ray.no_emitters = false;
		return ray;
	}

	HitGeom NoHitYet()
	{
		HitGeom hit;
		hit.distance = Infinity;
		return hit;
	}

	void CheckVec( const Vec3 &actual, const Vec3 &expected )
	{
		CHECK( actual.x == doctest::Approx( expected.x ) );
		CHECK( actual.y == doctest::Approx( expected.y ) );
		CHECK( actual.z == doctest::Approx( expected.z ) );
	}
}

TEST_SUITE( "geometry" )
{
	TEST_CASE( "Ray-sphere intersection" )
	{
		const Sphere sphere( Vec3( 0, 0, 0 ), 1.0f );
		HitGeom hit = NoHitYet();

		SUBCASE( "hit from outside returns the near side" )
		{
			REQUIRE( sphere.Intersect( MakeRay( Vec3( 0, 0, 10 ), Vec3( 0, 0, -1 ) ), hit ) );
			CHECK( hit.distance == doctest::Approx( 9.0 ) );
			CheckVec( hit.point, Vec3( 0, 0, 1 ) );
			CheckVec( hit.normal, Vec3( 0, 0, 1 ) );
		}

		SUBCASE( "miss" )
		{
			CHECK_FALSE( sphere.Intersect( MakeRay( Vec3( 0, 2, 10 ), Vec3( 0, 0, -1 ) ), hit ) );
		}

		SUBCASE( "grazing ray touches the sphere at one point" )
		{
			REQUIRE( sphere.Intersect( MakeRay( Vec3( 0, 1, 10 ), Vec3( 0, 0, -1 ) ), hit ) );
			CHECK( hit.distance == doctest::Approx( 10.0 ) );
			CheckVec( hit.point, Vec3( 0, 1, 0 ) );
		}

		SUBCASE( "hit from inside returns the far side" )
		{
			REQUIRE( sphere.Intersect( MakeRay( Vec3( 0, 0, 0 ), Vec3( 1, 0, 0 ) ), hit ) );
			CHECK( hit.distance == doctest::Approx( 1.0 ) );
			CheckVec( hit.point, Vec3( 1, 0, 0 ) );
		}

		SUBCASE( "sphere behind the ray is missed" )
		{
			CHECK_FALSE( sphere.Intersect( MakeRay( Vec3( 0, 0, 10 ), Vec3( 0, 0, 1 ) ), hit ) );
		}

		SUBCASE( "a closer hit already found is kept" )
		{
			hit.distance = 5.0;
			CHECK_FALSE( sphere.Intersect( MakeRay( Vec3( 0, 0, 10 ), Vec3( 0, 0, -1 ) ), hit ) );
			CHECK( hit.distance == doctest::Approx( 5.0 ) );
		}
	}

	TEST_CASE( "Ray-cube intersection" )
	{
		const Cube cube( Vec3( -1, -1, -1 ), Vec3( 1, 1, 1 ) );
		HitGeom hit = NoHitYet();

		SUBCASE( "hit from outside returns the front face" )
		{
			REQUIRE( cube.Intersect( MakeRay( Vec3( 0.2, 0.3, 10 ), Vec3( 0, 0, -1 ) ), hit ) );
			CHECK( hit.distance == doctest::Approx( 9.0 ) );
			CheckVec( hit.point, Vec3( 0.2, 0.3, 1 ) );
			CheckVec( hit.normal, Vec3( 0, 0, 1 ) );
		}

		SUBCASE( "hit from the side returns that face's normal" )
		{
			REQUIRE( cube.Intersect( MakeRay( Vec3( -10, 0.5, 0 ), Vec3( 1, 0, 0 ) ), hit ) );
			CHECK( hit.distance == doctest::Approx( 9.0 ) );
			CheckVec( hit.normal, Vec3( -1, 0, 0 ) );
		}

		SUBCASE( "miss" )
		{
			CHECK_FALSE( cube.Intersect( MakeRay( Vec3( 3, 0, 10 ), Vec3( 0, 0, -1 ) ), hit ) );
		}

		SUBCASE( "grazing ray just outside a face misses" )
		{
			CHECK_FALSE( cube.Intersect( MakeRay( Vec3( 1.001, 0, 10 ), Vec3( 0, 0, -1 ) ), hit ) );
		}

		SUBCASE( "hit from inside returns the face the ray leaves through" )
		{
			REQUIRE( cube.Intersect( MakeRay( Vec3( 0, 0, 0 ), Vec3( 0, 0, -1 ) ), hit ) );
			CHECK( hit.distance == doctest::Approx( 1.0 ) );
			CheckVec( hit.point, Vec3( 0, 0, -1 ) );
		}
	}

	TEST_CASE( "Ray-triangle intersection" )
	{
		const Triangle triangle( Vec3( -1, -1, 0 ), Vec3( 1, -1, 0 ), Vec3( 0, 1, 0 ) );
		HitGeom hit = NoHitYet();

		SUBCASE( "hit" )
		{
			REQUIRE( triangle.Intersect( MakeRay( Vec3( 0, 0, 5 ), Vec3( 0, 0, -1 ) ), hit ) );
			CHECK( hit.distance == doctest::Approx( 5.0 ) );
			CheckVec( hit.point, Vec3( 0, 0, 0 ) );
			CheckVec( hit.normal, Vec3( 0, 0, 1 ) );
		}

		SUBCASE( "miss outside the edges" )
		{
			CHECK_FALSE( triangle.Intersect( MakeRay( Vec3( 0.9, 0.9, 5 ), Vec3( 0, 0, -1 ) ), hit ) );
		}

		SUBCASE( "grazing hit exactly on an edge counts" )
		{
			CHECK( triangle.Intersect( MakeRay( Vec3( 0, -1, 5 ), Vec3( 0, 0, -1 ) ), hit ) );
		}

		SUBCASE( "ray parallel to the triangle misses" )
		{
			CHECK_FALSE( triangle.Intersect( MakeRay( Vec3( -5, 0, 0 ), Vec3( 1, 0, 0 ) ), hit ) );
		}

		SUBCASE( "triangles are two-sided: hit from behind" )
		{
			REQUIRE( triangle.Intersect( MakeRay( Vec3( 0, 0, -5 ), Vec3( 0, 0, 1 ) ), hit ) );
			CHECK( hit.distance == doctest::Approx( 5.0 ) );
		}

		SUBCASE( "triangle behind the ray is missed" )
		{
			CHECK_FALSE( triangle.Intersect( MakeRay( Vec3( 0, 0, 5 ), Vec3( 0, 0, 1 ) ), hit ) );
		}
	}

	TEST_CASE( "Ray-polygon intersection (convex pentagon)" )
	{
		// Regular pentagon of radius 1 in the z = 0 plane.
		Vec3 corners[5];
		for( int i = 0; i < 5; i++ )
		{
			const double angle = ( 90.0 + 72.0 * i ) * 3.14159265358979 / 180.0;
			corners[i] = Vec3( std::cos( angle ), std::sin( angle ), 0 );
		}
		const Polygon pentagon( corners[0], corners[1], corners[2], corners[3], corners[4] );
		HitGeom hit = NoHitYet();

		SUBCASE( "hit at the centre" )
		{
			REQUIRE( pentagon.Intersect( MakeRay( Vec3( 0, 0, 5 ), Vec3( 0, 0, -1 ) ), hit ) );
			CHECK( hit.distance == doctest::Approx( 5.0 ) );
			CheckVec( hit.point, Vec3( 0, 0, 0 ) );
			CHECK( std::fabs( hit.normal.z ) == doctest::Approx( 1.0 ) );
		}

		SUBCASE( "hit near an edge, inside" )
		{
			// Along +x the edge is at 0.809 / cos(18 deg), about 0.851 from the centre.
			CHECK( pentagon.Intersect( MakeRay( Vec3( 0.8, 0, 5 ), Vec3( 0, 0, -1 ) ), hit ) );
		}

		SUBCASE( "miss between two corners, outside" )
		{
			CHECK_FALSE( pentagon.Intersect( MakeRay( Vec3( 0.9, 0, 5 ), Vec3( 0, 0, -1 ) ), hit ) );
		}

		SUBCASE( "ray parallel to the polygon misses" )
		{
			CHECK_FALSE( pentagon.Intersect( MakeRay( Vec3( -5, 0, 0 ), Vec3( 1, 0, 0 ) ), hit ) );
		}

		SUBCASE( "hit from behind" )
		{
			CHECK( pentagon.Intersect( MakeRay( Vec3( 0, 0, -5 ), Vec3( 0, 0, 1 ) ), hit ) );
		}
	}
}
