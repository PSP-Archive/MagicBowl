/*
 * ClTrianglePlane.h
 *
 *  Simple class representing a plane of an object
 *  which is usually a triangle and mode of 3 vertices
 */

#ifndef CLTRIANGLEPLANE_H_
#define CLTRIANGLEPLANE_H_

extern "C"{
#include <psptypes.h>
}

class ClTrianglePlane {
public:
	ScePspFVector4 normal;
	ScePspFVector4 origin;

	ClTrianglePlane(ScePspFVector4* v1, ScePspFVector4* v2, ScePspFVector4* v3);
	virtual ~ClTrianglePlane();

	bool isFrontFaceTo(ScePspFVector4* direction);
	float getDistanceTo(ScePspFVector4* pos);

protected:
	static unsigned int counter;
};

#endif /* CLTRIANGLEPLANE_H_ */
