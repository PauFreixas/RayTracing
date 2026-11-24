#include "World.h"

bool World::readScene( const char *filename )
{
	Reader r;
	
	return r.ReadSceneDescription( filename , sce , cam );
}

const Camera &World::getCamera( void ) const
{
	return cam;
}

const Scene &World::getScene( void ) const
{
	return sce;
}