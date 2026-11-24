// Unit tests for the vector and matrix maths, and the reflection and refraction helpers.

#include <doctest/doctest.h>
#include <cmath>

#include "Vec3.h"
#include "Mat3x3.h"

namespace
{
	void CheckVec( const Vec3 &actual, const Vec3 &expected, double epsilon = 1e-9 )
	{
		CHECK( actual.x == doctest::Approx( expected.x ).epsilon( epsilon ) );
		CHECK( actual.y == doctest::Approx( expected.y ).epsilon( epsilon ) );
		CHECK( actual.z == doctest::Approx( expected.z ).epsilon( epsilon ) );
	}
}

TEST_SUITE( "math" )
{
	TEST_CASE( "Vec3 arithmetic" )
	{
		const Vec3 a( 1, 2, 3 );
		const Vec3 b( 4, -5, 6 );

		CheckVec( a + b, Vec3( 5, -3, 9 ) );
		CheckVec( a - b, Vec3( -3, 7, -3 ) );
		CheckVec( -a, Vec3( -1, -2, -3 ) );
		CheckVec( 2.0 * a, Vec3( 2, 4, 6 ) );
		CheckVec( a / 2.0, Vec3( 0.5, 1, 1.5 ) );
		CHECK( a * b == doctest::Approx( 1 * 4 - 2 * 5 + 3 * 6 ) );	// Dot product.
	}

	TEST_CASE( "Vec3 cross product follows the right-hand rule" )
	{
		CheckVec( Vec3( 1, 0, 0 ) ^ Vec3( 0, 1, 0 ), Vec3( 0, 0, 1 ) );
		CheckVec( Vec3( 0, 1, 0 ) ^ Vec3( 0, 0, 1 ), Vec3( 1, 0, 0 ) );

		const Vec3 a( 1, 2, 3 ), b( -2, 0.5, 4 );
		const Vec3 c = a ^ b;
		CHECK( c * a == doctest::Approx( 0.0 ) );		// Perpendicular to both inputs.
		CHECK( c * b == doctest::Approx( 0.0 ) );
	}

	TEST_CASE( "Vec3 length, unit vectors and distance" )
	{
		CHECK( Length( Vec3( 3, 4, 0 ) ) == doctest::Approx( 5.0 ) );
		CHECK( LengthSquared( Vec3( 3, 4, 0 ) ) == doctest::Approx( 25.0 ) );
		CHECK( Length( Unit( Vec3( 1, -7, 3 ) ) ) == doctest::Approx( 1.0 ) );
		CHECK( dist( Vec3( 1, 1, 1 ), Vec3( 1, 1, 4 ) ) == doctest::Approx( 3.0 ) );
	}

	TEST_CASE( "Vec3 / Vec3 removes the component parallel to the second vector" )
	{
		CheckVec( Vec3( 3, 4, 5 ) / Vec3( 0, 0, 2 ), Vec3( 3, 4, 0 ) );
		CheckVec( Vec3( 1, 2, 3 ) / Vec3( 0, 0, 0 ), Vec3( 1, 2, 3 ) );	// Zero vector: unchanged.
	}

	TEST_CASE( "Mat3x3 identity, product and transpose" )
	{
		Mat3x3 M;
		double value = 1.0;
		for( int i = 0; i < 3; i++ )
			for( int j = 0; j < 3; j++ ) M( i, j ) = value++;

		const Mat3x3 I = Mat3x3::Identity();
		const Mat3x3 MI = M * I;
		for( int i = 0; i < 3; i++ )
			for( int j = 0; j < 3; j++ )
			{
				CHECK( MI( i, j ) == doctest::Approx( M( i, j ) ) );
				CHECK( Transpose( M )( i, j ) == doctest::Approx( M( j, i ) ) );
			}

		CheckVec( I * Vec3( 1, 2, 3 ), Vec3( 1, 2, 3 ) );
		CheckVec( M * Vec3( 1, 0, 0 ), Vec3( M( 0, 0 ), M( 1, 0 ), M( 2, 0 ) ) );
	}

	TEST_CASE( "Mat3x3 determinant and adjoint (cofactor matrix)" )
	{
		Mat3x3 M;
		M( 0, 0 ) = 2; M( 0, 1 ) = -1; M( 0, 2 ) = 0;
		M( 1, 0 ) = 1; M( 1, 1 ) =  3; M( 1, 2 ) = 4;
		M( 2, 0 ) = 0; M( 2, 1 ) =  5; M( 2, 2 ) = -2;

		const double d = det( M );
		CHECK( d == doctest::Approx( 2 * ( 3 * -2 - 4 * 5 ) - ( -1 ) * ( 1 * -2 - 4 * 0 ) ) );
		CHECK( det( Mat3x3::Identity() ) == doctest::Approx( 1.0 ) );

		// Adjoint() returns the cofactor matrix C, so M * C^T = det(M) * I.
		const Mat3x3 product = M * Transpose( Adjoint( M ) );
		for( int i = 0; i < 3; i++ )
			for( int j = 0; j < 3; j++ )
				CHECK( product( i, j ) == doctest::Approx( i == j ? d : 0.0 ) );
	}

	TEST_CASE( "Reflection mirrors a direction about the normal" )
	{
		const Vec3 N( 0, 1, 0 );
		CheckVec( Reflection( Vec3( 1, -1, 0 ), N ), Vec3( 1, 1, 0 ) );
		CheckVec( Reflection( Vec3( 0, -1, 0 ), N ), Vec3( 0, 1, 0 ) );	// Head-on: straight back.
		CheckVec( Reflection( Vec3( 1, 0, 0 ), N ), Vec3( 1, 0, 0 ) );		// Parallel: unchanged.
	}

	TEST_CASE( "Refraction follows Snell's law" )
	{
		const Vec3 N( 0, 1, 0 );	// Surface normal, facing the incoming ray.

		SUBCASE( "head-on rays go straight through" )
		{
			CheckVec( Refraction( Vec3( 0, -1, 0 ), N, 1.5 ), Vec3( 0, -1, 0 ) );
		}

		SUBCASE( "air into glass bends towards the normal" )
		{
			const double angle = 45.0 * 3.14159265358979 / 180.0;
			const Vec3 V( std::sin( angle ), -std::cos( angle ), 0 );
			const Vec3 T = Refraction( V, N, 1.5 );

			CHECK( Length( T ) == doctest::Approx( 1.0 ) );
			// n1 sin(theta1) = n2 sin(theta2), with n1 = 1 and n2 = 1.5.
			CHECK( T.x == doctest::Approx( std::sin( angle ) / 1.5 ) );
			CHECK( T.y < 0.0 );	// Still going into the surface.
		}

		SUBCASE( "glass into air beyond the critical angle is total internal reflection" )
		{
			const double angle = 60.0 * 3.14159265358979 / 180.0;	// Critical angle for glass is about 41.8 degrees.
			const Vec3 V( std::sin( angle ), -std::cos( angle ), 0 );
			CheckVec( Refraction( V, N, 1.0 / 1.5 ), Vec3( 0, 0, 0 ) );
		}
	}
}
