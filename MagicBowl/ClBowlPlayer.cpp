/*
 * ClBowlPlayer.cpp
 *
 *  Created on: Apr 9, 2010
 *      Author: andreborrmann
 */

extern "C"{
#include <math.h>
#include <malloc.h>
#include <string.h>
#include <pspgu.h>
#include <pspgum.h>
#include <psptypes3d.h>
}

#include <MonzoomAPI.h>
#include <ClMonzoomToGU.h>
#include <ClAnimMaterialtexture.h>
#include <ClMaterialTexture.h>
#include "ClBowlPlayer.h"
#include "ClAnimatedSceneObject.h"
#include <ClVectorHelper.h>
#include <typeinfo>

using namespace monzoom;

ClBowlPlayer::ClBowlPlayer(ScePspFVector4* centralPos, float radius, ClSceneObject* world)
             :ClStaticSceneObject("Player"){
	// TODO Auto-generated constructor stub

	this->pos = *centralPos;
	boundingSphere = radius;
	generateMesh(20, radius);

	//set initial direction to 0
	direction.x = direction.y = direction.z = direction.w = 0.0f;
	acceleration.x = acceleration.y = acceleration.z = acceleration.w = 0.0f;
	worldMoveX.x = worldMoveX.y = worldMoveX.z = 0.0f;
	worldMoveZ.x = worldMoveZ.y = worldMoveZ.z = 0.0f;

	this->world = world;

	collInfo.foundColl = false;
	collInfo.collPlane = 0;
	collInfo.collTime = 0.0f;
	collInfo.nearestDistance = 999999999.0f;
	collInfo.lastCollObject = 0;
	//initialize the roll animation of the bowl
	rollAxis.x = rollAxis.y = rollAxis.z = 0.0f;
	gumLoadIdentity(&rollMatrix);
}

ClBowlPlayer::~ClBowlPlayer() {
	// TODO Auto-generated destructor stub

}

