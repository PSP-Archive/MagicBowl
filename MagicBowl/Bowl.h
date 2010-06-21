/*
 * Bowl.h
 *
 *  Created on: Dec 30, 2009
 *      Author: andreborrmann
 */

#ifndef CLBOWL_H_
#define CLBOWL_H_

#include "psptypes3d.h"
#include "ClTextureMgr.h"

using namespace monzoom;

class ClBowl {
public:
	ClBowl(ScePspFVector4* centralPos, float radius);
	virtual ~ClBowl();

	/*
	 * render this object
	 */
	void render(bool withTexture);

	/*
	 * move the bowl
	 */
	void move(float gravity);

	/*
	 * set the acceleration based on PSP stick movements
	 * this will change the movements of the bowl in X and Z direction
	 * of the world space. The Y (up/down) will be influenced by
	 * the gravity and the planes the bowl is rolling on
	 */
	void applyAcceleration(short padX, short padY, ScePspFMatrix4* viewMatrix);

	/*
	 * return the current position of the bowl
	 */
	void getPosition(ScePspFVector4* pos);

	/*
	 * returns the last touched plane ID
	 */
	int getTouchedPlaneId();

	/*
	 * returns plane type the bowl touches or -1
	 */
	int getTouchedPlaneType();

protected:
	NormalTexColFVertex* mesh;
	monzoom::Texture* texture;
	unsigned int vertexCountStripe;
	unsigned int stripes;
	int touchPlaneId;

	float collisionSphereRadius;
	float inertia; //inertia (trägheit) of the bowl
	float speed;   //current speed of the bowl
	float accel;   //acceleration
	float accelX, accelZ; //acceleration set by the user actions
	//the vectors describing the current acceleration
	//direction and acceleration amount as well as
	//the current movement direction with its speed (length of the same)
	ScePspFVector4 position, acceleration, direction, rollAxis ;
	ScePspFMatrix4 rollMatrix;
	//float rolling;
	//the vector describing the movement on X-axis of stick in the worlds X axis
	ScePspFVector4 worldMoveX;
	//the vector describing the movement on the Y-axis of the stick in the worlds Z axis
	ScePspFVector4 worldMoveZ;

	ScePspFVector4 planeMoveX, planeMoveZ;

	void generateMesh();
};

#endif /* CLBOWL_H_ */
