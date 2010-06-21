/*
 * Bowl.cpp
 *
 *  Created on: Dec 30, 2009
 *      Author: andreborrmann
 */

#define SEGMENT_DEGREES 15

extern "C" {
#include <malloc.h>
#include <math.h>
#include <pspgu.h>
#include <pspgum.h>
}

#include "Bowl.h"
#include "ClVectorHelper.h"

ClBowl::ClBowl(ScePspFVector4* centralPos, float radius){
	// TODO Auto-generated constructor stub
	position = *centralPos;
	collisionSphereRadius = radius;
	generateMesh();

	// set the initial speed to 0
	speed = 0.0f;
	accel = 0.0f;
	inertia = 0.1f;
	touchPlaneId = -1;
	//set initial direction to 0
	direction.x = direction.y = direction.z = direction.w = 0.0f;
	acceleration.x = acceleration.y = acceleration.z = acceleration.w = 0.0f;
	worldMoveX.x = worldMoveX.y = worldMoveX.z = 0.0f;
	worldMoveZ.x = worldMoveZ.y = worldMoveZ.z = 0.0f;

	texture = monzoom::ClTextureMgr::getInstance()->loadFromPNG("img\\Bowl.png");
	rollAxis.x = 0.0f;
	rollAxis.y = 0.0f;
	rollAxis.z = 0.0f;
	/*rolling = 0.0f;
	*/
	gumLoadIdentity(&rollMatrix);
}

ClBowl::~ClBowl() {
	// TODO Auto-generated destructor stub
	free(mesh);
}

/*
 * move the bowl. as this method will be executed 10 times per sec
 * we apply the time dependend physics accordingly
 */