void ClBowlPlayer::generateMesh(short degrees_of_segments, float radius){
	//load the mesh from a monzoom file
	ClMonzoomFile *mzFile = new ClMonzoomFile("data/Bowl.mon");
	std::vector<ClChunk*> objectList;
	ClChunkDre2 *mzObject;
	int objectCount;
	float boundingSphereRadius;
	unsigned int vertexCount;
	NormalTexFVertex *mesh;
	TriangleIdx* triangles;
	ScePspFVector4 pos;
	int i;

	//search for the bowl object in this file
	mzFile->getChunksByType(&objectList, &objectCount, "DRE2");
	for(i = 0;i < objectList.size();i++){
		mzObject = (ClChunkDre2*)(objectList[i]);
		if (strcmp(mzObject->rge1->robj->name.str,"Bowl") == 0){
			//load the mesh from this object
			vertexCount = ClMonzoomToGU::createObjectMesh(&mesh, &triangles, &pos, &boundingSphereRadius, mzObject, mzFile);
			this->setMesh(mesh, vertexCount, triangles);
			//this->setPosition(pos);
			this->setBoundingSphere(boundingSphereRadius);

			if(mzObject->matBodyCount != 0){
				// check for animated material (currently only the color component will
				// be considered
				std::vector<ColorAnimation> anim = ClMonzoomToGU::getMaterialColorAnimation(mzObject, mzFile);
				if (!anim.empty()){
					if (mzObject->matBodyList[0].text_obj_flag){
						ClAnimMaterialTexture *texture = new ClAnimMaterialTexture();
						texture->setTexture(ClMonzoomToGU::getMaterialTexture(mzObject, mzFile));
						texture->setColor(ClMonzoomToGU::getMaterialColor(mzObject, mzFile));
						texture->setAnimation(anim);
						this->setMaterial(texture);
					}else{
						//this is an animated material, create an animated material
						//object and set the animation data
						ClAnimMaterialColor* material = new ClAnimMaterialColor();
						material->setColor(ClMonzoomToGU::getMaterialColor(mzObject, mzFile));
						material->setAnimation(anim);
						this->setMaterial(material);
					} //texture flag ?
				}else{
					if(mzObject->matBodyList[0].text_obj_flag){
						ClMaterialTexture *texture = new ClMaterialTexture();
						texture->setTexture(ClMonzoomToGU::getMaterialTexture(mzObject, mzFile));
						texture->setColor(ClMonzoomToGU::getMaterialColor(mzObject, mzFile));
						this->setMaterial(texture);
					}else{
						ClMaterialColor *material = new ClMaterialColor();
						material->setColor(ClMonzoomToGU::getMaterialColor(mzObject, mzFile));
						this->setMaterial(material);
					} //texture flag ?
				} //anim material?
			}

			break;
		}
	}

	objectList.clear();
	delete(mzFile);

	return;
/*
	unsigned int      triangles = 0,
					  vertexCount = 0,
					  vertex = 0;

	Mesh mesh;
	//calculate total number of vertices
	triangles = (360/degrees_of_segments)* 180/degrees_of_segments * 2;
	mesh.vertexCount = triangles*3;
	//get memory
	mesh.mesh = (NormalTexFVertex*)memalign(16, mesh.vertexCount*sizeof(NormalTexFVertex));
	mesh.triangles = (TriangleIdx*)malloc(sizeof(TriangleIdx)*triangles);

	float ex, ey, ez;

	//generate sphere  mesh
	for (int ha=0;ha<180;ha+=degrees_of_segments){
		for (int la=0;la<360;la+=degrees_of_segments){
			ex = sinf(la*GU_PI/180.0f)*sinf(ha*GU_PI/180.0f);
			ey = cosf(ha*GU_PI/180.0f);
			ez = cosf(la*GU_PI/180.0f)*sinf(ha*GU_PI/180.0f);

			mesh.mesh[vertex].z = radius*ez;
			mesh.mesh[vertex].x = radius*ex;
			mesh.mesh[vertex].y = radius*ey;

			mesh.mesh[vertex].nx = ex;
			mesh.mesh[vertex].ny = ey;
			mesh.mesh[vertex].nz = ez;

			mesh.mesh[vertex].u = (atan2f(ex, ez)/(GU_PI*2));
			//u starts at 0 goes to 0.5 and starts at -0.5 and goes to 0
			//we need to set this to go from 0 to 1
			if (mesh.mesh[vertex].u < 0.0f) mesh.mesh[vertex].u += 1.0f;
			//if we are close to 360° the u component goes from closely 1 back to 0
			//this results in texture cracks, we need to avoid this
			if(la>=(360-degrees_of_segments) && mesh.mesh[vertex].u < 0.75) mesh.mesh[vertex].u += 1.0f;
			mesh.mesh[vertex].v = (asinf(ey)/GU_PI) + 0.5f;

			ex = sinf(la*GU_PI/180.0f)*sinf((ha+degrees_of_segments)*GU_PI/180.0f);
			ey = cosf((ha+degrees_of_segments)*GU_PI/180.0f);
			ez = cosf(la*GU_PI/180.0f)*sinf((ha+degrees_of_segments)*GU_PI/180.0f);

			mesh.mesh[vertex+1].z = radius*ez;
			mesh.mesh[vertex+1].x = radius*ex;
			mesh.mesh[vertex+1].y = radius*ey;

			mesh.mesh[vertex+1].nx = ex;
			mesh.mesh[vertex+1].ny = ey;
			mesh.mesh[vertex+1].nz = ez;

			mesh.mesh[vertex+1].u = (atan2f(ex, ez)/(GU_PI*2));
			if (mesh.mesh[vertex+1].u < 0.0f) mesh.mesh[vertex+1].u += 1.0f;
			if(la>=(360-degrees_of_segments) && mesh.mesh[vertex+1].u < 0.75) mesh.mesh[vertex+1].u += 1.0f;
			mesh.mesh[vertex+1].v = (asinf(ey)/GU_PI) + 0.5f;

			ex = sinf((la+degrees_of_segments)*GU_PI/180.0f)*sinf(ha*GU_PI/180.0f);
			ey = cosf(ha*GU_PI/180.0f);
			ez = cosf((la+degrees_of_segments)*GU_PI/180.0f)*sinf(ha*GU_PI/180.0f);

			mesh.mesh[vertex+2].z = radius*ez;
			mesh.mesh[vertex+2].x = radius*ex;
			mesh.mesh[vertex+2].y = radius*ey;

			mesh.mesh[vertex+2].nx = ex;
			mesh.mesh[vertex+2].ny = ey;
			mesh.mesh[vertex+2].nz = ez;

			mesh.mesh[vertex+2].u = (atan2f(ex, ez)/(GU_PI*2));
			//u starts at 0 goes to 0.5 and starts at -0.5 and goes to 0
			//we need to set this to go from 0 to 1
			if (mesh.mesh[vertex+2].u < 0.0f) mesh.mesh[vertex+2].u += 1.0f;
			//if we are close to 360° the u component goes from closely 1 back to 0
			//this results in texture cracks, we need to avoid this
			if(la>=(360-degrees_of_segments) && mesh.mesh[vertex+2].u < 0.75) mesh.mesh[vertex+2].u += 1.0f;
			mesh.mesh[vertex+2].v = (asinf(ey)/GU_PI) + 0.5f;

			mesh.mesh[vertex+3] = mesh.mesh[vertex+1];

			ex = sinf((la+degrees_of_segments)*GU_PI/180.0f)*sinf((ha+degrees_of_segments)*GU_PI/180.0f);
			ey = cosf((ha+degrees_of_segments)*GU_PI/180.0f);
			ez = cosf((la+degrees_of_segments)*GU_PI/180.0f)*sinf((ha+degrees_of_segments)*GU_PI/180.0f);

			mesh.mesh[vertex+4].z = radius*ez;
			mesh.mesh[vertex+4].x = radius*ex;
			mesh.mesh[vertex+4].y = radius*ey;

			mesh.mesh[vertex+4].nx = ex;
			mesh.mesh[vertex+4].ny = ey;
			mesh.mesh[vertex+4].nz = ez;

			mesh.mesh[vertex+4].u = (atan2f(ex, ez)/(GU_PI*2));
			//u starts at 0 goes to 0.5 and starts at -0.5 and goes to 0
			//we need to set this to go from 0 to 1
			if (mesh.mesh[vertex+4].u < 0.0f) mesh.mesh[vertex+4].u += 1.0f;
			//if we are close to 360° the u component goes from closely 1 back to 0
			//this results in texture cracks, we need to avoid this
			if(la>=(360-degrees_of_segments) && mesh.mesh[vertex+4].u < 0.75) mesh.mesh[vertex+4].u += 1.0f;
			mesh.mesh[vertex+4].v = (asinf(ey)/GU_PI) + 0.5f;

			mesh.mesh[vertex+5] = mesh.mesh[vertex+2];

			vertex+=6;
		}
	}

	//create the triangle index
	for (int v=0;v<mesh.vertexCount;v++){
		mesh.triangles[v/3].index[v%3] = v;
	}

	ClSceneObject::setMesh(mesh.mesh, mesh.vertexCount, mesh.triangles);
	//meshList.push_back(mesh);
*/
}

void ClBowlPlayer::applyAcceleration(short  padX, short  padY, ScePspFMatrix4* viewMatrix){
	//if the stick may not be exactly in the center
	//make sure that some small amount of movement
	//has no impact
	if (padX == 1 || padX == -1) padX = 0;
	if (padY == 1 || padY == -1) padY = 0;
	this->accelX = (float)padX / 2.0f;
	this->accelZ = (float)padY / 2.0f;

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

	ClVectorHelper::normalize(&worldMoveX);
	ClVectorHelper::normalize(&worldMoveZ);
}

/*
 * apply the boost to the stick movements
 */
void ClBowlPlayer::applyBoost(float boost){
	ClVectorHelper::scale(&worldMoveX, 1.0f+boost);
	ClVectorHelper::scale(&worldMoveZ, 1.0f+boost);
}

