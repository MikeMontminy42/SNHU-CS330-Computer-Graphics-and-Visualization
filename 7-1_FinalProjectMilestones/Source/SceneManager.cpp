///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ================
// This file contains the implementation of the `SceneManager` class, which is 
// responsible for managing the preparation and rendering of 3D scenes. It 
// handles textures, materials, lighting configurations, and object rendering.
//
// AUTHOR: Brian Battersby
// INSTITUTION: Southern New Hampshire University (SNHU)
// COURSE: CS-330 Computational Graphics and Visualization
//
// INITIAL VERSION: November 1, 2023
// LAST REVISED: December 1, 2024
//
// RESPONSIBILITIES:
// - Load, bind, and manage textures in OpenGL.
// - Define materials and lighting properties for 3D objects.
// - Manage transformations and shader configurations.
// - Render complex 3D scenes using basic meshes.
//
// NOTE: This implementation leverages external libraries like `stb_image` for 
// texture loading and GLM for matrix and vector operations.
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/


// FUNCTION TO DEFINE OBJECT MATERIALS
void SceneManager::DefineObjectMaterials()
{
	/*** STUDENTS - add the code BELOW for defining object materials. ***/
	/*** There is no limit to the number of object materials that can ***/
	/*** be defined. Refer to the code in the OpenGL Sample for help  ***/

	OBJECT_MATERIAL stoneMAT;
	stoneMAT.ambientColor = glm::vec3(0.4f, 0.4f, 0.4f);
	stoneMAT.ambientStrength = 1.0f;
	stoneMAT.diffuseColor = glm::vec3(0.8f, 0.8f, 0.8f);
	stoneMAT.specularColor = glm::vec3(0.4f, 0.4f, 0.4f);
	stoneMAT.shininess = 5.0;
	stoneMAT.tag = "stoneMAT";

	m_objectMaterials.push_back(stoneMAT);

	OBJECT_MATERIAL metalMAT;
	metalMAT.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
	metalMAT.ambientStrength = 1.0f;
	metalMAT.diffuseColor = glm::vec3(0.4f, 0.4f, 0.4f);
	metalMAT.specularColor = glm::vec3(0.8f, 0.8f, 0.8f);
	metalMAT.shininess = 15.0;
	metalMAT.tag = "metalMAT";

	m_objectMaterials.push_back(metalMAT);

	OBJECT_MATERIAL woodMaterial;
	woodMaterial.ambientColor = glm::vec3(0.3f, 0.25f, 0.1f);   
	woodMaterial.ambientStrength = 0.8f;                        
	woodMaterial.diffuseColor = glm::vec3(0.6f, 0.5f, 0.2f);
	woodMaterial.specularColor = glm::vec3(0.1f, 0.2f, 0.2f);
	woodMaterial.shininess = 5.0f;
	woodMaterial.tag = "woodMAT";

	m_objectMaterials.push_back(woodMaterial);

	OBJECT_MATERIAL rubberMaterial;
	rubberMaterial.ambientColor = glm::vec3(0.05f, 0.05f, 0.05f); 
	rubberMaterial.ambientStrength = 0.6f;                         
	rubberMaterial.diffuseColor = glm::vec3(0.1f, 0.1f, 0.1f);
	rubberMaterial.specularColor = glm::vec3(0.05f, 0.05f, 0.05f);
	rubberMaterial.shininess = 1.0f;
	rubberMaterial.tag = "rubberMAT";

	m_objectMaterials.push_back(rubberMaterial);
}


// FUNCTION TO MAKE SCENE LIGHTING APPEAR
void SceneManager::SetupSceneLights()
{
	// this line of code is NEEDED for telling the shaders to render 
	// the 3D scene with custom lighting, if no light sources have
	// been added then the display window will be black - to use the 
	// default OpenGL lighting then comment out the following line
    m_pShaderManager->setBoolValue(g_UseLightingName, true);

	/*** STUDENTS - add the code BELOW for setting up light sources ***/
	/*** Up to four light sources can be defined. Refer to the code ***/
	/*** in the OpenGL Sample for help                              ***/


	// point light 1
	m_pShaderManager->setVec3Value("pointLights[0].position", 16.0f, 25.0f, 1.5f);
	m_pShaderManager->setVec3Value("pointLights[0].ambient", 0.35f, 0.35f, 0.35f);
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", 0.7f, 0.7f, 0.8f);
	m_pShaderManager->setVec3Value("pointLights[0].specular", 0.5f, 0.5f, 0.6f);
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);
	// point light 2
	m_pShaderManager->setVec3Value("pointLights[1].position", -14.0f, 25.0f, -10.0f);
	m_pShaderManager->setVec3Value("pointLights[1].ambient", 0.35f, 0.35f, 0.35f);
	m_pShaderManager->setVec3Value("pointLights[1].diffuse", 0.7f, 0.7f, 0.8f);
	m_pShaderManager->setVec3Value("pointLights[1].specular", 0.5f, 0.5f, 0.6f);
	m_pShaderManager->setBoolValue("pointLights[1].bActive", true);

}