void ClBowl::move(float gravity){
	/*
	 * calculating the next position of the bowl
	 * If the bowl is not on a plane it would just fall down
	 * But if it is on a plane the orientation of the plane
	 * will set the direction of the bowls movement
	 */
	/*
	bool touchPlane = false;
	//check if the new position is colliding with a plane
	for (unsigned int i = 0; i < planeCount; i++){
		//optimization:
		//check this plane only if the bowl is close
		//enough (inside the bounding sphere)
		if (planes[i]->getDistanceTo(&position)> planes[i]->sphere) continue;
		//if we are above the plane than we need to check for collision
		//above means that the current position is inside the 4 edges
		//the 4 edges are bl, tl, br, tr
		//collusion check only if the plane is active
		if (planes[i]->isAbovePlane(&position) && planes[i]->getType()!=MB_PLANE_INACTIVE) {
			//get the distance between the plane and the bowl
			float skalP = position.x*planes[i]->normal.x;
			skalP += position.y*planes[i]->normal.y;
			skalP += position.z*planes[i]->normal.z;

			skalP += planes[i]->normal.w;

			//do not check exactly against the sphere radius as
			//we need some error tolerance ....
			if (skalP > 0.0f && skalP <= collisionSphereRadius + 0.01f) {
				touchPlane = true;
				//store the plane ID if it differs from the last one
				//and calculate the new direction
				if (i != touchPlaneId){
					//if we touch this plane the first time
					//it could be that we are touching it as a result of
					//falling down. Initialize the Y direction to zero
					direction.y = 0.0f;
					touchPlaneId = i;
					//if this plane was a switch plane we need to activate
					//the switch
					if (planes[i]->getType()==MB_PLANE_SWITCH_INACTIVE){
						planes[i]->setType(MB_PLANE_SWITCH_ACTIVE);
					}
					//the distance to this plane is to less
					//place the bowl at a position to the plane that the
					//sphere touches the plane
					float dist = collisionSphereRadius - skalP;
					position.x += dist*planes[i]->normal.x;
					position.y += dist*planes[i]->normal.y;
					position.z += dist*planes[i]->normal.z;


					 * if the normal of the plane is parallel to the
					 * gravity (or very close to) the bowl will no longer
					 * move into the Y direction downwards
					 * the acceleration will be set to zero

					if   ( -0.00001f <= planes[i]->normal.x
						&&  0.00001f >= planes[i]->normal.x
						&& -0.00001f <= planes[i]->normal.z
						&&  0.00001f >= planes[i]->normal.z){

						direction.y = 0.0f;
						acceleration.x = acceleration.y = acceleration.z = acceleration.w = 0.0f;
					} else {


						 * in the case that the bowl is on this plane
						 * the plane will influence the bowls direction
						 * this new direction vector is the cross product
						 * of normal and gravity and the cross product of this result
						 * with the normal again
						 * the gravity is only directed in y dircetion with -1

						ScePspFVector4 normal;
						ScePspFVector4 gravity = {0.0f, -1.0f, 0.0f, 0.0f};
						normal.x = planes[i]->normal.y*gravity.z - planes[i]->normal.z*gravity.y;
						normal.y = planes[i]->normal.z*gravity.x - planes[i]->normal.x*gravity.z;
						normal.z = planes[i]->normal.x*gravity.y - planes[i]->normal.y*gravity.x;

						acceleration.x = normal.y*planes[i]->normal.z - normal.z*planes[i]->normal.y;
						acceleration.y = normal.z*planes[i]->normal.x - normal.x*planes[i]->normal.z;
						acceleration.z = normal.x*planes[i]->normal.y - normal.y*planes[i]->normal.x;

						acceleration.w = 0.0f;
						//scale the acceleration vector
						ClVectorHelper::scale(&acceleration, 0.2f);

						//if the bowl is currently moving it should continue the same, but
						//on the new plane.
						//first get the current length of the direction (speed)
						float speed = ClVectorHelper::length(&direction);
						if (speed > 0.0f){
							//we assume first that the movement continue in Z and X
							//direction and dependen on the plane orientation
							//we will calculate the Y value.
							ScePspFVector4 newDir;
							newDir.x = direction.x;
							newDir.z = direction.z;
							newDir.y = (-planes[i]->normal.x*direction.x - planes[i]->normal.z*direction.z)/planes[i]->normal.y;
							//normalize the new direction vector
							ClVectorHelper::normalize(&newDir);

							//scale the vector with the original speed
							ClVectorHelper::scale(&newDir, speed);
							direction = newDir;
						}
					}
				} else {
				}
			}
		}
	}

	if (!touchPlane) {
		//the bowl is not touching any plane.
		//Apply only gravity as acceleration to the bowl
		acceleration.y = -gravity/4.0f;//10.0f;
		acceleration.x = acceleration.z = 0.0f;
		//no last touched plane
		touchPlaneId = -1;
		inertia = 0.0f;
		//rollAxis.x = rollAxis.y = rollAxis.z = 0.0f;
	} else {
		//if we have touched a plane the inertia will be set
		//and can be dependend from the material of the plane
		//initially it is fix
		inertia = 0.015f;
		//only if we have touched a plane the stick could change the
		//acceleration
		//the orientation of the touching plane would change the movements of the stick
		//calculate the Y movement from the planes orientation
		worldMoveX.y = (-planes[touchPlaneId]->normal.x*worldMoveX.x - planes[touchPlaneId]->normal.z*worldMoveX.z)/planes[touchPlaneId]->normal.y;
		worldMoveZ.y = (-planes[touchPlaneId]->normal.x*worldMoveZ.x - planes[touchPlaneId]->normal.z*worldMoveZ.z)/planes[touchPlaneId]->normal.y;
		//check whether it's better to add stick movement to
		//the acceleration or the position...
		direction.x += (worldMoveX.x*accelX + worldMoveZ.x*accelZ)/40.0f;
		direction.y += (worldMoveX.y*accelX + worldMoveZ.y*accelZ)/40.0f;
		direction.z += (worldMoveX.z*accelX + worldMoveZ.z*accelZ)/40.0f;

		//slow the bowl down by the inertia at each step to make sure
		//it will stop some when on a horizontal plane
		ClVectorHelper::scale(&direction, 1.0f-inertia);

	}


	//the current acceleration will be added to
	//the current direction of the bowl and than
	//furthermore added to the position of the same
	direction.x+= acceleration.x;
	direction.y+= acceleration.y;
	direction.z+= acceleration.z;

	position.x += direction.x;
	position.y += direction.y;
	position.z += direction.z;

	if (touchPlane){
		//the target direction vector will be used now to calculate the
		//rotation axis of the bowl
		ClVectorHelper::crossProduct(&rollAxis, &direction, &planes[touchPlaneId]->normal);
	}
	*/
}