ClSceneObject* ClBowlPlayer::move(float gravity){

	//apply the gravity, the stick movement
	//and any acceleration to the bowl

	//if we are currently not on plane, set gravity
	acceleration.y -= gravity/40.0f;//10.0f;
	//acceleration.x = acceleration.z = 0.0f;
	if (collInfo.foundColl){
		//the orientation of the touching plane would change the movements of the stick
		//calculate the Y movement from the planes orientation
		//but do this not if the touching plane is "senkrecht"
		if ( -0.00001f <= collInfo.collPlane->normal.y
			&& 0.00001f >= collInfo.collPlane->normal.y ){
			worldMoveX.y = 0.0f;
			worldMoveZ.y = 0.0f;
		} else {
			worldMoveX.y = (-collInfo.collPlane->normal.x*worldMoveX.x - collInfo.collPlane->normal.z*worldMoveX.z)/collInfo.collPlane->normal.y;
			worldMoveZ.y = (-collInfo.collPlane->normal.x*worldMoveZ.x - collInfo.collPlane->normal.z*worldMoveZ.z)/collInfo.collPlane->normal.y;
		}
		//add the stick movement to the current direction
		direction.x += (worldMoveX.x*accelX + worldMoveZ.x*accelZ)*0.04f;// /20.0f;
		direction.y += (worldMoveX.y*accelX + worldMoveZ.y*accelZ)*0.04f;// /20.0f;
		direction.z += (worldMoveX.z*accelX + worldMoveZ.z*accelZ)*0.04f;// /20.0f;

/*		//if the touched object is an animated one the object movement will impact the bowl movement
		const char* className = typeid(*collInfo.collObject).name();
		if (strstr(className, "ClAnimatedSceneObject") != 0){
			//if the object is an animated one we apply the object movement to the bowl as well
			const ScePspFVector4* oDir = ((ClAnimatedSceneObject*)collInfo.collObject)->getDirPerFrame();
			//if we touched this object the first time get the direction a second time to initialize the
			//direction "delta" if the object was already in move bevore we have touched it
			if (collInfo.lastCollObject != collInfo.collObject){
				oDir = ((ClAnimatedSceneObject*)collInfo.collObject)->getDirPerFrame();
				collInfo.lastCollObject = collInfo.collObject;
			}
			//add this direction to the bowl movement
			pos.x += oDir->x;
			pos.y += oDir->y;
			pos.z += oDir->z;
		}
		*/
	}
	direction.x+= acceleration.x;
	direction.y+= acceleration.y;
	direction.z+= acceleration.z;

	//now we knew the direction we want to move the bowl to. Check for any collision
	//in this way
	bool lastFoundColl = collInfo.foundColl;
	//before performing any new collision detection free the triangle
	//from the last call
	if (collInfo.collPlane){
		delete(collInfo.collPlane);
		collInfo.collPlane = 0;
	}
	if (this->collisionDetection(&collInfo, &direction, this->world->getChilds())){

		collisionHandling(&collInfo, gravity);
		//now check a second time whether the new pos and direction will
		//result again in a collision
		//store the current result
		CollisionInfo lastCollInfo = collInfo;
		if (this->collisionDetection(&collInfo, &direction, this->world->getChilds())){
			//again there is a collision occured
			//handle this one
			collisionHandling(&collInfo, gravity);
		} else {
			//no collision, just restore the default value
			collInfo = lastCollInfo;
		}
		//the target direction vector will be used now to calculate the
		//rotation axis of the bowl
		ClVectorHelper::crossProduct(&rollAxis, &direction, &collInfo.collPlane->normal);
	}
	{
		// move to the next position but slow down the movement a small peace each time
		ClVectorHelper::scale(&direction, 0.985f);
		pos.x += direction.x;
		pos.y += direction.y;
		pos.z += direction.z;
	}
	if (collInfo.foundColl){
		//if the touched object is an animated one the object movement will impact the bowl movement
		const char* className = typeid(*collInfo.collObject).name();
		if (strstr(className, "ClAnimatedSceneObject") != 0){
			//if the object is an animated one we apply the object movement to the bowl as well
			const ScePspFVector4* oDir = ((ClAnimatedSceneObject*)collInfo.collObject)->getDirPerFrame();
			//if we touched this object the first time get the direction a second time to initialize the
			//direction "delta" if the object was already in move bevore we have touched it
			if (collInfo.lastCollObject != collInfo.collObject)
				oDir = ((ClAnimatedSceneObject*)collInfo.collObject)->getDirPerFrame();
			//add this direction to teh bowl position
			pos.x += oDir->x;
			pos.y += oDir->y;
			pos.z += oDir->z;
		}
		if (collInfo.collObject != collInfo.lastCollObject)
			collInfo.lastCollObject = collInfo.collObject;

		updateRotation();
		return collInfo.collObject;
	}
	else {
		//if (collInfo.collObject != collInfo.lastCollObject)
		//currently there is no collision any more
		//if the last collision object was an animated one we need to check whether we need to move the bowl
		//with this object as it is just moving down (the bowl is lying on it)
		if (collInfo.lastCollObject){
			const char* className = typeid(*collInfo.lastCollObject).name();
			if (strstr(className, "ClAnimatedSceneObject") != 0){
				const ScePspFVector4* oDir = ((ClAnimatedSceneObject*)collInfo.collObject)->getDirPerFrame();
				//do we have a movement on y direction ?
				if (oDir->y < 0.0f){
					//set the bowl position based on the movement of the animated object
					pos.y += oDir->y;
					pos.x += oDir->x;
					pos.z += oDir->z;
				}
			}
		}
		collInfo.lastCollObject = 0;//collInfo.collObject;
		updateRotation();
		return 0;
	}

}

const ScePspFVector4* ClBowlPlayer::getPosition(){
	return &pos;
}

