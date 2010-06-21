/*
 * ClTrianglePlane.cpp
 *
 *  Created on: Apr 11, 2010
 *      Author: andreborrmann
 */

#include "ClTrianglePlane.h"
#include <ClVectorHelper.h>

unsigned int ClTrianglePlane::counter = 0;

ClTrianglePlane::ClTrianglePlane(ScePspFVector4 *v1, ScePspFVector4 *v2, ScePspFVector4 *v3){
//construct plane from 3 given points
	ScePspFVector4 side1 = {v2->x - v1->x,
							v2->y - v1->y,
							v2->z - v1->z, 0.0f};
	ScePspFVector4 side2 = {v3->x - v1->x,
			                v3->y - v1->y,
			                v3->z - v1->z, 0.0f};

	ClVectorHelper::crossProduct(&normal, &side2, &side1);
	ClVectorHelper::normalize(&normal);

	origin.x = v1->x;
	origin.y = v1->y;
	origin.z = v1->z;
	origin.w = 0.0f;

	normal.w = -ClVectorHelper::dotProduct(&normal, &origin);

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
