/*
 * ClBowlPlayer.h
 *
 *  Created on: Apr 9, 2010
 *      Author: andreborrmann
 */

#ifndef CLBOWLPLAYER_H_
#define CLBOWLPLAYER_H_

#include <ClStaticSceneObject.h>
#include "ClTrianglePlane.h"

using namespace monzoom;

class ClBowlPlayer : public monzoom::ClStaticSceneObject {
public:
	ClBowlPlayer(ScePspFVector4* centralPos, float radius, ClSceneObject* world);
	virtual ~ClBowlPlayer();
	/*
	 * set the acceleration based on PSP stick movements
	 * this will change the movements of the bowl in X and Z direction
	 * of the world space. The Y (up/down) will be influenced by
	 * the gravity and the planes the bowl is rolling on
	 */
	void applyAcceleration(short padX, short padY, ScePspFMatrix4* viewMatrix);

	void applyBoost(float boost);

	/*
	 * move the bowl, this will also do the collision detection
	 * and response calculation
	 * @return - the object the bowl collides with
	 */
	ClSceneObject* move(float gravity);

	const ScePspFVector4 * getPosition();

	virtual void render();

	void doShadow(std::vector<ClSceneLight*>* lightList);
	void castShadowVolume(ScePspFVector4* lightPos, bool useOnly);

protected:
	//definition of data provided by collision detection
	//and needed for response calculation
	typedef struct CollisionInfo{
		bool foundColl;
		float collTime;
		float nearestDistance;
		ScePspFVector4 collPoint;
		ClTrianglePlane* collPlane;
		ClSceneObject* collObject;
		ClSceneObject* lastCollObject;
	}CollisionInfo;

	CollisionInfo collInfo;

	void generateMesh(short degrees_of_segments, float radius);
	bool collisionDetection(CollisionInfo* collInfo, ScePspFVector4* direction, vector<monzoom::ClSceneObject*> objectList);
	void collisionHandling(CollisionInfo* collInfo, float gravityValue);
	void checkTriangle(CollisionInfo* collInfo, ScePspFVector4* tr1, ScePspFVector4* tr2, ScePspFVector4* tr3, ScePspFVector4* direction);
	bool getLowestRoot(float a, float b, float c, float maxR, float* root);
	void updateRotation();
private:
	ClSceneObject* world; //master object representing the world - and all objects in it
	float accelX, accelZ; //acceleration set by the user actions
	//the vector describing the movement on X-axis of stick in the worlds X axis
	ScePspFVector4 worldMoveX;
	//the vector describing the movement on the Y-axis of the stick in the worlds Z axis
	ScePspFVector4 worldMoveZ;
	ScePspFVector4 acceleration, direction, rollAxis;
	ScePspFMatrix4 rollMatrix;
};

#endif /* CLBOWLPLAYER_H_ */