void ClBowlPlayer::render(){
	float rSin, rCos,r1Cos, roll;
	Material material;
	ScePspFMatrix4 *rot;

	sceGumMatrixMode(GU_MODEL);
	sceGumLoadIdentity();
	ScePspFVector3 renderPos;
	renderPos.x = pos.x;
	renderPos.y = pos.y;
	renderPos.z = pos.z;

	//apply movement and rotation to the bowl while rendering
	sceGumTranslate(&renderPos);
	sceGumMultMatrix(&rollMatrix);

	int i = 0;
	//running through all meshes
	for (int m=0;m<meshList.size();m++){
		//dependent of the material for this object we set
		//the color and/or texture
		monzoom::ClMaterialColor* mat = getMaterial();
		if (mat){
			sceGuColor(mat->getColor());
			sceGuModelColor(0x00111111, mat->getColor(), mat->getColor(), 0x00000000);
			if (mat->getType() == 1){
				//it is a texture material
				ClMaterialTexture* tex = (ClMaterialTexture*)mat;
				ClTextureMgr::getInstance()->useTexture(tex->getTexture());
				/*
				sceGuEnable(GU_TEXTURE_2D);
				sceGuTexMapMode(GU_TEXTURE_COORDS,0,0);//GU_TEXTURE_MATRIX, 0, 0); //texture mapping using matrix instead u, v
				sceGuTexProjMapMode(GU_UV); //texture mapped based on the position of the objects relative to the texture

				sceGuTexMode(tex->getTexture()->type, 0,0,tex->getTexture()->swizzled);
				sceGuTexImage(0,tex->getTexture()->width,tex->getTexture()->height,tex->getTexture()->stride, tex->getTexture()->data);
				sceGuTexOffset(0.0f, 0.0f);
				sceGuTexScale(1.0f, 1.0f);
				sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);//apply RGBA value of texture
				sceGuTexFilter(GU_LINEAR,GU_LINEAR); //interpolate to have smooth borders
				sceGuTexWrap(GU_REPEAT,GU_REPEAT); //do repeat the texture if necessary
				*/
			} else
				sceGuDisable(GU_TEXTURE_2D);
		} else
			sceGuColor(0xffffffff);

		sceGuFrontFace(GU_CW);
		sceGumDrawArray(GU_TRIANGLES,GU_TEXTURE_32BITF |GU_NORMAL_32BITF | GU_VERTEX_32BITF | GU_TRANSFORM_3D, meshList[m].vertexCount, 0, meshList[m].mesh);
		sceGuFrontFace(GU_CW);
		sceGuModelColor(0x00000000, 0x00000000, 0x00000000, 0x00000000);
	}
}

bool ClBowlPlayer::collisionDetection(CollisionInfo* collInfo, ScePspFVector4* direction, vector<monzoom::ClSceneObject*> objectList){
	//do collision detection of myself against a given object list
	collInfo->nearestDistance = 9999999.0f;
	collInfo->foundColl = false;
	collInfo->collPlane = 0;

	bool skipTriangle;
	ScePspFVector4 tr1, tr2, tr3;

	//check all objects, first check myself is near enough, meaning inside the
	//bounding spheres
	const ScePspFVector4* tPos;
	const ScePspFVector4* oPos;

	for (int t=0;t<objectList.size();t++){
		//do not check against the outbox and my self
		if (strstr(objectList[t]->getName(), "OutBox") != 0
			|| objectList[t] == this
			|| objectList[t]->getState()!= SOS_VISIBLE) continue;
		//the distance of the two objects is the length of
		//the vector between the positions - the new one of the current
		//and the static of the other one
		tPos = objectList[t]->getPosition();
		ScePspFVector4 dist = { pos.x + direction->x - tPos->x,
				                pos.y + direction->y - tPos->y,
				                pos.z + direction->z - tPos->z,
				                0.0f};
		float distance = ClVectorHelper::length(&dist);
		if (distance <= this->boundingSphere + objectList[t]->getBoundingSphere()){
			//I'm close enough to this object, now do the further checks
			//check each triangle of this object
			vector<monzoom::ClSceneObject::Mesh>* meshList = objectList[t]->getMesh();
			int objectTriangles = (*meshList)[0].vertexCount / 3;
			skipTriangle = false;
			//in case the object is an animated one we need to retrieve the relative
			//position
			const char* className = typeid(*objectList[t]).name();
			if (strstr(className, "ClAnimatedSceneObject") != 0){
				oPos = ((ClAnimatedSceneObject*)objectList[t])->getOriginalPosition();
			}
			else
				oPos = 0;
			for (int triangle=0;triangle<objectTriangles && !skipTriangle;triangle++){
				//create a plane out of this triangle
				tr1.x = (*meshList)[0].mesh[triangle*3].x;
				tr1.y = (*meshList)[0].mesh[triangle*3].y;
				tr1.z = (*meshList)[0].mesh[triangle*3].z;
				tr2.x = (*meshList)[0].mesh[triangle*3+1].x;
				tr2.y = (*meshList)[0].mesh[triangle*3+1].y;
				tr2.z =	(*meshList)[0].mesh[triangle*3+1].z;
				tr3.x = (*meshList)[0].mesh[triangle*3+2].x;
				tr3.y = (*meshList)[0].mesh[triangle*3+2].y;
				tr3.z =	(*meshList)[0].mesh[triangle*3+2].z;
				if (oPos){
					tr1.x += tPos->x - oPos->x;
					tr1.y += tPos->y - oPos->y;
					tr1.z += tPos->z - oPos->z;
					tr2.x += tPos->x - oPos->x;
					tr2.y += tPos->y - oPos->y;
					tr2.z += tPos->z - oPos->z;
					tr3.x += tPos->x - oPos->x;
					tr3.y += tPos->y - oPos->y;
					tr3.z += tPos->z - oPos->z;
				}
				//it is possible that the mesh contains dummy triangles
				//they are thin and at least two points are the same
				//this triangles should not be considered for collision detection
				if (ClVectorHelper::equal(&tr1, &tr2) ||
					ClVectorHelper::equal(&tr1, &tr3) ||
					ClVectorHelper::equal(&tr2, &tr3)) {
				} else {
					//if this triangle is part of an animated object we may need
					//to apply the changed object position to the triangle points which are static by nature

					bool lastFoundColl = collInfo->foundColl;
					collInfo->foundColl = false;
					checkTriangle(collInfo, &tr1, &tr2,&tr3, direction);
					//if this check returns a collision set the object
					if (collInfo->foundColl){
						collInfo->collObject = objectList[t];
						//we assume there could be only 1 triangle
						//of a single mesh collide..therefore continue with the next object
						skipTriangle = true;
					} else {
						//if no coll found just restore the last state
						collInfo->foundColl = lastFoundColl;
					}
				}
			}
		}
	}
	return collInfo->foundColl;
}

