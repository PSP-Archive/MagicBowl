/*
 * ClTrianglePlane.cpp
 *
 *  Created on: Apr 11, 2010
 *      Author: andreborrmann
 */

#include "ClTrianglePlane.h"
#include <ClVectorHelper.h>

extern "C"{
#include <malloc.h>
}

unsigned int ClTrianglePlane::counter = 0;

ClTrianglePlane::ClTrianglePlane(ScePspFVector4 *v1, ScePspFVector4 *v2, ScePspFVector4 *v3){
//construct plane from 3 given points
	//for VFPU we need to allocate mem instead of using stack variable
	ScePspFVector4 *side1 = (ScePspFVector4*)memalign(16, sizeof(ScePspFVector4));
	ScePspFVector4 *side2 = (ScePspFVector4*)memalign(16, sizeof(ScePspFVector4));
	/*
	ScePspFVector4 side1 = {v2->x - v1->x,
							v2->y - v1->y,
							v2->z - v1->z, 0.0f};
	ScePspFVector4 side2 = {v3->x - v1->x,
			                v3->y - v1->y,
			                v3->z - v1->z, 0.0f};
*/
	side1->x = v2->x - v1->x;
	side1->y = v2->y - v1->y;
	side1->z = v2->z - v1->z;
	side1->w = 0.0f;

	side2->x = v3->x - v1->x;
	side2->y = v3->y - v1->y;
	side2->z = v3->z - v1->z;
	side2->w = 0.0f;

	ClVectorHelper::crossProduct(&normal, side2, side1);
	ClVectorHelper::normalize(&normal);

	origin.x = v1->x;
	origin.y = v1->y;
	origin.z = v1->z;
	origin.w = 0.0f;

	normal.w = -ClVectorHelper::dotProduct(&normal, &origin);

	free(side1);
	free(side2);
	counter++;
}

ClTrianglePlane::~ClTrianglePlane() {
	// TODO Auto-generated destructor stub
	counter--;
}

bool ClTrianglePlane::isFrontFaceTo(ScePspFVector4 *direction){
	return (ClVectorHelper::dotProduct(&normal, direction) <= 0);
}

float ClTrianglePlane::getDistanceTo(ScePspFVector4 *pos){
	return ClVectorHelper::dotProduct(pos, &normal) + normal.w;
}
