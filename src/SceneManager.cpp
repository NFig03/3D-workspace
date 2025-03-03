///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

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
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	// create the shape meshes object
	m_basicMeshes = new ShapeMeshes();

	// initialize the texture collection
	for (int i = 0; i < 16; i++)
	{
		m_textureIDs[i].tag = "/0";
		m_textureIDs[i].ID = -1;
	}
	m_loadedTextures = 0;
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
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
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

	modelView = translation * rotationX * rotationY * rotationZ * scale;

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
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
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

void SceneManager::LoadSceneTextures()
{
	bool bReturn = false;

	//loads in the textures
	bReturn = CreateGLTexture("C:/Users/NFigu/Pictures/dfc59b634fc228887ca8668526b24100.jpg", "Wood");
	bReturn = CreateGLTexture("C:/Users/NFigu/Pictures/images.jpg", "Metal");
	bReturn = CreateGLTexture("C:/Users/NFigu/Pictures/download.jpg", "Magazine Cover");
	bReturn = CreateGLTexture("C:/Users/NFigu/Pictures/d7i46lm-d44c1ab4-227d-4009-9114-e549c1420d21.jpg", "Black Metal");
	bReturn = CreateGLTexture("C:/Users/NFigu/Pictures/images (1).jpg", "White");

	//Binds the loaded textures to memory slots
	BindGLTextures();
}