// FUCNTION TO LOAD ALL SCENE TEXTURES USED
void SceneManager::LoadSceneTextures()
{
	bool bReturn = false;
	// LOADS FLOOR TEXTURE
	bReturn = CreateGLTexture("textures/asphalt-floor.png", "floor");

	// LOADS ATLAS STONE TEXTURES
	bReturn = CreateGLTexture("textures/concrete-stones.png", "atlas-stone");

	// LOADS FAR WALL TEXTURE
	bReturn = CreateGLTexture("textures/concrete-walls.png", "walls");

	// LOADS LIFTING BENCH TEXTURE
	bReturn = CreateGLTexture("textures/rubber-bench.png", "bench");

	// LOADS METAL TEXTURE
	bReturn = CreateGLTexture("textures/metal-beams.png", "metal");

	// LOADS WOOD TEXTURE
	bReturn = CreateGLTexture("textures/wood-base.png", "wood");

	// LOADS DUMBBELL TEXTURE
	bReturn = CreateGLTexture("textures/dumbbells.png", "dbell");


	// Binds textures to the available slots
	BindGLTextures();


}
/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	LoadSceneTextures();
	// define the materials for objects in the scene
	DefineObjectMaterials();
	// add and define the light sources for the scene
	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadSphereMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(25.0f, 1.0f, 15.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

    SetShaderColor(0.5, 0.52, 0.55, 1);
	SetShaderMaterial("stoneMAT");
	SetTextureUVScale(2.0f, 2.0f); // ADJUSTS TEXTURE SCALING
	SetShaderTexture("floor"); // SETS SHAPE TEXTURE

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(25.0f, 1.0f, 15.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 15.0f, -15.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader
	SetShaderColor(0.6, 0.62, 0.65, 1);
	SetShaderMaterial("stoneMAT");
	SetTextureUVScale(3.0f, 3.0f); // ADJUSTS TEXTURE SCALING
	SetShaderTexture("walls"); // SETS SHAPE TEXTURE

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/



	// ATLAS STONE TABLES

	// CLOSE TABLE SHAPES 

	// CLOSE TABLE WOOD BASE
	SetTransformations(
		glm::vec3(4.35f, 0.5f, 4.35f), // SCALE
		0.0f, 0.0f, 0.0f,              // ROTAION
		glm::vec3(16.0f, 8.0f, 6.0f)   // POSITION
	);
	SetShaderColor(1.0f, 0.894f, 0.769f, 1.0f); // Sets Bisque Color
	SetShaderMaterial("woodMAT");
	SetTextureUVScale(5.0f, 77.0f); // ADJUSTS TEXTURE SCALING
	SetShaderTexture("wood"); // SETS SHAPE TEXTURE
	m_basicMeshes->DrawBoxMesh();

	// CLOSE TABLE METAL BASE
	SetTransformations(
		glm::vec3(4.35f, 0.5f, 4.35f), // SCALE
		0.0f, 0.0f, 0.0f,              // ROTAION
		glm::vec3(16.0f, 7.5f, 6.0f)   // POSITION
	);
	SetShaderColor(0.15f, 0.15f, 0.15f, 1.0f); // Sets Steel Color
	SetShaderMaterial("metalMAT");
	SetShaderTexture("metal");
	m_basicMeshes->DrawBoxMesh();

	// LEG
	SetTransformations(
		glm::vec3(0.5f, 0.5f, 7.5f),     // SCALE
		90.0f, 0.0f, 0.0f,               // ROTAION
		glm::vec3(16.5f, 3.95f, 4.15f)   // POSITION
	);
	SetShaderColor(0.15f, 0.15f, 0.15f, 1.0f); // Sets Steel Color
	SetShaderMaterial("metalMAT");
	SetShaderTexture("metal");
	m_basicMeshes->DrawBoxMesh();

	// LEG
	SetTransformations(
		glm::vec3(0.5f, 0.5f, 7.5f),    // SCALE
		90.0f, 0.0f, 0.0f,              // ROTAION
		glm::vec3(16.5f, 3.95f, 7.9f)   // POSITION
	);
	SetShaderColor(0.15f, 0.15f, 0.15f, 1.0f); // Sets Steel Color
	SetShaderMaterial("metalMAT");
	SetShaderTexture("metal");
	m_basicMeshes->DrawBoxMesh();

	// BASE
	SetTransformations(
		glm::vec3(0.5f, 0.5f, 4.5f),     // SCALE
		0.0f, 90.0f, 0.0f,               // ROTAION
		glm::vec3(16.25f, 0.25f, 7.9f)   // POSITION
	);
	SetShaderColor(0.15f, 0.15f, 0.15f, 1.0f); // Sets Steel Color
	SetShaderMaterial("metalMAT");
	SetShaderTexture("metal");
	m_basicMeshes->DrawBoxMesh();

	// BASE
	SetTransformations(
		glm::vec3(0.5f, 0.5f, 4.5f),     // SCALE
		0.0f, 90.0f, 0.0f,               // ROTAION
		glm::vec3(16.25f, 0.25f, 4.125f) // POSITION
	);
	SetShaderColor(0.15f, 0.15f, 0.15f, 1.0f); // Sets Steel Color
	SetShaderMaterial("metalMAT");
	SetShaderTexture("metal");
	m_basicMeshes->DrawBoxMesh();

	// BASE REAR CROSS SUPPORT
	SetTransformations(
		glm::vec3(0.5f, 0.5f, 4.5f),    // SCALE
		0.0f, 0.0f, 90.0f,              // ROTAION
		glm::vec3(18.25f, 0.25f, 6.0f)  // POSITION
	);
	SetShaderColor(0.15f, 0.15f, 0.15f, 1.0f); // Sets Steel Color
	SetShaderMaterial("metalMAT");
	SetShaderTexture("metal");
	m_basicMeshes->DrawBoxMesh();
	
	// STONE "HOLE"
	SetTransformations(
		glm::vec3(1.0f, 0.1f, 1.0), // SCALE
		0.0f, 0.0f, 0.0f,			// ROTATION
		glm::vec3(16.0f, 7.155f, -3.0f) // POSITION
	);
	SetShaderColor(0.15f, 0.15f, 0.15f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();


	// FAR TABLE SHAPES

	// FAR TABLE WOOD BASE
	SetTransformations(
		glm::vec3(4.35f, 0.5f, 4.35f), // SCALE
		0.0f, 0.0f, 0.0f,              // ROTAION
		glm::vec3(16.0f, 7.0f, -3.0f)  // POSITION
	);
	SetShaderColor(1.0f, 0.894f, 0.769f, 1.0f); // Sets Bisque Color
	SetShaderMaterial("woodMAT");
	SetTextureUVScale(5.0f, 77.0f); // ADJUSTS TEXTURE SCALING
	SetShaderTexture("wood"); // SETS SHAPE TEXTURE
	m_basicMeshes->DrawBoxMesh();

	// FAR TABLE METAL BASE
	SetTransformations(
		glm::vec3(4.35f, 0.5f, 4.35f), // SCALE
		0.0f, 0.0f, 0.0f,              // ROTAION
		glm::vec3(16.0f, 6.5f, -3.0f)  // POSITION
	);
	SetShaderColor(0.15f, 0.15f, 0.15f, 1.0f); // Sets Steel Color
	SetShaderMaterial("metalMAT");
	SetShaderTexture("metal");
	m_basicMeshes->DrawBoxMesh();

	// LEG
	SetTransformations(
		glm::vec3(0.5f, 0.5f, 6.5f), // SCALE
		90.0f, 0.0f, 0.0f,              // ROTAION
		glm::vec3(16.5f, 3.125f, -4.925f)   // POSITION
	);
	SetShaderColor(0.15f, 0.15f, 0.15f, 1.0f); // Sets Steel Color
	SetShaderMaterial("metalMAT");
	SetShaderTexture("metal");
	m_basicMeshes->DrawBoxMesh();

	// LEG
	SetTransformations(
		glm::vec3(0.5f, 0.5f, 6.5f), // SCALE
		90.0f, 0.0f, 0.0f,              // ROTAION
		glm::vec3(16.5f, 3.125f, -1.075f)   // POSITION
	);
	SetShaderColor(0.15f, 0.15f, 0.15f, 1.0f); // Sets Steel Color
	SetShaderMaterial("metalMAT");
	SetShaderTexture("metal");
	m_basicMeshes->DrawBoxMesh();

	// BASE
	SetTransformations(
		glm::vec3(0.5f, 0.5f, 4.5f),     // SCALE
		0.0f, 90.0f, 0.0f,               // ROTAION
		glm::vec3(16.25f, 0.25f, -4.925f)   // POSITION
	);
	SetShaderColor(0.15f, 0.15f, 0.15f, 1.0f); // Sets Steel Color
	SetShaderMaterial("metalMAT");
	SetShaderTexture("metal");
	m_basicMeshes->DrawBoxMesh();

	// BASE
	SetTransformations(
		glm::vec3(0.5f, 0.5f, 4.5f),     // SCALE
		0.0f, 90.0f, 0.0f,               // ROTAION
		glm::vec3(16.25f, 0.25f, -1.075f) // POSITION
	);
	SetShaderColor(0.15f, 0.15f, 0.15f, 1.0f); // Sets Steel Color
	SetShaderMaterial("metalMAT");
	SetShaderTexture("metal");
	m_basicMeshes->DrawBoxMesh();

	// BASE REAR CROSS SUPPORT
	SetTransformations(
		glm::vec3(0.5f, 0.5f, 4.5f),    // SCALE
		0.0f, 0.0f, 90.0f,              // ROTAION
		glm::vec3(18.25f, 0.25f, -3.0f)  // POSITION
	);
	SetShaderColor(0.15f, 0.15f, 0.15f, 1.0f); // Sets Steel Color
	SetShaderMaterial("metalMAT");
	SetShaderTexture("metal");
	m_basicMeshes->DrawBoxMesh();

	// STONE "HOLE"
	SetTransformations(
		glm::vec3(1.0f, 0.1f, 1.0), // SCALE
		0.0f, 0.0f, 0.0f,			// ROTATION
		glm::vec3(16.0f, 8.155f, 6.0f) // POSITION
	);
	SetShaderColor(0.15f, 0.15f, 0.15f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();

	
	// ATLAS STONES

	// FAR STONE
	SetTransformations(
		glm::vec3(2.25f, 2.25f, 2.25f), // SCALE
		0.0f, 0.0f, 0.0f,				// ROTATION
		glm::vec3(10.0f, 2.35f, -3.0f)	// POSITION
	);
	SetShaderColor(0.725f, 0.725f, 0.655f, 1.0f); // Sets Concrete Color
	SetTextureUVScale(2.0f, 2.0f); // ADJUSTS TEXTURE SCALING
	SetShaderMaterial("stoneMAT");
	SetShaderTexture("atlas-stone"); // SETS SHAPE TEXTURE
	m_basicMeshes->DrawSphereMesh();

	// CLOSE STONE
	SetTransformations(
		glm::vec3(1.75f, 1.75f, 1.75f), // SCALE
		0.0f, 0.0f, 0.0f,				// ROTATION
		glm::vec3(10.0f, 1.95f, 6.0f)	// POSITION
	);
	SetShaderColor(0.725f, 0.725f, 0.655f, 1.0f); // Sets Concrete Color
	SetTextureUVScale(2.0f, 2.0f); // ADJUSTS TEXTURE SCALING
	SetShaderMaterial("stoneMAT");
	SetShaderTexture("atlas-stone"); // SETS SHAPE TEXTURE
	m_basicMeshes->DrawSphereMesh();


	// LIFTING BENCHES

	// LEFT SIDE BENCH

	// "SEAT" PLATFORM
	SetTransformations(
		glm::vec3(0.5f, 2.5f, 10.75f),    // SCALE
		0.0f, 0.0f, 90.0f,              // ROTAION
		glm::vec3(-15.25f, 3.025f, 3.0f)  // POSITION
	);
	SetShaderColor(0.15f, 0.15f, 0.15f, 1.0f); // Sets Dark Grey Color
	SetTextureUVScale(0.75f, 0.75f); // ADJUSTS TEXTURE SCALING
	SetShaderMaterial("rubberMAT");
	SetShaderTexture("bench"); // SETS SHAPE TEXTURE

	m_basicMeshes->DrawBoxMesh();

	// FAR BOTTOM SUPPORT BAR
	SetTransformations(
		glm::vec3(1.075f, 0.5f, 3.5f),     // SCALE
		0.0f, 90.0f, 0.0f,               // ROTAION
		glm::vec3(-15.25f, 0.25f, -1.25f)   // POSITION
	);
	SetShaderColor(0.25f, 0.25f, 0.25f, 1.0f); // Sets Steel Color
	SetTextureUVScale(35.0f, 35.0f); // ADJUSTS TEXTURE SCALING
	SetShaderMaterial("metalMAT");
	SetShaderTexture("metal"); // SETS SHAPE TEXTURE
	m_basicMeshes->DrawBoxMesh();

	// FAR VERTICAL SUPPORT PILLAR
	SetTransformations(
		glm::vec3(1.075f, 1.0f, 2.85f),     // SCALE
		90.0f, 0.0f, 0.0f,               // ROTAION
		glm::vec3(-15.25f, 1.345f, -1.25f)   // POSITION
	);
	SetShaderColor(0.25f, 0.25f, 0.25f, 1.0f); // Sets Steel Color
	SetTextureUVScale(35.0f, 35.0f); // ADJUSTS TEXTURE SCALING
	SetShaderMaterial("metalMAT");
	SetShaderTexture("metal"); // SETS SHAPE TEXTURE
	m_basicMeshes->DrawBoxMesh();

	// NEAR BOTTOM SUPPORT BAR
	SetTransformations(
		glm::vec3(1.075f, 0.5f, 3.5f),     // SCALE
		0.0f, 90.0f, 0.0f,               // ROTAION
		glm::vec3(-15.25f, 0.25f, 7.15f)   // POSITION
	);
	SetShaderColor(0.25f, 0.25f, 0.25f, 1.0f); // Sets Steel Color
	SetTextureUVScale(35.0f, 35.0f); // ADJUSTS TEXTURE SCALING
	SetShaderMaterial("metalMAT");
	SetShaderTexture("metal"); // SETS SHAPE TEXTURE
	m_basicMeshes->DrawBoxMesh();

	// NEAR VERTICAL SUPPORT PILLAR
	SetTransformations(
		glm::vec3(1.075f, 1.0f, 2.85f),     // SCALE
		90.0f, 0.0f, 0.0f,               // ROTAION
		glm::vec3(-15.25f, 1.345f, 7.15f)   // POSITION
	);
	SetShaderColor(0.25f, 0.25f, 0.25f, 1.0f); // Sets Steel Color
	SetTextureUVScale(35.0f, 35.0f); // ADJUSTS TEXTURE SCALING
	SetShaderMaterial("metalMAT");
	SetShaderTexture("metal"); // SETS SHAPE TEXTURE
	m_basicMeshes->DrawBoxMesh();

	// RIGHT SIDE BENCH

	// "SEAT" PLATFORM
	SetTransformations(
		glm::vec3(0.5f, 2.5f, 10.75f),    // SCALE
		0.0f, 0.0f, 90.0f,              // ROTAION
		glm::vec3(-5.25f, 3.025f, 3.0f)  // POSITION
	);
	SetShaderColor(0.15f, 0.15f, 0.15f, 1.0f); // Sets Dark Grey Color
	SetTextureUVScale(0.75f, 0.75f); // ADJUSTS TEXTURE SCALING
	SetShaderMaterial("rubberMAT");
	SetShaderTexture("bench"); // SETS SHAPE TEXTURE
	m_basicMeshes->DrawBoxMesh();

	// FAR BOTTOM SUPPORT BAR
	SetTransformations(
		glm::vec3(1.075f, 0.5f, 3.5f),     // SCALE
		0.0f, 90.0f, 0.0f,               // ROTAION
		glm::vec3(-5.25f, 0.25f, -1.25f)   // POSITION
	);
	SetShaderColor(0.25f, 0.25f, 0.25f, 1.0f); // Sets Steel Color
	SetTextureUVScale(35.0f, 35.0f); // ADJUSTS TEXTURE SCALING
	SetShaderMaterial("metalMAT");
	SetShaderTexture("metal"); // SETS SHAPE TEXTURE
	m_basicMeshes->DrawBoxMesh();

	// FAR VERTICAL SUPPORT PILLAR
	SetTransformations(
		glm::vec3(1.075f, 1.0f, 2.85f),     // SCALE
		90.0f, 0.0f, 0.0f,               // ROTAION
		glm::vec3(-5.25f, 1.345f, -1.25f)   // POSITION
	);
	SetShaderColor(0.25f, 0.25f, 0.25f, 1.0f); // Sets Steel Color
	SetTextureUVScale(35.0f, 35.0f); // ADJUSTS TEXTURE SCALING
	SetShaderMaterial("metalMAT");
	SetShaderTexture("metal"); // SETS SHAPE TEXTURE
	m_basicMeshes->DrawBoxMesh();

	// NEAR BOTTOM SUPPORT BAR
	SetTransformations(
		glm::vec3(1.075f, 0.5f, 3.5f),     // SCALE
		0.0f, 90.0f, 0.0f,               // ROTAION
		glm::vec3(-5.25f, 0.25f, 7.15f)   // POSITION
	);
	SetShaderColor(0.25f, 0.25f, 0.25f, 1.0f); // Sets Steel Color
	SetTextureUVScale(35.0f, 35.0f); // ADJUSTS TEXTURE SCALING
	SetShaderMaterial("metalMAT");
	SetShaderTexture("metal"); // SETS SHAPE TEXTURE
	m_basicMeshes->DrawBoxMesh();

	// NEAR VERTICAL SUPPORT PILLAR
	SetTransformations(
		glm::vec3(1.075f, 1.0f, 2.85f),     // SCALE
		90.0f, 0.0f, 0.0f,               // ROTAION
		glm::vec3(-5.25f, 1.345f, 7.15f)   // POSITION
	);
	SetShaderColor(0.25f, 0.25f, 0.25f, 1.0f); // Sets Steel Color
	SetTextureUVScale(35.0f, 35.0f); // ADJUSTS TEXTURE SCALING
	SetShaderMaterial("metalMAT");
	SetShaderTexture("metal"); // SETS SHAPE TEXTURE
	m_basicMeshes->DrawBoxMesh();


	// DUMBBELL RACK

	// DUMBBELL HOLDER BAR
	SetTransformations(
		glm::vec3(17.5f, 0.8f, 0.4f),     // SCALE
		90.0f, 0.0f, 0.0f,               // ROTAION
		glm::vec3(-10.25f, 5.345f, -10.15f)   // POSITION
	);
	SetShaderColor(0.25f, 0.25f, 0.25f, 1.0f); // Sets Steel Color
	SetTextureUVScale(35.0f, 35.0f); // ADJUSTS TEXTURE SCALING
	SetShaderMaterial("metalMAT");
	SetShaderTexture("metal"); // SETS SHAPE TEXTURE
	m_basicMeshes->DrawBoxMesh();

	// LEFT CROSS SUPPORT BAR
	SetTransformations(
		glm::vec3(0.8f, 3.0f, 1.0f),     // SCALE
		90.0f, 0.0f, 0.0f,               // ROTAION
		glm::vec3(-18.6f, 5.0f, -10.15f)   // POSITION
	);
	SetShaderColor(0.25f, 0.25f, 0.25f, 1.0f); // Sets Steel Color
	SetTextureUVScale(35.0f, 35.0f); // ADJUSTS TEXTURE SCALING
	SetShaderMaterial("metalMAT");
	SetShaderTexture("metal"); // SETS SHAPE TEXTURE
	m_basicMeshes->DrawBoxMesh();

	// LEFT CLOSE SUPPORT BAR
	SetTransformations(
		glm::vec3(0.8f, 6.0f, 1.0f),     // SCALE
		-20.0f, 0.0f, 0.0f,               // ROTAION
		glm::vec3(-18.6f, 2.345f, -8.1)   // POSITION
	);
	SetShaderColor(0.25f, 0.25f, 0.25f, 1.0f); // Sets Steel Color
	SetTextureUVScale(35.0f, 35.0f); // ADJUSTS TEXTURE SCALING
	SetShaderMaterial("metalMAT");
	SetShaderTexture("metal"); // SETS SHAPE TEXTURE
	m_basicMeshes->DrawBoxMesh();

	// LEFT FAR SUPPORT BAR
	SetTransformations(
		glm::vec3(0.8f, 6.0f, 1.0f),     // SCALE
		20.0f, 0.0f, 0.0f,               // ROTAION
		glm::vec3(-18.6f, 2.345f, -12.15f)   // POSITION
	);
	SetShaderColor(0.25f, 0.25f, 0.25f, 1.0f); // Sets Steel Color
	SetTextureUVScale(35.0f, 35.0f); // ADJUSTS TEXTURE SCALING
	SetShaderMaterial("metalMAT");
	SetShaderTexture("metal"); // SETS SHAPE TEXTURE
	m_basicMeshes->DrawBoxMesh();

	// MIDDLE CROSS SUPPORT BAR
	SetTransformations(
		glm::vec3(0.8f, 3.0f, 1.0f),     // SCALE
		90.0f, 0.0f, 0.0f,               // ROTAION
		glm::vec3(-10.25f, 5.0f, -10.15f)   // POSITION
	);
	SetShaderColor(0.25f, 0.25f, 0.25f, 1.0f); // Sets Steel Color
	SetTextureUVScale(35.0f, 35.0f); // ADJUSTS TEXTURE SCALING
	SetShaderMaterial("metalMAT");
	SetShaderTexture("metal"); // SETS SHAPE TEXTURE
	m_basicMeshes->DrawBoxMesh();

	// MIDDLE CLOSE SUPPORT BAR
	SetTransformations(
		glm::vec3(0.8f, 6.0f, 1.0f),     // SCALE
		-20.0f, 0.0f, 0.0f,               // ROTAION
		glm::vec3(-10.25f, 2.345f, -8.1f)   // POSITION
	);
	SetShaderColor(0.25f, 0.25f, 0.25f, 1.0f); // Sets Steel Color
	SetTextureUVScale(35.0f, 35.0f); // ADJUSTS TEXTURE SCALING
	SetShaderMaterial("metalMAT");
	SetShaderTexture("metal"); // SETS SHAPE TEXTURE
	m_basicMeshes->DrawBoxMesh();

	// MIDDLE FAR SUPPORT BAR
	SetTransformations(
		glm::vec3(0.8f, 6.0f, 1.0f),     // SCALE
		20.0f, 0.0f, 0.0f,               // ROTAION
		glm::vec3(-10.25f, 2.345f, -12.15f)   // POSITION
	);
	SetShaderColor(0.25f, 0.25f, 0.25f, 1.0f); // Sets Steel Color
	SetTextureUVScale(35.0f, 35.0f); // ADJUSTS TEXTURE SCALING
	SetShaderMaterial("metalMAT");
	SetShaderTexture("metal"); // SETS SHAPE TEXTURE
	m_basicMeshes->DrawBoxMesh();

	// RIGHT CROSS SUPPORT BAR
	SetTransformations(
		glm::vec3(0.8f, 3.0f, 1.0f),     // SCALE
		90.0f, 0.0f, 0.0f,               // ROTAION
		glm::vec3(-1.9f, 5.0f, -10.15f)   // POSITION
	);
	SetShaderColor(0.25f, 0.25f, 0.25f, 1.0f); // Sets Steel Color
	SetTextureUVScale(35.0f, 35.0f); // ADJUSTS TEXTURE SCALING
	SetShaderMaterial("metalMAT");
	SetShaderTexture("metal"); // SETS SHAPE TEXTURE
	m_basicMeshes->DrawBoxMesh();

	// RIGHT CLOSE SUPPORT BAR
	SetTransformations(
		glm::vec3(0.8f, 6.0f, 1.0f),     // SCALE
		-20.0f, 0.0f, 0.0f,               // ROTAION
		glm::vec3(-1.9, 2.345f, -8.1f)   // POSITION
	);
	SetShaderColor(0.25f, 0.25f, 0.25f, 1.0f); // Sets Steel Color
	SetTextureUVScale(35.0f, 35.0f); // ADJUSTS TEXTURE SCALING
	SetShaderMaterial("metalMAT");
	SetShaderTexture("metal"); // SETS SHAPE TEXTURE
	m_basicMeshes->DrawBoxMesh();

	// RIGHT FAR SUPPORT BAR
	SetTransformations(
		glm::vec3(0.8f, 6.0f, 1.0f),     // SCALE
		20.0f, 0.0f, 0.0f,               // ROTAION
		glm::vec3(-1.9f, 2.345f, -12.15f)   // POSITION
	);
	SetShaderColor(0.25f, 0.25f, 0.25f, 1.0f); // Sets Steel Color
	SetTextureUVScale(35.0f, 35.0f); // ADJUSTS TEXTURE SCALING
	SetShaderMaterial("metalMAT");
	SetShaderTexture("metal"); // SETS SHAPE TEXTURE
	m_basicMeshes->DrawBoxMesh();


	// YORK GLOBE DUMBBELLS (STARTING ON FAR LEFT SIDE OF RACK)

	// LEFT 65LB DUMBBELL
	// MIDDLE GRIP
	SetTransformations(
		glm::vec3(0.2f, 1.2f, 0.2f),     // SCALE
		90.0f, 0.0f, 0.0f,               // ROTAION
		glm::vec3(-17.5f, 5.75f, -10.75f)   // POSITION
	);
	SetShaderColor(0.25f, 0.25f, 0.25f, 1.0f); // Sets Steel Color
	SetTextureUVScale(1.0f, 1.0f); // ADJUSTS TEXTURE SCALING
	SetShaderMaterial("metalMAT");
	SetShaderTexture("dbell"); // SETS SHAPE TEXTURE
	m_basicMeshes->DrawCylinderMesh();

	// REAR WEIGHT GLOBE
	SetTransformations(
		glm::vec3(0.75f, 0.75f, 0.75f),     // SCALE
		0.0f, 0.0f, 0.0f,               // ROTAION
		glm::vec3(-17.5f, 5.75f, -11.45f)   // POSITION
	);
	SetShaderColor(0.25f, 0.25f, 0.25f, 1.0f); // Sets Steel Color
	SetTextureUVScale(1.0f, 1.0f); // ADJUSTS TEXTURE SCALING
	SetShaderMaterial("metalMAT");
	SetShaderTexture("dbell"); // SETS SHAPE TEXTURE
	m_basicMeshes->DrawSphereMesh();

	// FRONT WEIGHT GLOBE
	SetTransformations(
		glm::vec3(0.75f, 0.75f, 0.75f),     // SCALE
		0.0f, 0.0f, 0.0f,               // ROTAION
		glm::vec3(-17.5f, 5.75f, -8.85f)   // POSITION
	);
	SetShaderColor(0.25f, 0.25f, 0.25f, 1.0f); // Sets Steel Color
	SetTextureUVScale(1.0f, 1.0f); // ADJUSTS TEXTURE SCALING
	SetShaderMaterial("metalMAT");
	SetShaderTexture("dbell"); // SETS SHAPE TEXTURE
	m_basicMeshes->DrawSphereMesh();


	// RIGHT 65LB DUMBBELL
	// MIDDLE GRIP
	SetTransformations(
		glm::vec3(0.2f, 1.2f, 0.2f),     // SCALE
		90.0f, 0.0f, 0.0f,               // ROTAION
		glm::vec3(-15.75f, 5.75f, -10.75f)   // POSITION
	);
	SetShaderColor(0.25f, 0.25f, 0.25f, 1.0f); // Sets Steel Color
	SetTextureUVScale(1.0f, 1.0f); // ADJUSTS TEXTURE SCALING
	SetShaderMaterial("metalMAT");
	SetShaderTexture("dbell"); // SETS SHAPE TEXTURE
	m_basicMeshes->DrawCylinderMesh();

	// REAR WEIGHT GLOBE
	SetTransformations(
		glm::vec3(0.75f, 0.75f, 0.75f),     // SCALE
		0.0f, 0.0f, 0.0f,               // ROTAION
		glm::vec3(-15.75f, 5.75f, -11.45f)   // POSITION
	);
	SetShaderColor(0.25f, 0.25f, 0.25f, 1.0f); // Sets Steel Color
	SetTextureUVScale(1.0f, 1.0f); // ADJUSTS TEXTURE SCALING
	SetShaderMaterial("metalMAT");
	SetShaderTexture("dbell"); // SETS SHAPE TEXTURE
	m_basicMeshes->DrawSphereMesh();

	// FRONT WEIGHT GLOBE
	SetTransformations(
		glm::vec3(0.75f, 0.75f, 0.75f),     // SCALE
		0.0f, 0.0f, 0.0f,               // ROTAION
		glm::vec3(-15.75f, 5.75f, -8.85f)   // POSITION
	);
	SetShaderColor(0.25f, 0.25f, 0.25f, 1.0f); // Sets Steel Color
	SetTextureUVScale(1.0f, 1.0f); // ADJUSTS TEXTURE SCALING
	SetShaderMaterial("metalMAT");
	SetShaderTexture("dbell"); // SETS SHAPE TEXTURE
	m_basicMeshes->DrawSphereMesh();


	// LEFT 95LB DUMBBELL
	// MIDDLE GRIP
	SetTransformations(
		glm::vec3(0.2f, 1.2f, 0.2f),     // SCALE
		90.0f, 0.0f, 0.0f,               // ROTAION
		glm::vec3(-13.5f, 5.75f, -10.75f)   // POSITION
	);
	SetShaderColor(0.25f, 0.25f, 0.25f, 1.0f); // Sets Steel Color
	SetTextureUVScale(1.0f, 1.0f); // ADJUSTS TEXTURE SCALING
	SetShaderMaterial("metalMAT");
	SetShaderTexture("dbell"); // SETS SHAPE TEXTURE
	m_basicMeshes->DrawCylinderMesh();

	// REAR WEIGHT GLOBE
	SetTransformations(
		glm::vec3(0.875f, 0.875f, 0.875f),     // SCALE
		0.0f, 0.0f, 0.0f,               // ROTAION
		glm::vec3(-13.5f, 5.75f, -11.55f)   // POSITION
	);
	SetShaderColor(0.25f, 0.25f, 0.25f, 1.0f); // Sets Steel Color
	SetTextureUVScale(1.0f, 1.0f); // ADJUSTS TEXTURE SCALING
	SetShaderMaterial("metalMAT");
	SetShaderTexture("dbell"); // SETS SHAPE TEXTURE
	m_basicMeshes->DrawSphereMesh();

	// FRONT WEIGHT GLOBE
	SetTransformations(
		glm::vec3(0.875f, 0.875f, 0.875f),     // SCALE
		0.0f, 0.0f, 0.0f,               // ROTAION
		glm::vec3(-13.5f, 5.75f, -8.75f)   // POSITION
	);
	SetShaderColor(0.25f, 0.25f, 0.25f, 1.0f); // Sets Steel Color
	SetTextureUVScale(1.0f, 1.0f); // ADJUSTS TEXTURE SCALING
	SetShaderMaterial("metalMAT");
	SetShaderTexture("dbell"); // SETS SHAPE TEXTURE
	m_basicMeshes->DrawSphereMesh();


	// RIGHT 95LB DUMBBELL
	// MIDDLE GRIP
	SetTransformations(
		glm::vec3(0.2f, 1.2f, 0.2f),     // SCALE
		90.0f, 0.0f, 0.0f,               // ROTAION
		glm::vec3(-11.5f, 5.75f, -10.75f)   // POSITION
	);
	SetShaderColor(0.25f, 0.25f, 0.25f, 1.0f); // Sets Steel Color
	SetTextureUVScale(1.0f, 1.0f); // ADJUSTS TEXTURE SCALING
	SetShaderMaterial("metalMAT");
	SetShaderTexture("dbell"); // SETS SHAPE TEXTURE
	m_basicMeshes->DrawCylinderMesh();

	// REAR WEIGHT GLOBE
	SetTransformations(
		glm::vec3(0.875f, 0.875f, 0.875f),     // SCALE
		0.0f, 0.0f, 0.0f,               // ROTAION
		glm::vec3(-11.5f, 5.75f, -11.55f)   // POSITION
	);
	SetShaderColor(0.25f, 0.25f, 0.25f, 1.0f); // Sets Steel Color
	SetTextureUVScale(1.0f, 1.0f); // ADJUSTS TEXTURE SCALING
	SetShaderMaterial("metalMAT");
	SetShaderTexture("dbell"); // SETS SHAPE TEXTURE
	m_basicMeshes->DrawSphereMesh();

	// FRONT WEIGHT GLOBE
	SetTransformations(
		glm::vec3(0.875f, 0.875f, 0.875f),     // SCALE
		0.0f, 0.0f, 0.0f,               // ROTAION
		glm::vec3(-11.5f, 5.75f, -8.75f)   // POSITION
	);
	SetShaderColor(0.25f, 0.25f, 0.25f, 1.0f); // Sets Steel Color
	SetTextureUVScale(1.0f, 1.0f); // ADJUSTS TEXTURE SCALING
	SetShaderMaterial("metalMAT");
	SetShaderTexture("dbell"); // SETS SHAPE TEXTURE
	m_basicMeshes->DrawSphereMesh();


	// LEFT 125LB DUMBBELL
	// MIDDLE GRIP
	SetTransformations(
		glm::vec3(0.2f, 1.2f, 0.2f),     // SCALE
		90.0f, 0.0f, 0.0f,               // ROTAION
		glm::vec3(-8.85f, 5.75f, -10.75f)   // POSITION
	);
	SetShaderColor(0.25f, 0.25f, 0.25f, 1.0f); // Sets Steel Color
	SetTextureUVScale(1.0f, 1.0f); // ADJUSTS TEXTURE SCALING
	SetShaderMaterial("metalMAT");
	SetShaderTexture("dbell"); // SETS SHAPE TEXTURE
	m_basicMeshes->DrawCylinderMesh();

	// REAR WEIGHT GLOBE
	SetTransformations(
		glm::vec3(1.0f, 1.0f, 1.0f),     // SCALE
		0.0f, 0.0f, 0.0f,               // ROTAION
		glm::vec3(-8.85f, 5.75f, -11.65f)   // POSITION
	);
	SetShaderColor(0.25f, 0.25f, 0.25f, 1.0f); // Sets Steel Color
	SetTextureUVScale(1.0f, 1.0f); // ADJUSTS TEXTURE SCALING
	SetShaderMaterial("metalMAT");
	SetShaderTexture("dbell"); // SETS SHAPE TEXTURE
	m_basicMeshes->DrawSphereMesh();

	// FRONT WEIGHT GLOBE
	SetTransformations(
		glm::vec3(1.0f, 1.0f, 1.0f),     // SCALE
		0.0f, 0.0f, 0.0f,               // ROTAION
		glm::vec3(-8.85f, 5.75f, -8.65f)   // POSITION
	);
	SetShaderColor(0.25f, 0.25f, 0.25f, 1.0f); // Sets Steel Color
	SetTextureUVScale(1.0f, 1.0f); // ADJUSTS TEXTURE SCALING
	SetShaderMaterial("metalMAT");
	SetShaderTexture("dbell"); // SETS SHAPE TEXTURE
	m_basicMeshes->DrawSphereMesh();


	// RIGHT 125LB DUMBBELL
	// MIDDLE GRIP
	SetTransformations(
		glm::vec3(0.2f, 1.2f, 0.2f),     // SCALE
		90.0f, 0.0f, 0.0f,               // ROTAION
		glm::vec3(-6.85f, 5.75f, -10.75f)   // POSITION
	);
	SetShaderColor(0.25f, 0.25f, 0.25f, 1.0f); // Sets Steel Color
	SetTextureUVScale(1.0f, 1.0f); // ADJUSTS TEXTURE SCALING
	SetShaderMaterial("metalMAT");
	SetShaderTexture("dbell"); // SETS SHAPE TEXTURE
	m_basicMeshes->DrawCylinderMesh();

	// REAR WEIGHT GLOBE
	SetTransformations(
		glm::vec3(1.0f, 1.0f, 1.0f),     // SCALE
		0.0f, 0.0f, 0.0f,               // ROTAION
		glm::vec3(-6.85f, 5.75f, -11.65f)   // POSITION
	);
	SetShaderColor(0.25f, 0.25f, 0.25f, 1.0f); // Sets Steel Color
	SetTextureUVScale(1.0f, 1.0f); // ADJUSTS TEXTURE SCALING
	SetShaderMaterial("metalMAT");
	SetShaderTexture("dbell"); // SETS SHAPE TEXTURE
	m_basicMeshes->DrawSphereMesh();

	// FRONT WEIGHT GLOBE
	SetTransformations(
		glm::vec3(1.0f, 1.0f, 1.0f),     // SCALE
		0.0f, 0.0f, 0.0f,               // ROTAION
		glm::vec3(-6.85f, 5.75f, -8.65f)   // POSITION
	);
	SetShaderColor(0.25f, 0.25f, 0.25f, 1.0f); // Sets Steel Color
	SetTextureUVScale(1.0f, 1.0f); // ADJUSTS TEXTURE SCALING
	SetShaderMaterial("metalMAT");
	SetShaderTexture("dbell"); // SETS SHAPE TEXTURE
	m_basicMeshes->DrawSphereMesh();


	// LEFT 155LB DUMBBELL
	// MIDDLE GRIP
	SetTransformations(
		glm::vec3(0.2f, 1.2f, 0.2f),     // SCALE
		90.0f, 0.0f, 0.0f,               // ROTAION
		glm::vec3(-9.35f, 1.15f, 1.55f)   // POSITION
	);
	SetShaderColor(0.25f, 0.25f, 0.25f, 1.0f); // Sets Steel Color
	SetTextureUVScale(1.0f, 1.0f); // ADJUSTS TEXTURE SCALING
	SetShaderMaterial("metalMAT");
	SetShaderTexture("dbell"); // SETS SHAPE TEXTURE
	m_basicMeshes->DrawCylinderMesh();

	// REAR WEIGHT GLOBE
	SetTransformations(
		glm::vec3(1.2f, 1.2f, 1.2f),     // SCALE
		0.0f, 0.0f, 0.0f,               // ROTAION
		glm::vec3(-9.35f, 1.15f, 3.875f)   // POSITION
	);
	SetShaderColor(0.25f, 0.25f, 0.25f, 1.0f); // Sets Steel Color
	SetTextureUVScale(1.0f, 1.0f); // ADJUSTS TEXTURE SCALING
	SetShaderMaterial("metalMAT");
	SetShaderTexture("dbell"); // SETS SHAPE TEXTURE
	m_basicMeshes->DrawSphereMesh();

	// FRONT WEIGHT GLOBE
	SetTransformations(
		glm::vec3(1.2f, 1.2f, 1.2f),     // SCALE
		0.0f, 0.0f, 0.0f,               // ROTAION
		glm::vec3(-9.35f, 1.15f, 0.475f)   // POSITION
	);
	SetShaderColor(0.25f, 0.25f, 0.25f, 1.0f); // Sets Steel Color
	SetTextureUVScale(1.0f, 1.0f); // ADJUSTS TEXTURE SCALING
	SetShaderMaterial("metalMAT");
	SetShaderTexture("dbell"); // SETS SHAPE TEXTURE
	m_basicMeshes->DrawSphereMesh();


	// RIGHT 155LB DUMBBELL
	// MIDDLE GRIP
	SetTransformations(
		glm::vec3(0.2f, 1.2f, 0.2f),     // SCALE
		90.0f, 0.0f, 0.0f,               // ROTAION
		glm::vec3(-1.15f, 1.15f, 1.55f)   // POSITION
	);
	SetShaderColor(0.25f, 0.25f, 0.25f, 1.0f); // Sets Steel Color
	SetTextureUVScale(1.0f, 1.0f); // ADJUSTS TEXTURE SCALING
	SetShaderMaterial("metalMAT");
	SetShaderTexture("dbell"); // SETS SHAPE TEXTURE
	m_basicMeshes->DrawCylinderMesh();

	// REAR WEIGHT GLOBE
	SetTransformations(
		glm::vec3(1.2f, 1.2f, 1.2f),     // SCALE
		0.0f, 0.0f, 0.0f,               // ROTAION
		glm::vec3(-1.15f, 1.15f, 3.875f)   // POSITION
	);
	SetShaderColor(0.25f, 0.25f, 0.25f, 1.0f); // Sets Steel Color
	SetTextureUVScale(1.0f, 1.0f); // ADJUSTS TEXTURE SCALING
	SetShaderMaterial("metalMAT");
	SetShaderTexture("dbell"); // SETS SHAPE TEXTURE
	m_basicMeshes->DrawSphereMesh();

	// FRONT WEIGHT GLOBE
	SetTransformations(
		glm::vec3(1.2f, 1.2f, 1.2f),     // SCALE
		0.0f, 0.0f, 0.0f,               // ROTAION
		glm::vec3(-1.15f, 1.15f, 0.475f)   // POSITION
	);
	SetShaderColor(0.25f, 0.25f, 0.25f, 1.0f); // Sets Steel Color
	SetTextureUVScale(1.0f, 1.0f); // ADJUSTS TEXTURE SCALING
	SetShaderMaterial("metalMAT");
	SetShaderTexture("dbell"); // SETS SHAPE TEXTURE
	m_basicMeshes->DrawSphereMesh();

}