void ClBowlPlayer::checkTriangle(CollisionInfo *collInfo, ScePspFVector4 *tr1, ScePspFVector4 *tr2, ScePspFVector4 *tr3, ScePspFVector4* direction){
	//float collTime;
	ScePspFVector4 collPos;
	ClTrianglePlane* collPlane;
	ClTrianglePlane* tri = new ClTrianglePlane(tr1, tr2, tr3);
	bool collFound = false;
	if (!tri->isFrontFaceTo(direction)){
		delete(tri);
		return;
	}

	//get the distance from pos to traingle
	float distance = tri->getDistanceTo(&pos);
	float triDotDir = ClVectorHelper::dotProduct(&tri->normal, direction);

	float t0, t1;
	bool embeded = false;
	//the pos is moving parallel to the plane
	if (triDotDir >= -0.00000001f
		&& triDotDir <= 0.00000001f){
		//the sphere is not intersecting the plane
		//therefore no collision possible
		if (fabsf(distance) >= boundingSphere){
			delete(tri);
			return;
		} else {
			//the sphere intersects the plane complete
			t0 = 0.0f;
			t1 = 1.0f;
			embeded = true;
		}
	}else {
		//the movement is not parallel to the plane
		//calculate the time intervall where the sphere
		//intersects with the plane if moving along direction
		t0 = (-boundingSphere  - distance) / triDotDir;
		t1 = (boundingSphere - distance) / triDotDir;
		if (t0 > t1){
			float temp = t0;
			t0 = t1;
			t1 = temp;
		}
		//check that at least one value is inside 0.0-1.0 range
		if (t0 > 1.0f || t1 < 0.0f){
			//both values outside range, no collision possible
			delete(tri);
			return;
		}
		//make sure the values are
		if (t0 < 0.0f) t0 = 0.0f;
		if (t1 < 0.0f) t1 = 0.0f;
		if (t0 > 1.0f) t0 = 1.0f;
		if (t1 > 1.0f) t1 = 1.0f;

		//now we know that the sphere will intersect the triangle
		//between t0 and t1
		//first easy check -> the sphere intersects within the triangle
		//does mean rest on it, which occurs only on time t0
		ScePspFVector4 collPoint;
		float collTime = 1.0f;

		if (!embeded){
			ScePspFVector4 planeIntersect = {pos.x - tri->normal.x + t0*direction->x,
					                         pos.y - tri->normal.y + t0*direction->y,
					                         pos.z - tri->normal.z + t0*direction->z,
					                         0.0f };
			//check whether this point is inside the triangle
			if (ClVectorHelper::checkInsideTriangle(&planeIntersect, tr1, tr2, tr3)){
				collFound = true;
				collTime = t0;
				collPoint = planeIntersect;
				collPlane = tri;
			}
		}

		//if we do not have found any collision by now
		//we need to check whether the sphere just touches the sides
		//or the edges of the triangle
		//TODO: check against points and edges not yet implemented
		if (!collFound){
			/*
			float a, b, c;
			float newT;
			float dirSqrLength = ClVectorHelper::sqrLength(direction);
			//if no collision found until now, check for the
			//triangle edges

			//tr1->tr2
			ScePspFVector4 edge = {tr2->x - tr1->x, tr2->y - tr1->y, tr2->z - tr1->z, 0.0f};
			ScePspFVector4 pos2Tri = {tr1->x - pos.x, tr1->y - pos.y,tr1->z - pos.z, 0.0f};
			float edgeSqrLength = ClVectorHelper::sqrLength(&edge);
			float edgeDotDir = ClVectorHelper::dotProduct(&edge, direction);
			float edgeDotPos2Tri = ClVectorHelper::dotProduct(&edge, &pos2Tri);

			a = edgeSqrLength*-dirSqrLength + edgeDotDir*edgeDotDir;
			b = edgeSqrLength*(2*ClVectorHelper::dotProduct(direction, &pos2Tri)) - 2*edgeDotDir*edgeDotPos2Tri;
			c = edgeSqrLength*(1-ClVectorHelper::sqrLength(&pos2Tri)) + edgeDotPos2Tri*edgeDotPos2Tri;

			if (getLowestRoot(a, b, c, collTime, &newT)){
				//check if intersection point is within line segment
				float f = (edgeDotDir*newT - edgeDotPos2Tri)/edgeSqrLength;
				if (f >= 0.0f && f <= 1.0f){
					collTime = newT;
					collFound = true;
					collPoint.x = tr1->x + f*edge.x;
					collPoint.y = tr1->y + f*edge.y;
					collPoint.z = tr1->z + f*edge.z;
					collPoint.w = 0.0f;
				}
			}

			//tr2->tr3
			edge.x = tr3->x - tr2->x; edge.y = tr3->y - tr2->y;
			edge.z = tr3->z - tr2->z; edge.w = 0.0f;
			pos2Tri.x = tr2->x - pos.x; pos2Tri.y =  tr2->y - pos.y;
			pos2Tri.z = tr2->z - pos.z; pos2Tri.w = 0.0f;
			edgeSqrLength = ClVectorHelper::sqrLength(&edge);
			edgeDotDir = ClVectorHelper::dotProduct(&edge, direction);
			edgeDotPos2Tri = ClVectorHelper::dotProduct(&edge, &pos2Tri);

			a = edgeSqrLength*-dirSqrLength + edgeDotDir*edgeDotDir;
			b = edgeSqrLength*(2*ClVectorHelper::dotProduct(direction, &pos2Tri)) - 2*edgeDotDir*edgeDotPos2Tri;
			c = edgeSqrLength*(1-ClVectorHelper::sqrLength(&pos2Tri)) + edgeDotPos2Tri*edgeDotPos2Tri;

			if (getLowestRoot(a, b, c, collTime, &newT)){
				//check if intersection point is within line segment
				float f = (edgeDotDir*newT - edgeDotPos2Tri)/edgeSqrLength;
				if (f >= 0.0f && f <= 1.0f){
					collTime = newT;
					collFound = true;
					collPoint.x = tr2->x + f*edge.x;
					collPoint.y = tr2->y + f*edge.y;
					collPoint.z = tr2->z + f*edge.z;
					collPoint.w = 0.0f;
				}
			}

			//tr3->tr1
			edge.x = tr1->x - tr3->x; edge.y = tr1->y - tr3->y;
			edge.z = tr1->z - tr3->z; edge.w = 0.0f;
			pos2Tri.x = tr3->x - pos.x; pos2Tri.y =  tr3->y - pos.y;
			pos2Tri.z = tr3->z - pos.z; pos2Tri.w = 0.0f;
			edgeSqrLength = ClVectorHelper::sqrLength(&edge);
			edgeDotDir = ClVectorHelper::dotProduct(&edge, direction);
			edgeDotPos2Tri = ClVectorHelper::dotProduct(&edge, &pos2Tri);

			a = edgeSqrLength*-dirSqrLength + edgeDotDir*edgeDotDir;
			b = edgeSqrLength*(2*ClVectorHelper::dotProduct(direction, &pos2Tri)) - 2*edgeDotDir*edgeDotPos2Tri;
			c = edgeSqrLength*(1-ClVectorHelper::sqrLength(&pos2Tri)) + edgeDotPos2Tri*edgeDotPos2Tri;

			if (getLowestRoot(a, b, c, collTime, &newT)){
				//check if intersection point is within line segment
				float f = (edgeDotDir*newT - edgeDotPos2Tri)/edgeSqrLength;
				if (f >= 0.0f && f <= 1.0f){
					collTime = newT;
					collFound = true;
					collPoint.x = tr3->x + f*edge.x;
					collPoint.y = tr3->y + f*edge.y;
					collPoint.z = tr3->z + f*edge.z;
					collPoint.w = 0.0f;
				}
			}
			*/
		}
		//if we do have found a collision after all we return true
		if (collFound){
			//is this collision point closer to an object as the previous one
			//than take always the closest...
			float distToColl = ClVectorHelper::length(direction)*collTime;
			if (!collInfo->foundColl ||
				 distToColl < collInfo->nearestDistance){
				collInfo->foundColl = true;
				collInfo->collTime = collTime;
				collInfo->collPoint = collPoint;
				//the last collision triangle is not needed any further
				//free the instance...
				if (collInfo->collPlane) delete(collInfo->collPlane);
				collInfo->collPlane = tri;
				collInfo->nearestDistance = distToColl;
			}
		}
	}
	if (!collFound){
		//still no collision detected, we delete the instance of the triangle as it is no longer neede
		delete(tri);
	}
}