void SceneManager::DefineObjectMaterials()
{
	OBJECT_MATERIAL paperMaterial;
	paperMaterial.ambientColor = glm::vec3(0.8f, 0.8f, 0.8f);
	paperMaterial.ambientStrength = 0.2f;
	paperMaterial.diffuseColor = glm::vec3(0.9f, 0.9f, 0.9f);
	paperMaterial.specularColor = glm::vec3(0.05f, 0.05f, 0.05f);
	paperMaterial.shininess = 10.0f;
	paperMaterial.tag = "Paper";

	m_objectMaterials.push_back(paperMaterial);

	
	OBJECT_MATERIAL woodMaterial;
	woodMaterial.ambientColor = glm::vec3(0.2f, 0.1f, 0.1f);
	woodMaterial.ambientStrength = 0.3f;
	woodMaterial.diffuseColor = glm::vec3(0.6f, 0.4f, 0.2f);
	woodMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	woodMaterial.shininess = 25.0f;
	woodMaterial.tag = "Wood";


	m_objectMaterials.push_back(woodMaterial);

	OBJECT_MATERIAL plasticMaterial;
	plasticMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
	plasticMaterial.ambientStrength = 0.1f;
	plasticMaterial.diffuseColor = glm::vec3(0.7f, 0.7f, 0.7f);
	plasticMaterial.specularColor = glm::vec3(0.6f, 0.6f, 0.6f);
	plasticMaterial.shininess = 30;
	plasticMaterial.tag = "Plastic";

	m_objectMaterials.push_back(plasticMaterial);

	OBJECT_MATERIAL metalMaterial;
	metalMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
	metalMaterial.ambientStrength = 0.1f;
	metalMaterial.diffuseColor = glm::vec3(0.7f, 0.7f, 0.7f);
	metalMaterial.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
	metalMaterial.shininess = 80.0f;
	metalMaterial.tag = "Metal";

	m_objectMaterials.push_back(metalMaterial);

}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// this line of code is NEEDED for telling the shaders to render 
	// the 3D scene with custom lighting, if no light sources have
	// been added then the display window will be black - to use the 
	// default OpenGL lighting then comment out the following line

	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	m_pShaderManager->setVec3Value("lightSources[0].position", -3.0f, 4.0f, 6.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.7f, 0.7f, 0.6f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 15.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.1f);

	m_pShaderManager->setVec3Value("lightSources[1].position", 3.0f, 4.0f, 6.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.7f, 0.7f, 0.6f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 15.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.1f);

	m_pShaderManager->setVec3Value("lightSources[2].position", 0.0f, 3.0f, 20.0f);
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", 0.8f, 0.8f, 0.8f);
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 12.0f);
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.1f);

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
	DefineObjectMaterials();
	SetupSceneLights();
	
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
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

	//Draw the plane for the scene
	/******************************************************************/
	// set the XYZ scale for the plane
	scaleXYZ = glm::vec3(85.0f, 1.0f, 200.0f);

	// set the XYZ rotation for the plane
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the plane
	positionXYZ = glm::vec3(0.0f, -32.0f, -200.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderMaterial("Wood");
	SetShaderTexture("Wood");

	// draw the plane for the scene
	m_basicMeshes->DrawPlaneMesh();
	
	//draw the torus for the handle of the mug
	/****************************************************************/

	// set the XYZ scale for the handle
	scaleXYZ = glm::vec3(0.35f, 0.35f, 0.4f);

	// set the XYZ rotation for the handle
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the handle
	positionXYZ = glm::vec3(-11.3f, -8.85f, -33.4f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderMaterial("Metal");
	SetShaderTexture("White");

	// draw the keyboard with transformation values
	m_basicMeshes->DrawTorusMesh();
	/****************************************************************/

	//draw the torus for the lip of the mug
	/****************************************************************/

	// set the XYZ scale for the mug
	scaleXYZ = glm::vec3(0.55f, 0.55f, 0.4f);

	// set the XYZ rotation for the mug
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mug
	positionXYZ = glm::vec3(-10.5f, -8.4f, -33.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderMaterial("Metal");
	SetShaderTexture("White");

	// draw the torus with transformation values
	m_basicMeshes->DrawTorusMesh();
	/****************************************************************/

	//draw the cylinder for the monitor stand
	/****************************************************************/

	// set the XYZ scale for the monitor stand
	scaleXYZ = glm::vec3(0.25f, 2.3f, 0.25f);

	// set the XYZ rotation for the monitor stand
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the monitor stand
	positionXYZ = glm::vec3(7.7f, -9.6f, -30.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderMaterial("Metal");
	SetShaderTexture("Metal");

	// draw the monitor stand with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	//draw the cylinder for the mug
	/****************************************************************/

	// set the XYZ scale for the dish
	scaleXYZ = glm::vec3(0.6f, 1.0f, 0.6f);

	// set the XYZ rotation for the dish
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the dish
	positionXYZ = glm::vec3(-10.5f, -9.4f, -33.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderMaterial("Metal");
	SetShaderTexture("White");

	// draw the keyboard with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	//draw the torus for the middle of the dish
	/****************************************************************/

	// set the XYZ scale for the dish
	scaleXYZ = glm::vec3(0.75f, 0.75f, 0.3f);

	// set the XYZ rotation for the dish
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the dish
	positionXYZ = glm::vec3(-10.5f, -9.38f, -33.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderMaterial("Metal");
	SetShaderTexture("White");

	// draw the dish with transformation values
	m_basicMeshes->DrawTorusMesh();
	/****************************************************************/

	//draw the cylinder for the left monitor leg
	/****************************************************************/

	// set the XYZ scale for the leg
	scaleXYZ = glm::vec3(2.2f, 0.3f, 0.2f);

	// set the XYZ rotation for the leg
	XrotationDegrees = 0.0f;
	YrotationDegrees = -125.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the leg
	positionXYZ = glm::vec3(8.9f, -9.6f, -31.8f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderMaterial("Metal");
	SetShaderTexture("Metal");

	// draw the leg with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	//draw the cylinder for the right monitor leg
	/****************************************************************/

	// set the XYZ scale for the leg
	scaleXYZ = glm::vec3(2.2f, 0.3f, 0.2f);

	// set the XYZ rotation for the leg
	XrotationDegrees = 0.0f;
	YrotationDegrees = 3.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the leg
	positionXYZ = glm::vec3(5.7f, -9.6f, -29.9f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderMaterial("Metal");
	SetShaderTexture("Metal");

	// draw the leg with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	//draw the torus for the bottom of the dish
	/****************************************************************/

	// set the XYZ scale for the dish
	scaleXYZ = glm::vec3(0.6f, 0.6f, 0.4f);

	// set the XYZ rotation for the dish
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the dish
	positionXYZ = glm::vec3(-10.5f, -9.5f, -33.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderMaterial("Metal");
	SetShaderTexture("White");

	// draw the dish with transformation values
	m_basicMeshes->DrawTorusMesh();
	/****************************************************************/

	//draw the cylinder for the mouse
	/****************************************************************/

	// set the XYZ scale for the mouse
	scaleXYZ = glm::vec3(0.65f, 0.75f, 0.65f);

	// set the XYZ rotation for the mouse
	XrotationDegrees = 0.0f;
	YrotationDegrees = 30.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mouse
	positionXYZ = glm::vec3(-3.0f, -9.4f, -38.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderMaterial("Plastic");
	SetShaderTexture("White");

	// draw the keyboard with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	//draw the box for the player
	/****************************************************************/

	// set the XYZ scale for the player
	scaleXYZ = glm::vec3(3.15f, 2.5f, 2.65f);

	// set the XYZ rotation for the player
	XrotationDegrees = 0.0f;
	YrotationDegrees = 30.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the player
	positionXYZ = glm::vec3(-4.3f, -8.5f, -23.2f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("White");
	SetShaderMaterial("Plastic");

	// draw the player with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	//draw the cylinder for the right book end
	/****************************************************************/

	// set the XYZ scale for the book end
	scaleXYZ = glm::vec3(2.6f, 1.0f, 1.5f);

	// set the XYZ rotation for the book end
	XrotationDegrees = 0.0f;
	YrotationDegrees = 30.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the book end
	positionXYZ = glm::vec3(18.14f, -7.8f, -36.48f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("Wood");
	SetShaderMaterial("Wood");

	// draw the book with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	//draw the cylinder for the 5th left book end
	/****************************************************************/

	// set the XYZ scale for the book end
	scaleXYZ = glm::vec3(3.0f, 1.0f, 1.5f);

	// set the XYZ rotation for the book end
	XrotationDegrees = 0.0f;
	YrotationDegrees = 30.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the book end
	positionXYZ = glm::vec3(20.0f, -7.4f, -37.6f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("Wood");
	SetShaderMaterial("Wood");

	// draw the book with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	//draw the cylinder for the 4th left book end
	/****************************************************************/

	// set the XYZ scale for the book end
	scaleXYZ = glm::vec3(2.4f, 1.0f, 1.2f);

	// set the XYZ rotation for the book end
	XrotationDegrees = 0.0f;
	YrotationDegrees = 30.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the book end
	positionXYZ = glm::vec3(20.5f, -7.6f, -37.9f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("Wood");
	SetShaderMaterial("Wood");

	// draw the book with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	//draw the cylinder for the 3rd left book end
	/****************************************************************/

	// set the XYZ scale for the book end
	scaleXYZ = glm::vec3(2.0f, 1.0f, 1.0f);

	// set the XYZ rotation for the book end
	XrotationDegrees = 0.0f;
	YrotationDegrees = 30.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the book end
	positionXYZ = glm::vec3(20.9f, -7.8f, -38.1f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("Wood");
	SetShaderMaterial("Wood");

	// draw the book with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	//draw the cylinder for the 2nd left book end
	/****************************************************************/

	// set the XYZ scale for the book end
	scaleXYZ = glm::vec3(1.6f, 1.0f, 0.8f);

	// set the XYZ rotation for the book end
	XrotationDegrees = 0.0f;
	YrotationDegrees = 30.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the book end
	positionXYZ = glm::vec3(21.3f, -8.2f, -38.4f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("Wood");
	SetShaderMaterial("Wood");

	// draw the book with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	//draw the cylinder for the 1st left book end
	/****************************************************************/

	// set the XYZ scale for the book end
	scaleXYZ = glm::vec3(1.2f, 1.0f, 0.6f);

	// set the XYZ rotation for the book end
	XrotationDegrees = 0.0f;
	YrotationDegrees = 30.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the book end
	positionXYZ = glm::vec3(21.8f, -8.6f, -38.7f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("Wood");
	SetShaderMaterial("Wood");

	// draw the book with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	//draw the box for the right standing book
	/****************************************************************/

	// set the XYZ scale for the book
	scaleXYZ = glm::vec3(6.5f, 1.0f, 3.5f);

	// set the XYZ rotation for the book
	XrotationDegrees = 0.0f;
	YrotationDegrees = 30.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the book
	positionXYZ = glm::vec3(18.14f, -6.5f, -36.48f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);

	// draw the book with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	//draw the box for the left standing book
	/****************************************************************/

	// set the XYZ scale for the book
	scaleXYZ = glm::vec3(6.5f, 1.0f, 3.5f);

	// set the XYZ rotation for the book
	XrotationDegrees = 0.0f;
	YrotationDegrees = 30.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the book
	positionXYZ = glm::vec3(19.0f, -6.5f, -37.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.6706, 0.8588, 0.8902, 1);

	// draw the book with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	//draw the box for the back right book
	/****************************************************************/

	// set the XYZ scale for the book
	scaleXYZ = glm::vec3(6.5f, 1.4f, 3.5f);

	// set the XYZ rotation for the book
	XrotationDegrees = 0.0f;
	YrotationDegrees = 30.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the book
	positionXYZ = glm::vec3(-4.0f, -9.5f, -23.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);

	// draw the book with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	//draw the box for the monitor
	/****************************************************************/

	// set the XYZ scale for the monitor
	scaleXYZ = glm::vec3(14.0f, 0.3f, 8.5f);

	// set the XYZ rotation for the monitor
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -30.0f;

	// set the XYZ position for the monitor
	positionXYZ = glm::vec3(7.75f, -3.0f, -30.2f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("Magazine Cover");
	SetShaderMaterial("Plastic");

	// draw the monitor with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	//draw the box for the keyboard
	/****************************************************************/

	// set the XYZ scale for the keyboard
	scaleXYZ = glm::vec3(7.0f, 0.3f, 3.0f);

	// set the XYZ rotation for the keyboard
	XrotationDegrees = 0.0f;
	YrotationDegrees = 30.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the keyboard
	positionXYZ = glm::vec3(2.0f, -9.5f, -41.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderMaterial("Plastic");
	SetShaderTexture("White");

	// draw the keyboard with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	//draw the box for the desktop
	/****************************************************************/

	// set the XYZ scale for the desktop
	scaleXYZ = glm::vec3(35.0f, 0.8f, 18.0f);

	// set the XYZ rotation for the desktop
	XrotationDegrees = 0.0f;
	YrotationDegrees = 30.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the desktop
	positionXYZ = glm::vec3(5.0f, -10.0f, -35.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("Wood");
	SetShaderMaterial("Wood");

	// draw the desktop with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/


	//draw the box for the drawer compartment
	/****************************************************************/

	// set the XYZ scale for the compartment
	scaleXYZ = glm::vec3(10.0f, 4.5f, 14.3f);

	// set the XYZ rotation for the compartment
	XrotationDegrees = 0.0f;
	YrotationDegrees = 30.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the desktop
	positionXYZ = glm::vec3(14.5f, -12.0f, -40.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("Wood");
	SetShaderMaterial("Wood");

	// draw the compartment with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/


	//draw the physical drawer bottom
	/****************************************************************/

	// set the XYZ scale for the bottom
	scaleXYZ = glm::vec3(3.8f, 1.0f, 4.0f);

	// set the XYZ rotation for the bottom
	XrotationDegrees = 0.0f;
	YrotationDegrees = 30.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the desktop
	positionXYZ = glm::vec3(10.1f, -14.2f, -48.95f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderMaterial("Wood");
	SetShaderTexture("Wood");

	// draw the bottom with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/


	//draw the magazine in the drawer
	/****************************************************************/

	// set the XYZ scale for the magazine
	scaleXYZ = glm::vec3(2.8f, 1.0f, 3.7f);

	// set the XYZ rotation for the magazine
	XrotationDegrees = 0.0f;
	YrotationDegrees = 30.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the desktop
	positionXYZ = glm::vec3(10.0f, -14.1f, -48.95f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("Magazine Cover");
	SetShaderMaterial("Paper");

	// draw the magazine with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/


	//draw the left pen in the drawer
	/****************************************************************/

	// set the XYZ scale for the pen
	scaleXYZ = glm::vec3(0.2f, 3.0f, 0.2f);

	// set the XYZ rotation for the pen
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -30.0f;

	// set the XYZ position for the desktop
	positionXYZ = glm::vec3(9.0f, -13.8f, -51.7f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("White");
	SetShaderMaterial("Plastic");

	// draw the pen with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/


	//draw the right pen in the drawer
	/****************************************************************/

	// set the XYZ scale for the pen
	scaleXYZ = glm::vec3(0.2f, 3.0f, 0.2f);

	// set the XYZ rotation for the pen
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -65.0f;

	// set the XYZ position for the desktop
	positionXYZ = glm::vec3(7.1f, -13.8f, -50.7f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("White");
	SetShaderMaterial("Plastic");

	// draw the pen with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/


	//draw the right side of the drawer
	/****************************************************************/

	// set the XYZ scale for the right side
	scaleXYZ = glm::vec3(8.0f, 2.5f, 0.8f);

	// set the XYZ rotation for the right side
	XrotationDegrees = 0.0f;
	YrotationDegrees = 120.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the desktop
	positionXYZ = glm::vec3(6.75f, -13.0f, -47.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("Wood");
	SetShaderMaterial("Wood");

	// draw the right side with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/


	//draw the left side of the drawer
	/****************************************************************/

	// set the XYZ scale for the left side
	scaleXYZ = glm::vec3(8.0f, 2.5f, 0.8f);

	// set the XYZ rotation for the left side
	XrotationDegrees = 0.0f;
	YrotationDegrees = 120.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the desktop
	positionXYZ = glm::vec3(13.0f, -13.0f, -51.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("Wood");
	SetShaderMaterial("Wood");

	// draw the left side with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/


	//draw the drawer track
	/****************************************************************/

	// set the XYZ scale for the track
	scaleXYZ = glm::vec3(8.3f, 0.5f, 0.1f);

	// set the XYZ rotation for the track
	XrotationDegrees = 0.0f;
	YrotationDegrees = 120.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the desktop
	positionXYZ = glm::vec3(13.5f, -13.8f, -51.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("Metal");
	SetShaderMaterial("Metal");

	// draw the track with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/


	//draw the drawer front
	/****************************************************************/

	// set the XYZ scale for the front
	scaleXYZ = glm::vec3(9.0f, 3.5f, 0.8f);

	// set the XYZ rotation for the front
	XrotationDegrees = 0.0f;
	YrotationDegrees = 30.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the desktop
	positionXYZ = glm::vec3(8.1f, -12.5f, -52.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("Wood");
	SetShaderMaterial("Wood");

	// draw the front with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/


	//draw the back right leg of the desk
	/****************************************************************/

	// set the XYZ scale for the leg
	scaleXYZ = glm::vec3(1.0f, 22.0f, 1.0f);

	// set the XYZ rotation for the leg
	XrotationDegrees = 0.0f;
	YrotationDegrees = 30.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the leg
	positionXYZ = glm::vec3(-5.0f, -20.99f, -22.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderMaterial("Metal");
	SetShaderTexture("Black Metal");

	// draw the leg with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/


	//draw the middle back right leg of the desk
	/****************************************************************/

	// set the XYZ scale for the leg
	scaleXYZ = glm::vec3(1.0f, 22.0f, 1.0f);

	// set the XYZ rotation for the leg
	XrotationDegrees = 0.0f;
	YrotationDegrees = 30.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the leg
	positionXYZ = glm::vec3(3.0f, -20.99f, -26.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("Black Metal");
	SetShaderMaterial("Metal");

	// draw the leg with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/


	//draw the front right leg of the desk
	/****************************************************************/

	// set the XYZ scale for the leg
	scaleXYZ = glm::vec3(1.0f, 22.0f, 1.0f);

	// set the XYZ rotation for the leg
	XrotationDegrees = 0.0f;
	YrotationDegrees = 30.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the leg
	positionXYZ = glm::vec3(-12.0f, -20.99f, -34.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("Black Metal");
	SetShaderMaterial("Metal");

	// draw the leg with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/


	//draw the middle left leg of the desk
	/****************************************************************/

	// set the XYZ scale for the leg
	scaleXYZ = glm::vec3(1.0f, 22.0f, 1.0f);

	// set the XYZ rotation for the leg
	XrotationDegrees = 0.0f;
	YrotationDegrees = 30.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the leg
	positionXYZ = glm::vec3(19.5f, -20.99f, -42.8f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("Black Metal");
	SetShaderMaterial("Metal");

	// draw the leg with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/


	//draw the front middle left leg of the desk
	/****************************************************************/

	// set the XYZ scale for the leg
	scaleXYZ = glm::vec3(1.0f, 22.0f, 1.0f);

	// set the XYZ rotation for the leg
	XrotationDegrees = 0.0f;
	YrotationDegrees = 30.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the leg
	positionXYZ = glm::vec3(6.47f, -20.99f, -43.55f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("Black Metal");
	SetShaderMaterial("Metal");

	// draw the leg with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/


	//draw the back left leg of the desk
	/****************************************************************/

	// set the XYZ scale for the leg
	scaleXYZ = glm::vec3(1.0f, 22.0f, 1.0f);

	// set the XYZ rotation for the leg
	XrotationDegrees = 0.0f;
	YrotationDegrees = 30.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the leg
	positionXYZ = glm::vec3(13.0f, -20.99f, -32.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("Black Metal");
	SetShaderMaterial("Metal");

	// draw the leg with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/


	//draw the top front cross beam
	/****************************************************************/

	// set the XYZ scale for the beam
	scaleXYZ = glm::vec3(1.0f, 21.0f, 1.0f);

	// set the XYZ rotation for the beam
	XrotationDegrees = 0.0f;
	YrotationDegrees = 30.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the beam
	positionXYZ = glm::vec3(-3.0f, -11.0f, -38.4f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("Black Metal");
	SetShaderMaterial("Metal");

	// draw the beam with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/


	//draw the bottom left leg cross beam
	/****************************************************************/

	// set the XYZ scale for the beam
	scaleXYZ = glm::vec3(1.0f, 10.65f, 1.0f);

	// set the XYZ rotation for the beam
	XrotationDegrees = 0.0f;
	YrotationDegrees = 30.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the beam
	positionXYZ = glm::vec3(14.8f, -29.0f, -40.2f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("Black Metal");
	SetShaderMaterial("Metal");

	// draw the beam with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/


	//draw the bottom right leg cross beam
	/****************************************************************/

	// set the XYZ scale for the beam
	scaleXYZ = glm::vec3(1.0f, 13.5f, 1.0f);

	// set the XYZ rotation for the beam
	XrotationDegrees = 0.0f;
	YrotationDegrees = 120.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the beam
	positionXYZ = glm::vec3(9.645f, -29.0f, -38.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("Black Metal");
	SetShaderMaterial("Metal");

	// draw the beam with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/


	//draw the far back right bottom leg cross beam
	/****************************************************************/

	// set the XYZ scale for the beam
	scaleXYZ = glm::vec3(1.0f, 8.75f, 1.0f);

	// set the XYZ rotation for the beam
	XrotationDegrees = 0.0f;
	YrotationDegrees = 30.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the beam
	positionXYZ = glm::vec3(-0.75f, -29.0f, -24.25f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("Black Metal");
	SetShaderMaterial("Metal");

	// draw the beam with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/


	//draw the second bottom left leg cross beam
	/****************************************************************/

	// set the XYZ scale for the beam
	scaleXYZ = glm::vec3(1.0f, 13.9f, 1.0f);

	// set the XYZ rotation for the beam
	XrotationDegrees = 0.0f;
	YrotationDegrees = 120.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the beam
	positionXYZ = glm::vec3(-8.2f, -29.0f, -27.6f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("Black Metal");
	SetShaderMaterial("Metal");

	// draw the beam with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	//draw the box for the computer
	/****************************************************************/

	// set the XYZ scale for the computer
	scaleXYZ = glm::vec3(10.0f, 18.0f, 5.0f);

	// set the XYZ rotation for the computer
	XrotationDegrees = 0.0f;
	YrotationDegrees = 120.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the computer
	positionXYZ = glm::vec3(-4.7f, -29.0f, -30.2f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderMaterial("Metal");
	SetShaderTexture("White");

	// draw the computer with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/
}
