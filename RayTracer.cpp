/*==================================================================================
* COSC 363  Computer Graphics
* Department of Computer Science and Software Engineering, University of Canterbury.
*
* A basic ray tracer
* See Lab07.pdf   for details.
*===================================================================================
*/
#include <iostream>
#include <cmath>
#include <vector>
#include <glm/glm.hpp>
#include "Sphere.h"
#include "SceneObject.h"
#include "Ray.h"
#include "Plane.h"
#include <GL/freeglut.h>
#include "TextureBMP.h"
#include "Cylinder.h"
#include <glm/gtc/random.hpp>
using namespace std;

const float EDIST = 40.0;
const int NUMDIV = 500;
const int MAX_STEPS = 5;
const float XMIN = -10.0;
const float XMAX = 10.0;
const float YMIN = -10.0;
const float YMAX = 10.0;
TextureBMP texture;

const bool  USE_FOG = 0;
const float FOG_GROWTH_RATE = 0.01;
const float FOG_START_DIST = -50;

const float	SCENE_REFRACTIVE_INDEX = 1.0;

const bool  USE_AA = 0;

const bool  USE_SOFT_SHADOWS = 0;
const float SOFT_SHADOW_RADIUS = 10.0;
const int   SOFT_SHADOW_NUM_SAMPLES = 10;

const bool  USE_DOF = 1;
const float DOF_FOCAL_DIST = 80.0f;
const int   DOF_NUM_SAMPLES = 10;
const float DOF_APERTURE = 0.7;

vector<SceneObject*> sceneObjects;

// Room corner coordinates
glm::vec3 blb = glm::vec3(-20., -15., -200);
glm::vec3 brb = glm::vec3(20., -15., -200);
glm::vec3 blt = glm::vec3(-20., 15., -200);
glm::vec3 brt = glm::vec3(20., 15., -200);

glm::vec3 flb = glm::vec3(-20., -15., 40);
glm::vec3 frb = glm::vec3(20., -15., 40);
glm::vec3 flt = glm::vec3(-20., 15., 40);
glm::vec3 frt = glm::vec3(20., 15., 40);

glm::vec3 getShadowColor(SceneObject* obj, Ray shadowRay, glm::vec3 lightVec, glm::vec3 baseColor) {
	glm::vec3 color = glm::vec3(0);
	shadowRay.closestPt(sceneObjects);
	if (shadowRay.index > -1 && shadowRay.dist < glm::length(lightVec)) {
		SceneObject* shadowObj = sceneObjects[shadowRay.index];
		if (!shadowObj->isTransparent() && !shadowObj->isRefractive()) {
			color = obj->getColor() * 0.2f;
		}
		if (shadowObj->isTransparent()) {
			color = obj->getColor() * max(0.2f, shadowObj->getTransparencyCoeff());
		}
		if (shadowObj->isRefractive()) {
			color = obj->getColor() * max(0.2f, shadowObj->getRefractionCoeff());
		}
	}
	else {
		color = baseColor;
	}
	return color;
}