void ClBowl::applyAcceleration(short  padX, short  padY, ScePspFMatrix4* viewMatrix){
	//if the stick may not be exactly in the center
	//make sure that some small amount of movement
	//has no impact
	if (padX == 1 || padX == -1) padX = 0;
	if (padY == 1 || padY == -1) padY = 0;
	this->accelX = (float)padX / 5.0f;
	this->accelZ = (float)padY / 5.0f;

	//calculate the worldspace movement into X and Z direction dependent
	//on the view matrix. we are translating pure axis movement into
	//worldspace
	ScePspFVector4 moveX = {1.0f, 0.0f, 0.0f, 0.0f};
	ScePspFVector4 moveZ = {0.0f, 0.0f, 1.0f, 0.0f};

	//invert the view matrix to make sure we are using the right
	//orientation of the axis
	ScePspFMatrix4 viewMatrixInv;
	gumFastInverse(&viewMatrixInv, viewMatrix);

	worldMoveX.x = viewMatrixInv.x.x*moveX.x; //nothing more as the rest is 0
	worldMoveX.y = 0.0f;//viewMatrix->x.y*moveX.x; // "
	worldMoveX.z = viewMatrixInv.x.z*moveX.x; // "

	worldMoveZ.x = viewMatrixInv.z.x*moveZ.z; //nothing more as the rest is 0
	worldMoveZ.y = 0.0f;//viewMatrix->z.y*moveZ.z; // "
	worldMoveZ.z = viewMatrixInv.z.z*moveZ.z; // "
}

void ClBowl::getPosition(ScePspFVector4 *pos){
	*pos = position;
}

int ClBowl::getTouchedPlaneId(){
	return touchPlaneId;
}

int ClBowl::getTouchedPlaneType(){
/*	if (touchPlaneId >= 0 )
		return planes[touchPlaneId]->getType();
	else
		return -1;
		*/
	return -1;
}

void ClBowl::generateMesh(){
	// the bowl will be a sphere...
	//create the mesh for the sphere
	int vertex = 0;
	unsigned int vertexCount = 0;

	//calculate the vertices needed for the sphere
	vertexCount = (360/SEGMENT_DEGREES + 1)* 180/SEGMENT_DEGREES * 2;
	//calculate the number of vertices needed for 1 stripe
	vertexCountStripe = (360/SEGMENT_DEGREES) + 1;
	//get memory
	mesh = (NormalTexColFVertex*)memalign(16, vertexCount*2*sizeof(NormalTexColFVertex));

	float ex, ey, ez;

	stripes = 0;
	//generate sphere  mesh
	for (int ha=0;ha<180;ha+=SEGMENT_DEGREES){
		for (int la=0;la<=360;la+=SEGMENT_DEGREES){
			ex = sinf(la*GU_PI/180.0f)*sinf(ha*GU_PI/180.0f);
			ey = cosf(ha*GU_PI/180.0f);
			ez = cosf(la*GU_PI/180.0f)*sinf(ha*GU_PI/180.0f);

			mesh[vertex].color = 0xffff3300;
			mesh[vertex].z = collisionSphereRadius*ez;
			mesh[vertex].x = collisionSphereRadius*ex;
			mesh[vertex].y = collisionSphereRadius*ey;

			mesh[vertex].nx = ex;
			mesh[vertex].ny = ey;
			mesh[vertex].nz = ez;

			mesh[vertex].u = (atan2f(ex, ez)/(GU_PI*2));
			//u starts at 0 goes to 0.5 and starts at -0.5 and goes to 0
			//we need to set this to go from 0 to 1
			if (mesh[vertex].u < 0.0f) mesh[vertex].u += 1.0f;
			//if we are close to 360° the u component goes from closely 1 back to 0
			//this results in texture cracks, we need to avoid this
			if(la>350 && mesh[vertex].u < 0.75) mesh[vertex].u += 1.0f;
			mesh[vertex].v = (asinf(ey)/GU_PI) + 0.5f;

			ex = sinf(la*GU_PI/180.0f)*sinf((ha+SEGMENT_DEGREES)*GU_PI/180.0f);
			ey = cosf((ha+SEGMENT_DEGREES)*GU_PI/180.0f);
			ez = cosf(la*GU_PI/180.0f)*sinf((ha+SEGMENT_DEGREES)*GU_PI/180.0f);

			mesh[vertex+1].color = 0xffff3300;
			mesh[vertex+1].z = collisionSphereRadius*ez;
			mesh[vertex+1].x = collisionSphereRadius*ex;
			mesh[vertex+1].y = collisionSphereRadius*ey;

			mesh[vertex+1].nx = ex;
			mesh[vertex+1].ny = ey;
			mesh[vertex+1].nz = ez;

			mesh[vertex+1].u = (atan2f(ex, ez)/(GU_PI*2));
			if (mesh[vertex+1].u < 0.0f) mesh[vertex+1].u += 1.0f;
			if(la>350 && mesh[vertex+1].u < 0.75) mesh[vertex+1].u += 1.0f;
			mesh[vertex+1].v = (asinf(ey)/GU_PI) + 0.5f;

			vertex+=2;
		}

		stripes++;
	}
}