void ClBowlPlayer::collisionHandling(CollisionInfo* collInfo, float gravityValue){
	//we now knew the triangle which we collide with and have already moved
	//to the collision point, now do the collision response step
	//move to the collision point (but only very close to and not exactly)
	//if we are not already very close to
	if (collInfo->nearestDistance >= 0.0005f){
		pos.x += direction.x*(collInfo->collTime - 0.0005f);
		pos.y += direction.y*(collInfo->collTime - 0.0005f);
		pos.z += direction.z*(collInfo->collTime - 0.0005f);
	}

	//the plane we have touched will set an acceleration to the bowl
	//like rolling down a plane
	//calculate the acceleration

	//if plane normal is parallel to gravity the planes accel is 0
	if   ( -0.00001f <= collInfo->collPlane->normal.x
		&&  0.00001f >= collInfo->collPlane->normal.x
		&& -0.00001f <= collInfo->collPlane->normal.z
		&&  0.00001f >= collInfo->collPlane->normal.z){

		direction.y = 0.0f;
		acceleration.x = acceleration.y = acceleration.z = acceleration.w = 0.0f;

	} else {
		/* in the case that the bowl is on this plane
		 * the plane will influence the bowls direction
		 * this new direction vector is the cross product
		 * of normal and gravity and the cross product of this result
		 * with the normal again
		 * the gravity is only directed in y dircetion with -1
		 */
		ScePspFVector4 normal;
		ScePspFVector4 gravity = {0.0f, -1.0f, 0.0f, 0.0f};
		ClVectorHelper::crossProduct(&normal, &collInfo->collPlane->normal, &gravity);
		ClVectorHelper::crossProduct(&acceleration, &normal, &collInfo->collPlane->normal);
		acceleration.w = 0.0f;
		//scale the acceleration vector - why ?
		//check without...
		ClVectorHelper::scale(&acceleration, 0.7f);

		//if the bowl is currently moving it should continue the same, but
		//on the new plane.
		//first get the current length of the direction (speed)
		float speed = ClVectorHelper::length(&direction);
		if (speed > 0.0f){
			//we assume first that the movement continue in Z and X
			//direction and dependent on the plane orientation
			//we will calculate the Y value.
			//TODO: if we collide with a "senkrecht" wall this would
			//      lead to divide by zero ....
			//first approach, just stopp x/z Movement!
			ScePspFVector4 newDir;
			if ( -0.00001f <= collInfo->collPlane->normal.y
			    && 0.00001f >= collInfo->collPlane->normal.y ){
				//TODO: add "sliding" code here on this kind of planes
				//      or bouncing ;o)
				newDir.x = 0.0f;
				newDir.z = 0.0f;
				newDir.y = -gravityValue/80.0f;
			} else {
				newDir.x = direction.x + acceleration.x;
				newDir.z = direction.z + acceleration.z;
				newDir.y = (-collInfo->collPlane->normal.x*direction.x - collInfo->collPlane->normal.z*direction.z)/collInfo->collPlane->normal.y;
/*				//normalize the new direction vector
				ClVectorHelper::normalize(&newDir);

				//scale the vector with the original speed
				ClVectorHelper::scale(&newDir, speed);
				*/
			}
			direction = newDir;
		}
	}
}

