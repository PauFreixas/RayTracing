#include <math.h>
/***************************************************************************
*                                                                          *
* This is the source file for a ray tracer. It defines most of the		   *
* fundamental functions using in ray tracing.  In particular, "Trace" and  *
* "Shade".  It also defines the MakeImage function, which casts all the    *
* rays from the eye, and several object intersection methods.  Some of the *
* functions are left blank, or with "place holders" that don't do very     *
* much.  You need to fill these in.                                        *
*                                                                          *
*                                                                          *
***************************************************************************/

static const int tree_depth = 5;		// Maximum number of reflection/refraction bounces

// Distance by which secondary rays are pushed off the surface they start from.
// Without it, rounding errors make a surface hit itself ("shadow acne").
static const double RayBias = 1.0E-3;


#include "Raytracer.h"

// Cast_line casts all the initial rays starting from the eye for a single
//  raster line. Copies pixels to image object.
void Raytracer::cast_line( const World &world )
{
	if( isDone ) return;

	const Camera &camera = world.getCamera();
	const Scene  &scene  = world.getScene();

    Ray ray;
	ray.origin = camera.eye; // All initial rays originate from the eye.
	ray.no_emitters = false;

    Vec3 G  = Unit( camera.lookat - camera.eye );						// Gaze direction.
    Vec3 U  = Unit( camera.up / G );									// Up vector.
    Vec3 R  = Unit( G ^ U );											// Right vector.
	// The view plane is 2 units tall and 2 * aspect units wide, so pixels stay square.
	const double aspect = double( resolutionX ) / double( resolutionY );
    Vec3 O  = ( camera.vpdist * G ) - aspect * R + U;				// "Origin" for the raster.
    Vec3 dU = U * ( 2.0 / ( resolutionY - 1 ) );						// Up increments.
	Vec3 dR = R * ( 2.0 * aspect / ( resolutionX - 1 ) );			// Right increments.

    for( int i = 0; i < resolutionX; i++ )
    {
		ray.direction = Unit( O + i * dR - currentLine * dU  );
		(*I)( resolutionY-currentLine-1, i ) = ToneMap( Trace( ray, scene, tree_depth ) );
    }

	if( ++currentLine == resolutionY ) isDone = true;
}

// Render renders every remaining line of the image in one go.
void Raytracer::Render( const World &world )
{
	while( !isDone ) cast_line( world );
}


// This is a trivial tone mapper; it merely maps values that are
// in [0,1] and maps them to integers between 0 and 255.  If the
// real value is above 1, it merely truncates.  A true tone mapper
// would attempt to handle very large values nicely, without
// truncation; that is, it would try to compensate for the fact that
// displays have a very limited dynamic range.
Pixel Raytracer::ToneMap( const Color &color )
{
	int red   = (int)floor( 256 * color.red   );
    int green = (int)floor( 256 * color.green );
    int blue  = (int)floor( 256 * color.blue  );
    channel r = (channel)( red   >= 255 ? 255 : red   ); 
    channel g = (channel)( green >= 255 ? 255 : green ); 
    channel b = (channel)( blue  >= 255 ? 255 : blue  );
    return Pixel( r, g, b );
}

// Trace is the most fundamental of all the ray tracing functions.  It
// answers the query "What color do I see looking along the given ray
// in the current scene?"  This is an inherently recursive process, as
// trace may again be called as a result of the ray hitting a reflecting
// object.  To prevent the possibility of infinite recursion, a maximum
// depth is placed on the resulting ray tree.
Color Raytracer::Trace( const Ray &ray, const Scene &scene, int max_tree_depth  )
{
    Color   color;                    // The color to return.
    HitInfo hitinfo;                  // Holds info to pass to shader.

	// Intitallizes hit distance to infinity to allow finding intersections in all ray length
	hitinfo.geom.distance = Infinity;

	if (Cast( ray, scene, hitinfo ) > 0.0f && max_tree_depth > -1 )
	{
        // The ray hits an object, so shade the point that the ray hit.
        // Cast has put all necessary information for Shade in "hitinfo".
		
		// If the ray has no_emitters activated and the first hit is an emitter
		//  this ray shouldn't contribute to the color of the current pixel
		if( hitinfo.material.Emitter() && ray.no_emitters == true ) color = Color ();

		// The ray hits an object, so shade the point that the ray hit.
        // Cast has put all necessary information for Shade in "hitinfo".
		else color = Shade( hitinfo, scene, max_tree_depth - 1  );
    }
    else
    {
        // Either the ray has failed to hit anything, or
        // the recursion has bottomed out.
        color = scene.bgcolor;
    }
    
    return color;
}

// Cast finds the first point of intersection (if there is one)
// between a ray and a list of geometric objects.  If no intersection
// exists, the function returns false.  Information about the
// closest object hit is returned in "hitinfo". 
int Raytracer::Cast( const Ray &ray, const Scene &scene, HitInfo &hitinfo, Object *ignore )
{
	int hit = false;

    // Each intersector is ONLY allowed to write into the "HitGeom"
    // structure if it has determined that the ray hits the object
    // at a CLOSER distance than currently recorded in HitGeom.distance.
    // When a closer hit is found, the material fields of the "HitInfo"
    // structure are updated to hold the material of the object that 
    // was just hit.

    for( Object *object = scene.first; object != NULL; object = object->next )
    {
        if( object != ignore && object->Intersect( ray, hitinfo.geom ) )
            {
            hitinfo.material = object->material;  // Material of closest surface.
            hit = true;                           // We have hit an object.
            }
    }
    return hit;
}