//---The most important function in a ray tracer! ---------------------------------- 
//   Computes the colour value obtained by tracing a ray and finding its 
//     closest point of intersection with objects in the scene.
//----------------------------------------------------------------------------------
glm::vec3 trace(Ray ray, int step)
{
	glm::vec3 backgroundCol(0);						//Background colour = (0,0,0)
	glm::vec3 lightPos(0, 13, -100);					//Light's position
	glm::vec3 color(0);
	SceneObject* obj;

    ray.closestPt(sceneObjects);					//Compare the ray with all objects in the scene
    if(ray.index == -1) return backgroundCol;		//no intersection
	obj = sceneObjects[ray.index];					//object on which the closest point of intersection is found

	if (ray.index == 0)
	{
		// Stripe pattern
		int stripeWidth = 5;
		int iz = (ray.hit.z) / stripeWidth - 100;
		int ix = (ray.hit.x) / stripeWidth - 100;
		int k = iz % 2; // 2 colors
		int l = ix % 2; // 2 colors
		if ((abs(k) == abs(l)))
			color = glm::vec3(0.55, 0.18, 0.22);
		else
			color = glm::vec3(0.98, 0.72, 0.69);
		obj->setColor(color);
		//float texcoords = (ray.hit.x - -15)/(5 - -15);
		//float texcoordt = (ray.hit.z - -60) / (-90 - -60);
		//if (texcoords > 0 && texcoords < 1 &&
		//	texcoordt > 0 && texcoordt < 1)
		//{
		//	color = texture.getColorAt(texcoords, texcoordt);
		//	obj->setColor(color);
		//}
	}

	// Procedural pattern on back wall
	if (ray.index == 1) {
		color = glm::vec3(
			sin(ray.hit.x * ray.hit.y * 0.1) / 2 + 0.5,
			sin(ray.hit.x / 10) / 2 + 0.5,
			sin(ray.hit.x / ray.hit.y * 0.1) / 2 + 0.5);
		obj->setColor(color);
	}

	color = obj->lighting(lightPos, -ray.dir, ray.hit);
	glm::vec3 lightVec = lightPos - ray.hit;

	// Soft shadows
	if (USE_SOFT_SHADOWS) {
		glm::vec3 colors[SOFT_SHADOW_NUM_SAMPLES] = {};

		// Taking multiple color samples
		for (int i = 0; i < SOFT_SHADOW_NUM_SAMPLES; i++) {
			glm::vec3 lightOffset = glm::ballRand(2.);
			Ray shadowRay(ray.hit, lightVec + lightOffset);
			colors[i] = getShadowColor(obj, shadowRay, lightVec, color);
		}

		// Averaging color samples
		glm::vec3 avgColor = glm::vec3(0);
		for (int i = 0; i < SOFT_SHADOW_NUM_SAMPLES; i++) {
			avgColor += colors[i];
		}
		avgColor /= SOFT_SHADOW_NUM_SAMPLES;
		color = avgColor;
	}
	else {
		Ray shadowRay(ray.hit, lightVec);
		color = getShadowColor(obj, shadowRay, lightVec, color);
	}

	if (obj->isReflective() && step < MAX_STEPS) {
		float rho = obj->getReflectionCoeff();
		glm::vec3 normalVec = obj->normal(ray.hit);
		glm::vec3 reflectedDir = glm::reflect(ray.dir, normalVec);
		Ray reflectedRay(ray.hit, reflectedDir);
		glm::vec3 reflectedColor = trace(reflectedRay, step + 1);
		color = color + (rho * reflectedColor);
	}

	if (obj->isTransparent() && step < MAX_STEPS) {
		float rho = obj->getTransparencyCoeff();
		glm::vec3 objColor = obj->getColor();
		Ray transmittedRay(ray.hit, ray.dir);
		glm::vec3 transmittedColor = trace(transmittedRay, step + 1);
		color = transmittedColor * rho + color * (1 - rho);
	}

	if (obj->isRefractive() && step < MAX_STEPS) {
		float objRefrIdx = obj->getRefractiveIndex();
		glm::vec3 objColor = obj->getColor();
		float objRefrCoeff = obj->getRefractionCoeff();

		float eta = SCENE_REFRACTIVE_INDEX / objRefrIdx;
		glm::vec3 n = obj->normal(ray.hit);
		glm::vec3 g = glm::refract(ray.dir, n, eta);
		Ray refractRay(ray.hit, g);
		refractRay.closestPt(sceneObjects);

		glm::vec3 m = obj->normal(refractRay.hit);
		glm::vec3 h = glm::refract(g, -m, 1.0f / eta);
		Ray exitRay(refractRay.hit, h);

		glm::vec3 refractColor = trace(exitRay, step + 1);
		color = objColor * (1 - objRefrCoeff) + refractColor * objRefrCoeff;
	}

	if (USE_FOG) {
		float lambda = 1 - glm::exp(FOG_GROWTH_RATE * (-FOG_START_DIST - ray.dist));
		lambda = lambda < 0 ? 0 : lambda;
		color = (1 - lambda) * color + lambda * glm::vec3(1, 1, 1);
	}


	return color;
}