bool ClBowlPlayer::getLowestRoot(float a, float b, float c, float maxR, float *root){

	//check if a solution exist for quadratic equations
	float determinant = b*b - 4.0f*a*c;
	if (determinant < 0.0f)
		return false;

	//calculate the 2 roots
	float sqrD = sqrtf(determinant);
	float r1 = (-b -sqrD) / (2*a);
	float r2 = (-b +sqrD) / (2*a);
	//sort to make sure x1<=x2
	if (r1 >= r2){
		float tmp = r2;
		r2 = r1;
		r1 = tmp;
	}
	//get the lowest root
	if (r1 > 0.0f &&  r1 < maxR){
		*root = r1;
		return true;
	}
	//if r1 < 0 we need to pass back r2
	if (r2 > 0.0f && r2 < maxR){
		*root = r2;
		return true;
	}

	//if neither r1 nor r2, there is no solution
	return false;
}

void ClBowlPlayer::updateRotation(){
	//in addition to the movement of the bowl we need to rotate the same
	//for this we have the rotation axis in place
	//check for the rotation axis
	//calculate rotation matrix every time the bowl moves...
	float rSin, rCos,r1Cos, roll;
	ScePspFMatrix4 *rot;
	roll = ClVectorHelper::length(&rollAxis);

	if (roll > 0.0001f){
		//the real roll length is the length of the
		//direction vector
		//360°/2PI*r = roll°/direction.lenght
		//roll° = 360°*direction.lenght/2PI*r
		//roll_rad = 360°*dir.l*PI/180°*2PI*r
		//roll_rad =dir.l/r

		roll  = ClVectorHelper::length(&direction)/boundingSphere;
		ClVectorHelper::normalize(&rollAxis);

		rSin = sinf(roll);
		rCos = cosf(roll);
		r1Cos = 1 - rCos;
		ScePspFMatrix4 rotMatrix =
				{{rollAxis.x*rollAxis.x*(r1Cos)+rCos, rollAxis.x*rollAxis.y*(r1Cos)-rollAxis.z*rSin, rollAxis.x*rollAxis.z*(r1Cos)+rollAxis.y*rSin, 0.0f},
				 {rollAxis.x*rollAxis.y*(r1Cos)+rollAxis.z*rSin, rollAxis.y*rollAxis.y*(r1Cos)+rCos, rollAxis.y*rollAxis.z*(r1Cos)-rollAxis.x*rSin, 0.0f},
				 {rollAxis.x*rollAxis.z*(r1Cos)-rollAxis.y*rSin, rollAxis.y*rollAxis.z*(r1Cos)+rollAxis.x*rSin, rollAxis.z*rollAxis.z*(r1Cos)+rCos, 0.0f},
				 {0.0f, 0.0f, 0.0f, 1.0f}
				};
		//apply rotation matrix
		rot = (ScePspFMatrix4*)memalign(16, sizeof(ScePspFMatrix4));
		gumMultMatrix(rot, &rotMatrix, &rollMatrix);
		/*
		ClVectorHelper::normalize(&rot->x);
		ClVectorHelper::normalize(&rot->y);
		ClVectorHelper::normalize(&rot->z);*/
		rollMatrix = *rot;
		free(rot);
	}

}

void ClBowlPlayer::doShadow(std::vector<ClSceneLight*>* lightList){

	//for all lights create the shadow volume and set the stencil buffer while drawing the same
	ScePspFVector3 lightPos;
	ScePspFVector4 lightPos2;
	sceGuStencilFunc(GU_ALWAYS,0, 0x0);

	for (int i=0;i<lightList->size();i++){
		lightPos = lightList->at(i)->getPosition();
		lightPos2.x = lightPos.x;
		lightPos2.y = lightPos.y;
		lightPos2.z = lightPos.z;
		lightPos2.w = 0.0f;

		sceGuStencilOp(GU_KEEP, GU_INCR, GU_KEEP);

		sceGuFrontFace(GU_CW);
		sceGuDisable(GU_TEXTURE_2D);
		sceGuDisable(GU_CLIP_PLANES);
		sceGuDisable(GU_SCISSOR_TEST);
		castShadowVolume(&lightPos2);

		sceGuFrontFace(GU_CCW);
		sceGuStencilOp(GU_KEEP, GU_DECR, GU_KEEP);
		castShadowVolume(&lightPos2);
	}
	//restore the states
	sceGuFrontFace(GU_CW);
	sceGuEnable(GU_CLIP_PLANES);
	sceGuEnable(GU_SCISSOR_TEST);
}