// Shade assigns a color to a point on a surface, as it is seen
// from another point.  The coordinates of these points, the normal
// of the surface, and the surface material are all recorded in the
// HitInfo structure.
//
// This is a classic Whitted shader:
//   color = emission
//         + diffuse weight * (ambient + sum over visible lights of Lambert diffuse)
//         + sum over visible lights of Phong specular
//         + reflection weight   * Trace(mirror ray)
//         + transmission weight * Trace(refracted ray)
//
// The weights split each surface's light between diffuse, mirror and glass:
//   opacity * (1 - reflectivity)                    -> diffuse
//   opacity * reflectivity + (1 - opacity) * F      -> mirror reflection
//   (1 - opacity) * (1 - F)                         -> refraction
// where F is the Fresnel reflectance (Schlick's approximation).  They add up to 1.
Color Raytracer::Shade( const HitInfo &hit, const Scene &scene, int max_tree_depth )
{
	const Material &mat = hit.material;
	const Vec3 P = hit.geom.point;
	const Vec3 D = Unit( P - hit.geom.origin );		// Direction of the incoming ray.
	const Vec3 V = -D;								// Direction towards the viewer.

	// Make the normal face the viewer.  Cubes, triangles and polygons are seen
	// from both sides, and a ray inside a glass sphere sees its normal from behind.
	Vec3 N = Unit( hit.geom.normal );
	const bool entering = ( N * V ) > 0.0;
	if( !entering ) N = -N;

	// Split the surface's light between diffuse, mirror and transmission.
	const double opacity      = mat.m_Opacity;
	const double reflectivity = mat.m_Reflectivity;
	const double transparency = 1.0 - opacity;

	// Fresnel term and refracted direction, only needed for transparent surfaces.
	double fresnel = 0.0;
	Vec3   T;
	bool   refracts = false;
	if( transparency > 0.0 )
	{
		// Indices of refraction on the incoming (n1) and outgoing (n2) side.
		const double ior = mat.m_RefractiveIndex > 0.0 ? mat.m_RefractiveIndex : 1.0;
		const double n1  = entering ? 1.0 : ior;
		const double n2  = entering ? ior : 1.0;

		// Snell's law.  Refraction() returns a zero vector on total internal reflection.
		T = Refraction( D, N, n2 / n1 );
		refracts = LengthSquared( T ) > 0.0;

		if( refracts )
		{
			// Schlick: F = F0 + (1 - F0)(1 - cos)^5, using the angle on the
			// less dense side of the surface.
			const double F0  = ( ( n1 - n2 ) / ( n1 + n2 ) ) * ( ( n1 - n2 ) / ( n1 + n2 ) );
			const double cosTheta = ( n1 <= n2 ) ? ( N * V ) : ( -N * Unit( T ) );
			fresnel = F0 + ( 1.0 - F0 ) * pow( 1.0 - cosTheta, 5.0 );
		}
		else
		{
			fresnel = 1.0;	// Total internal reflection: everything is reflected.
		}
	}

	const double diffuseWeight      = opacity * ( 1.0 - reflectivity );
	const double reflectionWeight   = opacity * reflectivity + transparency * fresnel;
	const double transmissionWeight = transparency * ( 1.0 - fresnel );

	// Emission and ambient light.
	Color color = mat.m_Emission + diffuseWeight * ( scene.ambient * mat.m_Diffuse );

	// Direct light: add up the contribution of every light that isn't in shadow.
	for( int i = 0; i < scene.num_lights; i++ )
	{
		const PointLight &light = scene.light[i];
		const Vec3   toLight       = light.m_Position - P;
		const double lightDistance = Length( toLight );
		const Vec3   L             = toLight / lightDistance;

		const double NdotL = N * L;
		if( NdotL <= 0.0 ) continue;		// The light is behind the surface.

		// Hard shadows: is anything between the point and the light?
		Ray shadowRay;
		shadowRay.origin      = P + RayBias * N;
		shadowRay.direction   = L;
		shadowRay.no_emitters = false;
		HitInfo shadowHit;
		shadowHit.geom.distance = lightDistance;	// Only hits closer than the light count.
		if( Cast( shadowRay, scene, shadowHit ) ) continue;

		// Lambert diffuse.
		color += diffuseWeight * NdotL * ( mat.m_Diffuse * light.m_Color );

		// Phong specular: highlight where the mirrored light direction lines up with the viewer.
		if( mat.m_Phong_exp > 0.0f )
		{
			const Vec3   R     = Reflection( -L, N );
			const double RdotV = R * V;
			if( RdotV > 0.0 )
				color += pow( RdotV, mat.m_Phong_exp ) * ( mat.m_Specular * light.m_Color );
		}
	}

	// Secondary rays, while the ray tree is allowed to go deeper.
	if( max_tree_depth > 0 )
	{
		// Mirror reflection.
		if( reflectionWeight > 0.001 )
		{
			Ray reflected;
			reflected.origin      = P + RayBias * N;
			reflected.direction   = Unit( Reflection( D, N ) );
			reflected.no_emitters = false;
			color += reflectionWeight * Trace( reflected, scene, max_tree_depth );
		}

		// Refraction: the ray continues through the surface, bent by Snell's law.
		if( refracts && transmissionWeight > 0.001 )
		{
			Ray refracted;
			refracted.origin      = P - RayBias * N;
			refracted.direction   = Unit( T );
			refracted.no_emitters = false;
			color += transmissionWeight * Trace( refracted, scene, max_tree_depth );
		}
	}

	return color;
}