//---The main display module -----------------------------------------------------------
// In a ray tracing application, it just displays the ray traced image by drawing
// each cell as a quad.
//---------------------------------------------------------------------------------------
void display()
{
	float xp, yp;  //grid point
	float cellX = (XMAX - XMIN) / NUMDIV;  //cell width
	float cellY = (YMAX - YMIN) / NUMDIV;  //cell height
	glm::vec3 eye(0., 0., 0.);

	glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

	glBegin(GL_QUADS);  //Each cell is a tiny quad.

	for (int i = 0; i < NUMDIV; i++)	//Scan every cell of the image plane
	{
		xp = XMIN + i * cellX;
		for (int j = 0; j < NUMDIV; j++)
		{
			yp = YMIN + j * cellY;
			glm::vec3 col = glm::vec3(0, 0, 0);
			
			if (USE_AA) {
				glm::vec3 dir1(xp + 0.25 * cellX, yp + 0.25 * cellY, -EDIST);
				glm::vec3 dir2(xp + 0.75 * cellX, yp + 0.25 * cellY, -EDIST);
				glm::vec3 dir3(xp + 0.25 * cellX, yp + 0.75 * cellY, -EDIST);
				glm::vec3 dir4(xp + 0.75 * cellX, yp + 0.75 * cellY, -EDIST);

				Ray ray1 = Ray(eye, dir1);
				Ray ray2 = Ray(eye, dir2);
				Ray ray3 = Ray(eye, dir3);
				Ray ray4 = Ray(eye, dir4);

				glm::vec3 col1 = trace(ray1, 1);
				glm::vec3 col2 = trace(ray2, 1);
				glm::vec3 col3 = trace(ray3, 1);
				glm::vec3 col4 = trace(ray4, 1);

				col = (col1 + col2 + col3 + col4) * 0.25f;
			}
			else {
				if (USE_DOF) {
					glm::vec3 colors[DOF_NUM_SAMPLES] = {};
					glm::vec3 dir = glm::vec3(xp + 0.5 * cellX, yp + 0.5 * cellY, -EDIST) * float(DOF_FOCAL_DIST / EDIST);

					for (int i = 0; i < DOF_NUM_SAMPLES; i++) {
						glm::vec2 offset = glm::diskRand(DOF_APERTURE);
						glm::vec3 eyeWithOffset = glm::vec3(eye.x + offset.x, eye.y + offset.y, eye.z);
						Ray ray = Ray(eyeWithOffset, dir-eyeWithOffset);
						colors[i] = trace(ray, 1);
					}

					glm::vec3 avgColor = glm::vec3(0);
					for (int i = 0; i < DOF_NUM_SAMPLES; i++) {
						avgColor += colors[i];
					}
					avgColor /= DOF_NUM_SAMPLES;
					col = avgColor;
				}
				else {
					glm::vec3 dir(xp + 0.5 * cellX, yp + 0.5 * cellY, -EDIST);	//direction of the primary ray
					Ray ray = Ray(eye, dir);
					col = trace(ray, 1); //Trace the primary ray and get the colour value
				}
			}

			glColor3f(col.r, col.g, col.b);
			glVertex2f(xp, yp);				//Draw each cell with its color value
			glVertex2f(xp + cellX, yp);
			glVertex2f(xp + cellX, yp + cellY);
			glVertex2f(xp, yp + cellY);
		}
	}

    glEnd();
    glFlush();
}