void ClBowlPlayer::castShadowVolume(ScePspFVector4* lightPos){
	ScePspFMatrix4 transform, trans2;
	ScePspFVector4 lightPos2;

	//first set up the transformation matrix from original render
	//than transpose and recalculate the ligthpos into the object space....
	gumLoadIdentity(&transform);
	ScePspFVector3 renderPos;
	renderPos.x = pos.x;
	renderPos.y = pos.y;
	renderPos.z = pos.z;

	sceGumMatrixMode(GU_MODEL);
	gumTranslate(&transform, &renderPos);
	//apply the rotation of the bowl to the light pos to get
	//the right relative position for shadow volume generation
	gumMultMatrix(&trans2, &transform, &rollMatrix);
	//do move the shadow volume to the same position as the bowl and rotate if necessary...
	sceGumLoadMatrix(&trans2);


	gumFullInverse(&transform, &trans2);
	//as the vertexes of the shadow object are in model space and the light position in world space
	//we need to adopt the light position relative to this objects based on the objects position
	ClVectorHelper::vectorMulMatrix(&lightPos2, lightPos, &transform);

	unsigned int tri;
	vector<int> lightFacingTri;
	for (tri=0;tri<this->meshList[0].triangleCount;tri++){
		ScePspFVector4 relLight;
		relLight.x = lightPos2.x - meshList[0].mesh[tri*3].x;
		relLight.y = lightPos2.y - meshList[0].mesh[tri*3].y;
		relLight.z = lightPos2.z - meshList[0].mesh[tri*3].z;

		ScePspFVector4 side1, side2, v1, v2;
		side1.x = meshList[0].mesh[tri*3+1].x - meshList[0].mesh[tri*3].x;
		side1.y = meshList[0].mesh[tri*3+1].y - meshList[0].mesh[tri*3].y;
		side1.z = meshList[0].mesh[tri*3+1].z - meshList[0].mesh[tri*3].z;

		side2.x = meshList[0].mesh[tri*3+2].x - meshList[0].mesh[tri*3].x;
		side2.y = meshList[0].mesh[tri*3+2].y - meshList[0].mesh[tri*3].y;
		side2.z = meshList[0].mesh[tri*3+2].z - meshList[0].mesh[tri*3].z;

		ScePspFVector4 normal = {meshList[0].mesh[tri*3].nx,
								 meshList[0].mesh[tri*3].ny,
								 meshList[0].mesh[tri*3].nz,
								 0.0f};
		ClVectorHelper::crossProduct(&normal, &side1, &side2);
		//check whether this triangle is facing to the light or not
		float dot = ClVectorHelper::dotProduct(&relLight, &normal);
		if (dot > 0.0f){
			//found a triangle facing towards the light

			//reuse this triangle to close the shadowvolume
			lightFacingTri.push_back(tri);
			int neighbor;
			//searching for not facing towards the light
			for (neighbor=0;neighbor<3;neighbor++){
				int index = meshList[0].neighbors[tri].index[neighbor];
				if (index != -1){

					side1.x = meshList[0].mesh[index*3+1].x - meshList[0].mesh[index*3].x;
					side1.y = meshList[0].mesh[index*3+1].y - meshList[0].mesh[index*3].y;
					side1.z = meshList[0].mesh[index*3+1].z - meshList[0].mesh[index*3].z;

					side2.x = meshList[0].mesh[index*3+2].x - meshList[0].mesh[index*3].x;
					side2.y = meshList[0].mesh[index*3+2].y - meshList[0].mesh[index*3].y;
					side2.z = meshList[0].mesh[index*3+2].z - meshList[0].mesh[index*3].z;

					normal.x = meshList[0].mesh[index*3].nx;
					normal.y = meshList[0].mesh[index*3].ny;
					normal.z = meshList[0].mesh[index*3].nz;

					ClVectorHelper::crossProduct(&normal, &side1, &side2);
					dot = ClVectorHelper::dotProduct(&relLight, &normal);
				} else {
					dot = -1.0f;
				}
				if (dot <= 0.0f){
					//this neigbor is not facing to the light, this mean the side
					//the triangles are touching is casting shadow
					//draw the shadow-triangles
					side1.x = meshList[0].mesh[tri*3+neighbor].x;
					side1.y = meshList[0].mesh[tri*3+neighbor].y;
					side1.z = meshList[0].mesh[tri*3+neighbor].z;

					side2.x = meshList[0].mesh[tri*3+((neighbor+1)%3)].x;
					side2.y = meshList[0].mesh[tri*3+((neighbor+1)%3)].y;
					side2.z = meshList[0].mesh[tri*3+((neighbor+1)%3)].z;

					v1.x = side1.x - lightPos2.x;
					v1.y = side1.y - lightPos2.y;
					v1.z = side1.z - lightPos2.z;

					v2.x = side2.x - lightPos2.x;
					v2.y = side2.y - lightPos2.y;
					v2.z = side2.z - lightPos2.z;


					ClVectorHelper::normalize(&v1);ClVectorHelper::scale(&v1, 50.0f);
					ClVectorHelper::normalize(&v2);ClVectorHelper::scale(&v2, 50.0f);

					ColorFVertex* shadow = (ColorFVertex*)sceGuGetMemory(sizeof(ColorFVertex)*6);
					shadow[0].x = side1.x;
					shadow[0].y = side1.y;
					shadow[0].z = side1.z;
					shadow[0].color = 0xff000000;

					shadow[1].x = side1.x + v1.x;
					shadow[1].y = side1.y + v1.y;
					shadow[1].z = side1.z + v1.z;
					shadow[1].color = 0xff000000;

					shadow[2].x = side2.x;
					shadow[2].y = side2.y;
					shadow[2].z = side2.z;
					shadow[2].color = 0xff000000;

					shadow[3] = shadow[1];

					shadow[4].x = side2.x + v2.x;
					shadow[4].y = side2.y + v2.y;
					shadow[4].z = side2.z + v2.z;
					shadow[4].color = 0xff000000;

					shadow[5] = shadow[2];
					//draw the shadow volume triangles
					sceGumDrawArray(GU_TRIANGLES, GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D,6 ,0 , shadow);

				}
			}
		}
	}

	//after the sides we will render the closing parts of the shadow volume
	if (!lightFacingTri.empty()){
		ColorFVertex* shadow = (ColorFVertex*)sceGuGetMemory(sizeof(ColorFVertex)*lightFacingTri.size()*3);
		for (int c=0;c<lightFacingTri.size();c++){
			shadow[c*3].x = meshList[0].mesh[lightFacingTri[c]*3].x;
			shadow[c*3].y = meshList[0].mesh[lightFacingTri[c]*3].y;
			shadow[c*3].z = meshList[0].mesh[lightFacingTri[c]*3].z;

			shadow[c*3+1].x = meshList[0].mesh[lightFacingTri[c]*3+1].x;
			shadow[c*3+1].y = meshList[0].mesh[lightFacingTri[c]*3+1].y;
			shadow[c*3+1].z = meshList[0].mesh[lightFacingTri[c]*3+1].z;

			shadow[c*3+2].x = meshList[0].mesh[lightFacingTri[c]*3+2].x;
			shadow[c*3+2].y = meshList[0].mesh[lightFacingTri[c]*3+2].y;
			shadow[c*3+2].z = meshList[0].mesh[lightFacingTri[c]*3+2].z;
		}
		sceGumDrawArray(GU_TRIANGLES, GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D,lightFacingTri.size()*3 ,0 , shadow);
	}
}