void ClBowl::render(bool withTexture){
	float rSin, rCos,r1Cos, roll;
	// only initialize the MODEL Matrix if needed
	sceGumMatrixMode(GU_MODEL);
	sceGumLoadIdentity();

	ScePspFVector3 renderPos;
	renderPos.x = position.x;
	renderPos.y = position.y;
	renderPos.z = position.z;

	sceGumTranslate(&renderPos);
	//in addition to the movement of the bowl we need to rotate the same
	//for this we have the rotation axis in place
	//normalize the rotation axis
	roll = ClVectorHelper::length(&rollAxis);
	roll = ClVectorHelper::length(&rollAxis);
	roll = ClVectorHelper::length(&rollAxis);
	if (roll > 0.0001f){
		//the real roll length is the length of the
		//direction vector
		roll  = ClVectorHelper::length(&direction)*this->collisionSphereRadius*5.0f;
		ClVectorHelper::normalize(&rollAxis);

		rSin = sinf(roll*GU_PI/180.0f);
		rCos = cosf(roll*GU_PI/180.0f);
		r1Cos = 1 - rCos;
		ScePspFMatrix4 rotMatrix =
				{{rollAxis.x*rollAxis.x*(r1Cos)+rCos, rollAxis.x*rollAxis.y*(r1Cos)-rollAxis.z*rSin, rollAxis.x*rollAxis.z*(r1Cos)+rollAxis.y*rSin, 0.0f},
				 {rollAxis.y*rollAxis.x*(r1Cos)+rollAxis.z*rSin, rollAxis.y*rollAxis.y*(r1Cos)+rCos, rollAxis.y*rollAxis.z*(r1Cos)-rollAxis.x*rSin, 0.0f},
				 {rollAxis.x*rollAxis.z*(r1Cos)+rollAxis.y*rSin, rollAxis.y*rollAxis.z*(r1Cos)+rollAxis.x*rSin, rollAxis.z*rollAxis.z*(r1Cos)+rCos, 0.0f},
				 {0.0f, 0.0f, 0.0f, 1.0f}
				};
		//apply rotation matrix
		ScePspFMatrix4 identity, rot;
		gumLoadIdentity(&identity);
		gumMultMatrix(&rot, &identity, &rotMatrix);
		gumMultMatrix(&identity, &rot, &rollMatrix);
		rollMatrix = identity;
	}
	sceGumMultMatrix(&rollMatrix);

	if (texture && withTexture){
		sceGuEnable(GU_TEXTURE_2D);
		sceGuTexProjMapMode(GU_UV);
		sceGuTexMapMode(GU_TEXTURE_COORDS, 0, 0);
		sceGuTexImage(0, texture->width, texture->height, texture->stride, (void*) texture->data);
		sceGuTexMode(texture->type, 0, 0, texture->swizzled);
		sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGB);
		sceGuTexFilter(GU_LINEAR, GU_LINEAR);
		sceGuTexWrap(GU_REPEAT,GU_REPEAT);
		sceGuTexScale(1.0f, 1.0f);
		sceGuTexOffset(0.0f, 0.0f);
	} else
		sceGuDisable(GU_TEXTURE_2D);

	NormalTexColFVertex* drawMesh;

	sceGuFrontFace(GU_CCW);
	for (unsigned int sd=0;sd<stripes;sd++){
		drawMesh = mesh + sd*vertexCountStripe*2;
		sceGumDrawArray(GU_TRIANGLE_STRIP,GU_NORMAL_32BITF|GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_3D,vertexCountStripe*2,0,drawMesh);
	}
	sceGuFrontFace(GU_CW);
}