//---This function initializes the scene ------------------------------------------- 
//   Specifically, it creates scene objects (spheres, planes, cones, cylinders etc)
//     and add them to the list of scene objects.
//   It also initializes the OpenGL 2D orthographc projection matrix for drawing the
//     the ray traced image.
//----------------------------------------------------------------------------------
void initialize()
{
    glMatrixMode(GL_PROJECTION);
    gluOrtho2D(XMIN, XMAX, YMIN, YMAX);
	glClearColor(0, 0, 0, 1);

	// Room ------------------------------------------------------------------------
	Plane* floor = new Plane(brb, blb, flb, frb);
	floor->setColor(glm::vec3(0.8, 0.8, 0));
	floor->setSpecularity(false);
	sceneObjects.push_back(floor);

	Plane* back_wall = new Plane(blb, brb, brt, blt);
	back_wall ->setColor(glm::vec3(0.95, 0.11, 0.25));
	back_wall ->setSpecularity(false);
	sceneObjects.push_back(back_wall);

	Plane* roof = new Plane(brt, frt, flt, blt);
	roof ->setColor(glm::vec3(1., 0.60, 0.18));
	roof ->setSpecularity(false);
	sceneObjects.push_back(roof);

	Plane* left_wall = new Plane(blb, blt, flt, flb);
	left_wall ->setColor(glm::vec3(0.16, 0.75, 0.07));
	left_wall ->setSpecularity(false);
	sceneObjects.push_back(left_wall);

	Plane* front_wall = new Plane(frb, flb, flt, frt);
	front_wall ->setColor(glm::vec3(1, 0.11, 1.));
	front_wall ->setSpecularity(false);
	sceneObjects.push_back(front_wall);

	Plane* right_wall = new Plane(brb, frb, frt, brt);
	right_wall ->setColor(glm::vec3(0.03, 0.74, 0.74));
	right_wall ->setSpecularity(false);
	sceneObjects.push_back(right_wall);

	// ------------------------------------------------------------------------------

	Plane* mirror = new Plane(
		glm::vec3(14, -1, -179),
		glm::vec3(-14, -1, -179),
		glm::vec3(-14, -14, -179),
		glm::vec3(14, -14, -179));
	mirror->setColor(glm::vec3(0., 0., 0.));
	mirror->setReflectivity(true, 0.99);
	mirror->setSpecularity(false);
	sceneObjects.push_back(mirror);

	Plane* mirrorFrame1 = new Plane(
		glm::vec3(15, 0, -179.1),
		glm::vec3(-15, 0, -179.1),
		glm::vec3(-15, -15, -179.1),
		glm::vec3(15, -15, -179.1));
	mirrorFrame1->setColor(glm::vec3(.5, .3, 0));
	sceneObjects.push_back(mirrorFrame1);

	Plane* mirror2 = new Plane(
		glm::vec3(-14, 1, 39),
		glm::vec3(-14, -14, 39),
		glm::vec3(14, -14, 39),
		glm::vec3(14, 0, 39));
	mirror2->setColor(glm::vec3(0., 0., 0.));
	mirror2->setReflectivity(true, 0.99);
	mirror2->setSpecularity(false);
	sceneObjects.push_back(mirror2);

	Plane* mirrorFrame2 = new Plane(
		glm::vec3(-15, 0, 39.1),
		glm::vec3(-15, -15, 39.1),
		glm::vec3(15, -15, 39.1),
		glm::vec3(15, 0, 39.1));
	mirrorFrame2->setColor(glm::vec3(.5, .3, 0));
	sceneObjects.push_back(mirrorFrame2);

	Cylinder* cylinder = new Cylinder(glm::vec3(10., -15.0, -120.0), 30., 2.);
	cylinder->setColor(glm::vec3(0.3, 0.3, 0.6));
	sceneObjects.push_back(cylinder);

	//texture = TextureBMP("Butterfly.bmp");

	Sphere *transparentSphere = new Sphere(glm::vec3(-10.0, -12.0, -120.0), 3);
	transparentSphere->setColor(glm::vec3(0,1,0));
	transparentSphere->setReflectivity(true, 0.5);
	transparentSphere->setTransparency(true, 0.7);
	sceneObjects.push_back(transparentSphere);
	
	Sphere *refractSphere = new Sphere(glm::vec3(0.0, -11.0, -80.0), 4);
	refractSphere->setColor(glm::vec3(0, 0, 0));
	refractSphere->setRefractivity(true, 0.9, 1.1);
	sceneObjects.push_back(refractSphere);
}

int main(int argc, char *argv[]) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB );
    glutInitWindowSize(1000, 1000);
    glutInitWindowPosition(20, 20);
    glutCreateWindow("Raytracing");

    glutDisplayFunc(display);
    initialize();

    glutMainLoop();
    return 0;
}
