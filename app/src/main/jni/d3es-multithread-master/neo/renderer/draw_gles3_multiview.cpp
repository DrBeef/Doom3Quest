/*
 * This file is part of the D3wasm project (http://www.continuation-labs.com/projects/d3wasm)
 * Copyright (c) 2019 Gabriel Cuvillier.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <GLES3/gl3.h>
#include "sys/platform.h"
#include "renderer/VertexCache.h"

#include "tr_local.h"

#include "glsl/glsl_shaders.h"

shaderProgram_t interactionShader;
shaderProgram_t interactionPhongShader;
shaderProgram_t fogShader;
shaderProgram_t blendLightShader;
shaderProgram_t zfillShader;
shaderProgram_t zfillClipShader;
shaderProgram_t diffuseMapShader;
shaderProgram_t diffuseCubeShader;
shaderProgram_t skyboxCubeShader;
shaderProgram_t reflectionCubeShader;
shaderProgram_t stencilShadowShader;

#define ORTHO_PROJECTION			0
#define NORMAL_PROJECTION			1
#define WEAPON_PROJECTION			2
#define DEPTH_HACK_PROJECTION		3
#define NUM_DEPTH_HACK_PROJECTIONS  50
GLuint		viewMatricesBuffer;

bool        projectionMatricesSet = false;
GLuint		projectionMatricesBuffer[DEPTH_HACK_PROJECTION + NUM_DEPTH_HACK_PROJECTIONS + 1];

#define ATTR_VERTEX     0   // Don't change this, as WebGL require the vertex attrib 0 to be always bound
#define ATTR_COLOR      1
#define ATTR_TEXCOORD   2
#define ATTR_NORMAL     3
#define ATTR_TANGENT    4
#define ATTR_BITANGENT  5

/*
====================
GL_UseProgram
====================
*/
void GL_UseProgram(shaderProgram_t* program) {
	if ( backEnd.glState.currentProgram == program ) {
		return;
	}

	qglUseProgram(program ? program->program : 0);
	backEnd.glState.currentProgram = program;
}

/*
====================
GL_Uniform1fv
====================
*/
static void GL_Uniform1fv(GLint location, const GLfloat* value) {
	qglUniform1fv(*( GLint * )((char*) backEnd.glState.currentProgram + location), 1, value);
}

/*
====================
GL_Uniform1iv
====================
*/
static void GL_Uniform1iv(GLint location, const GLint* value) {
	qglUniform1iv(*( GLint * )((char*) backEnd.glState.currentProgram + location), 1, value);
}

/*
====================
GL_Uniform4fv
====================
*/
static void GL_Uniform4fv(GLint location, const GLfloat* value) {
	qglUniform4fv(*( GLint * )((char*) backEnd.glState.currentProgram + location), 1, value);
}

/*
====================
GL_UniformMatrix4fv
====================
*/
static void GL_UniformMatrix4fv(GLint location, const GLfloat* value) {
	qglUniformMatrix4fv(*( GLint * )((char*) backEnd.glState.currentProgram + location), 1, GL_FALSE, value);
}

/*
====================
GL_ViewMatricesUniformBuffer
====================
*/
static void GL_ViewMatricesUniformBuffer(const float value[32]) {

	// Update the scene matrices.
	glBindBuffer(GL_UNIFORM_BUFFER, viewMatricesBuffer);
	float* viewMatrices = (float*)glMapBufferRange(
			GL_UNIFORM_BUFFER,
			0,
			2 * 16 * sizeof(float),
			GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

	if (viewMatrices == NULL)
	{
		common->Error("View Matrices Uniform Buffer is NULL");
		return;
	}

	memcpy((char*)viewMatrices, value, 32 * sizeof(float));

	glUnmapBuffer(GL_UNIFORM_BUFFER);
	qglBindBuffer(GL_UNIFORM_BUFFER, 0);
}

/*
====================
GL_ProjectionMatricesUniformBuffer
====================
*/
static void GL_ProjectionMatricesUniformBuffer(GLint projectionMatricesBuffer, const float value[16]) {

	// Update the scene matrices.
	glBindBuffer(GL_UNIFORM_BUFFER, projectionMatricesBuffer);
	float* projectionMatrix = (float*)glMapBufferRange(
			GL_UNIFORM_BUFFER,
			0,
			16 * sizeof(float),
			GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

	if (projectionMatrix == NULL)
	{
		common->Error("Projection Matrices Uniform Buffer is NULL");
		return;
	}

	memcpy((char*)projectionMatrix, value, 16 * sizeof(float));

	glUnmapBuffer(GL_UNIFORM_BUFFER);
	qglBindBuffer(GL_UNIFORM_BUFFER, 0);
}

/*
====================
GL_EnableVertexAttribArray
====================
*/
void GL_EnableVertexAttribArray(GLuint index) {
	qglEnableVertexAttribArray(index);
}

/*
====================
GL_DisableVertexAttribArray
====================
*/
void GL_DisableVertexAttribArray(GLuint index) {
	qglDisableVertexAttribArray(index);
}

/*
====================
GL_VertexAttribPointer
====================
*/
static void GL_VertexAttribPointer(GLuint index, GLint size, GLenum type,
                                   GLboolean normalized, GLsizei stride,
                                   const GLvoid* pointer) {
	qglVertexAttribPointer(*( GLint * )((char*) backEnd.glState.currentProgram + index),
	                       size, type, normalized, stride, pointer);
}

/*
=================
R_LoadGLSLShader

loads GLSL vertex or fragment shaders
=================
*/
static void R_LoadGLSLShader(const char* buffer, shaderProgram_t* shaderProgram, GLenum type) {
	if ( !glConfig.isInitialized ) {
		return;
	}

	switch ( type ) {
	case GL_VERTEX_SHADER:
		// create vertex shader
		shaderProgram->vertexShader = qglCreateShader(GL_VERTEX_SHADER);
		qglShaderSource(shaderProgram->vertexShader, 1, (const GLchar**) &buffer, 0);
		qglCompileShader(shaderProgram->vertexShader);
		break;
	case GL_FRAGMENT_SHADER:
		// create fragment shader
		shaderProgram->fragmentShader = qglCreateShader(GL_FRAGMENT_SHADER);
		qglShaderSource(shaderProgram->fragmentShader, 1, (const GLchar**) &buffer, 0);
		qglCompileShader(shaderProgram->fragmentShader);
		break;
	default:
		common->Printf("R_LoadGLSLShader: no type\n");
		return;
	}
}

/*
=================
R_LinkGLSLShader

links the GLSL vertex and fragment shaders together to form a GLSL program
=================
*/
static bool R_LinkGLSLShader(shaderProgram_t* shaderProgram, const char* name) {
	char buf[BUFSIZ];
	int len;
	GLint linked;

	shaderProgram->program = qglCreateProgram();

	qglAttachShader(shaderProgram->program, shaderProgram->vertexShader);
	qglAttachShader(shaderProgram->program, shaderProgram->fragmentShader);

	// Bind attributes locations
	qglBindAttribLocation(shaderProgram->program, ATTR_VERTEX, "attr_Vertex");
	qglBindAttribLocation(shaderProgram->program, ATTR_COLOR, "attr_Color");
	qglBindAttribLocation(shaderProgram->program, ATTR_TEXCOORD, "attr_TexCoord");
	qglBindAttribLocation(shaderProgram->program, ATTR_NORMAL, "attr_Normal");
	qglBindAttribLocation(shaderProgram->program, ATTR_TANGENT, "attr_Tangent");
	qglBindAttribLocation(shaderProgram->program, ATTR_BITANGENT, "attr_Bitangent");

	qglLinkProgram(shaderProgram->program);

	qglGetProgramiv(shaderProgram->program, GL_LINK_STATUS, &linked);

	if ( com_developer.GetBool()) {
		qglGetShaderInfoLog(shaderProgram->vertexShader, sizeof(buf), &len, buf);
		common->Printf("VS:\n%.*s\n", len, buf);
		qglGetShaderInfoLog(shaderProgram->fragmentShader, sizeof(buf), &len, buf);
		common->Printf("FS:\n%.*s\n", len, buf);
	}

	if ( !linked ) {
		common->Error("R_LinkGLSLShader: program failed to link: %s\n", name);
		return false;
	}

	return true;
}

/*
=================
R_ValidateGLSLProgram

makes sure GLSL program is valid
=================
*/
static bool R_ValidateGLSLProgram(shaderProgram_t* shaderProgram) {
	GLint validProgram;

	qglValidateProgram(shaderProgram->program);

	qglGetProgramiv(shaderProgram->program, GL_VALIDATE_STATUS, &validProgram);

	if ( !validProgram ) {
		common->Printf("R_ValidateGLSLProgram: program invalid\n");
		return false;
	}

	return true;
}

/*
=================
RB_GLSL_GetUniformLocations

=================
*/
static void RB_GLSL_GetUniformLocations(shaderProgram_t* shader) {
	int i;
	char buffer[32];

	GL_UseProgram(shader);

	shader->localLightOrigin = qglGetUniformLocation(shader->program, "u_lightOrigin");

	//May need to move this to the shader matrices uniform block?
	shader->localViewOrigin = qglGetUniformLocation(shader->program, "u_viewOrigin");
	shader->lightProjection = qglGetUniformLocation(shader->program, "u_lightProjection");
	shader->bumpMatrixS = qglGetUniformLocation(shader->program, "u_bumpMatrixS");
	shader->bumpMatrixT = qglGetUniformLocation(shader->program, "u_bumpMatrixT");
	shader->diffuseMatrixS = qglGetUniformLocation(shader->program, "u_diffuseMatrixS");
	shader->diffuseMatrixT = qglGetUniformLocation(shader->program, "u_diffuseMatrixT");
	shader->specularMatrixS = qglGetUniformLocation(shader->program, "u_specularMatrixS");
	shader->specularMatrixT = qglGetUniformLocation(shader->program, "u_specularMatrixT");
	shader->colorModulate = qglGetUniformLocation(shader->program, "u_colorModulate");
	shader->colorAdd = qglGetUniformLocation(shader->program, "u_colorAdd");
	shader->fogColor = qglGetUniformLocation(shader->program, "u_fogColor");
	shader->diffuseColor = qglGetUniformLocation(shader->program, "u_diffuseColor");
	shader->specularColor = qglGetUniformLocation(shader->program, "u_specularColor");
	shader->glColor = qglGetUniformLocation(shader->program, "u_glColor");
	shader->alphaTest = qglGetUniformLocation(shader->program, "u_alphaTest");
	shader->specularExponent = qglGetUniformLocation(shader->program, "u_specularExponent");

	shader->modelMatrix = qglGetUniformLocation(shader->program, "u_modelMatrix");

	//Shader Matrices for the View Matrices
	GLuint viewMatricesUniformLocation = glGetUniformBlockIndex(shader->program, "ViewMatrices");
	int numBufferBindings = 0;
	shader->viewMatricesBinding = numBufferBindings++;
	glUniformBlockBinding(
			shader->program,
			viewMatricesUniformLocation,
			shader->viewMatricesBinding);

	//Shader Matrices for the Projection Matrix
	GLuint projectionMatrixUniformLocation = glGetUniformBlockIndex(shader->program, "ProjectionMatrix");
	shader->projectionMatrixBinding = numBufferBindings++;
	glUniformBlockBinding(
			shader->program,
			projectionMatrixUniformLocation,
			shader->projectionMatrixBinding);

	shader->modelViewMatrix = qglGetUniformLocation(shader->program, "u_modelViewMatrix");
	shader->textureMatrix = qglGetUniformLocation(shader->program, "u_textureMatrix");
	shader->clipPlane = qglGetUniformLocation(shader->program, "u_clipPlane");
	shader->fogMatrix = qglGetUniformLocation(shader->program, "u_fogMatrix");

	shader->attr_TexCoord = qglGetAttribLocation(shader->program, "attr_TexCoord");
	shader->attr_Tangent = qglGetAttribLocation(shader->program, "attr_Tangent");
	shader->attr_Bitangent = qglGetAttribLocation(shader->program, "attr_Bitangent");
	shader->attr_Normal = qglGetAttribLocation(shader->program, "attr_Normal");
	shader->attr_Vertex = qglGetAttribLocation(shader->program, "attr_Vertex");
	shader->attr_Color = qglGetAttribLocation(shader->program, "attr_Color");

	// Init default values
	for ( i = 0; i < MAX_FRAGMENT_IMAGES; i++ ) {
		idStr::snPrintf(buffer, sizeof(buffer), "u_fragmentMap%d", i);
		shader->u_fragmentMap[i] = qglGetUniformLocation(shader->program, buffer);
		qglUniform1i(shader->u_fragmentMap[i], i);
	}

	for ( i = 0; i < MAX_FRAGMENT_IMAGES; i++ ) {
		idStr::snPrintf(buffer, sizeof(buffer), "u_fragmentCubeMap%d", i);
		shader->u_fragmentCubeMap[i] = qglGetUniformLocation(shader->program, buffer);
		qglUniform1i(shader->u_fragmentCubeMap[i], i);
	}

	if (shader->textureMatrix >= 0) {
		// Load identity matrix for Texture marix
		GL_UniformMatrix4fv(offsetof(shaderProgram_t, textureMatrix), mat4_identity.ToFloatPtr());
	}

	static const GLfloat one[1] = { 1.0f };
	if (shader->alphaTest >= 0) {
		// Alpha test always pass by default
		GL_Uniform1fv(offsetof(shaderProgram_t, alphaTest), one);
	}

	if (shader->colorModulate >= 0) {
		static const GLfloat zero[1] = { 0.0f };
		GL_Uniform1fv(offsetof(shaderProgram_t, colorModulate), zero);
	}

	if (shader->colorAdd >= 0) {
		GL_Uniform1fv(offsetof(shaderProgram_t, colorAdd), one);
	}

	GL_CheckErrors();

	GL_UseProgram(NULL);
}

/*
=================
RB_GLSL_InitShaders

=================
*/
static bool RB_GLSL_InitShaders(void) {

	//Generate buffer for 2 * view matrices
	qglGenBuffers(1, &viewMatricesBuffer);
    glBindBuffer(GL_UNIFORM_BUFFER, viewMatricesBuffer);
    glBufferData(
            GL_UNIFORM_BUFFER,
            2 * 16 * sizeof(float),
            NULL,
            GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    for (int i = 0; i <= (NUM_DEPTH_HACK_PROJECTIONS+DEPTH_HACK_PROJECTION); ++i)
	{
		qglGenBuffers(1, &projectionMatricesBuffer[i]);
		glBindBuffer(GL_UNIFORM_BUFFER, projectionMatricesBuffer[i]);
		glBufferData(
				GL_UNIFORM_BUFFER,
				16 * sizeof(float),
				NULL,
				GL_STATIC_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}

    // main Interaction shader
	common->Printf("Loading main interaction shader\n");
	memset(&interactionShader, 0, sizeof(shaderProgram_t));

	R_LoadGLSLShader(interactionShaderVP, &interactionShader, GL_VERTEX_SHADER);
	R_LoadGLSLShader(interactionShaderFP, &interactionShader, GL_FRAGMENT_SHADER);

	if ( !R_LinkGLSLShader(&interactionShader, "interaction") && !R_ValidateGLSLProgram(&interactionShader)) {
		return false;
	} else {
		RB_GLSL_GetUniformLocations(&interactionShader);
	}

	// main Interaction shader, Phong version
	common->Printf("Loading main interaction shader (Phong) \n");
	memset(&interactionPhongShader, 0, sizeof(shaderProgram_t));

	R_LoadGLSLShader(interactionPhongShaderVP, &interactionPhongShader, GL_VERTEX_SHADER);
	R_LoadGLSLShader(interactionPhongShaderFP, &interactionPhongShader, GL_FRAGMENT_SHADER);

	if ( !R_LinkGLSLShader(&interactionPhongShader, "interactionPhong") &&
	        !R_ValidateGLSLProgram(&interactionPhongShader)) {
		return false;
	} else {
		RB_GLSL_GetUniformLocations(&interactionPhongShader);
	}

	// default diffuse shader
	common->Printf("Loading default diffuse shader\n");
	memset(&diffuseMapShader, 0, sizeof(shaderProgram_t));

	R_LoadGLSLShader(diffuseMapShaderVP, &diffuseMapShader, GL_VERTEX_SHADER);
	R_LoadGLSLShader(diffuseMapShaderFP, &diffuseMapShader, GL_FRAGMENT_SHADER);

	if ( !R_LinkGLSLShader(&diffuseMapShader, "diffuseMap") && !R_ValidateGLSLProgram(&diffuseMapShader)) {
		return false;
	} else {
		RB_GLSL_GetUniformLocations(&diffuseMapShader);
	}

	// Skybox cubemap shader
	common->Printf("Loading skybox cubemap shader\n");
	memset(&skyboxCubeShader, 0, sizeof(shaderProgram_t));

	R_LoadGLSLShader(skyboxCubeShaderVP, &skyboxCubeShader, GL_VERTEX_SHADER);
	R_LoadGLSLShader(cubeMapShaderFP, &skyboxCubeShader, GL_FRAGMENT_SHADER);   // Use the common "cubeMapShaderFP"

	if ( !R_LinkGLSLShader(&skyboxCubeShader, "skyboxCube") && !R_ValidateGLSLProgram(&skyboxCubeShader)) {
		return false;
	} else {
		RB_GLSL_GetUniformLocations(&skyboxCubeShader);
	}

	// Reflection cubemap shader
	common->Printf("Loading reflection cubemap shader\n");
	memset(&reflectionCubeShader, 0, sizeof(shaderProgram_t));

	R_LoadGLSLShader(reflectionCubeShaderVP, &reflectionCubeShader, GL_VERTEX_SHADER);
	R_LoadGLSLShader(cubeMapShaderFP, &reflectionCubeShader, GL_FRAGMENT_SHADER); // Use the common "cubeMapShaderFP"

	if ( !R_LinkGLSLShader(&reflectionCubeShader, "reflectionCube") &&
	        !R_ValidateGLSLProgram(&reflectionCubeShader)) {
		return false;
	} else {
		RB_GLSL_GetUniformLocations(&reflectionCubeShader);
	}

	// Diffuse cubemap shader
	common->Printf("Loading diffuse cubemap shader\n");
	memset(&diffuseCubeShader, 0, sizeof(shaderProgram_t));

	R_LoadGLSLShader(diffuseCubeShaderVP, &diffuseCubeShader, GL_VERTEX_SHADER);
	R_LoadGLSLShader(cubeMapShaderFP, &diffuseCubeShader, GL_FRAGMENT_SHADER); // Use the common "cubeMapShaderFP"

	if ( !R_LinkGLSLShader(&diffuseCubeShader, "diffuseCube") && !R_ValidateGLSLProgram(&diffuseCubeShader)) {
		return false;
	} else {
		RB_GLSL_GetUniformLocations(&diffuseCubeShader);
	}

	// Z Fill shader
	common->Printf("Loading Zfill shader\n");
	memset(&zfillShader, 0, sizeof(shaderProgram_t));

	R_LoadGLSLShader(zfillShaderVP, &zfillShader, GL_VERTEX_SHADER);
	R_LoadGLSLShader(zfillShaderFP, &zfillShader, GL_FRAGMENT_SHADER);

	if ( !R_LinkGLSLShader(&zfillShader, "zfill") && !R_ValidateGLSLProgram(&zfillShader)) {
		return false;
	} else {
		RB_GLSL_GetUniformLocations(&zfillShader);
	}

	// Z Fill shader, Clip planes version
	common->Printf("Loading Zfill shader (Clip plane version)\n");
	memset(&zfillClipShader, 0, sizeof(shaderProgram_t));

	R_LoadGLSLShader(zfillClipShaderVP, &zfillClipShader, GL_VERTEX_SHADER);
	R_LoadGLSLShader(zfillClipShaderFP, &zfillClipShader, GL_FRAGMENT_SHADER);

	if ( !R_LinkGLSLShader(&zfillClipShader, "zfillClip") && !R_ValidateGLSLProgram(&zfillClipShader)) {
		return false;
	} else {
		RB_GLSL_GetUniformLocations(&zfillClipShader);
	}

	// Fog shader
	common->Printf("Loading Fog shader\n");
	memset(&fogShader, 0, sizeof(shaderProgram_t));

	R_LoadGLSLShader(fogShaderVP, &fogShader, GL_VERTEX_SHADER);
	R_LoadGLSLShader(fogShaderFP, &fogShader, GL_FRAGMENT_SHADER);

	if ( !R_LinkGLSLShader(&fogShader, "fog") && !R_ValidateGLSLProgram(&fogShader)) {
		return false;
	} else {
		RB_GLSL_GetUniformLocations(&fogShader);
	}

	// BlendLight shader
	common->Printf("Loading BlendLight shader\n");
	memset(&blendLightShader, 0, sizeof(shaderProgram_t));

	R_LoadGLSLShader(blendLightShaderVP, &blendLightShader, GL_VERTEX_SHADER);
	R_LoadGLSLShader(fogShaderFP, &blendLightShader, GL_FRAGMENT_SHADER);       // Reuse the common "FogShaderFP"

	if ( !R_LinkGLSLShader(&blendLightShader, "blendLight") && !R_ValidateGLSLProgram(&blendLightShader)) {
		return false;
	} else {
		RB_GLSL_GetUniformLocations(&blendLightShader);
	}

	// Stencil shadow shader
	common->Printf("Loading Stencil shadow shader\n");
	memset(&stencilShadowShader, 0, sizeof(shaderProgram_t));

	R_LoadGLSLShader(stencilShadowShaderVP, &stencilShadowShader, GL_VERTEX_SHADER);
	R_LoadGLSLShader(stencilShadowShaderFP, &stencilShadowShader, GL_FRAGMENT_SHADER);

	if ( !R_LinkGLSLShader(&stencilShadowShader, "stencilShadow") && !R_ValidateGLSLProgram(&stencilShadowShader)) {
		return false;
	} else {
		RB_GLSL_GetUniformLocations(&stencilShadowShader);
	}

	return true;
}

/*
==================
R_ReloadGLSLPrograms_f
==================
*/
void R_ReloadGLSLPrograms_f(const idCmdArgs& args) {
	common->Printf("----- R_ReloadGLSLPrograms -----\n");

	if ( !RB_GLSL_InitShaders()) {
		common->Printf("GLSL shaders failed to init.\n");
	}

	common->Printf("-------------------------------\n");
}

/*
=================
RB_ComputeProjection

Compute the required projection matrix depth hacks
=================
*/
void RB_ComputeProjection(bool weaponDepthHack, float modelDepthHack, float *projection) {
	// Get the projection matrix
	float localProjectionMatrix[16];
	memcpy(localProjectionMatrix, backEnd.viewDef->projectionMatrix, sizeof(localProjectionMatrix));

	// Quick and dirty hacks on the projection matrix
	if ( weaponDepthHack ) {
		localProjectionMatrix[14] = backEnd.viewDef->projectionMatrix[14] * 0.25;
	} else if ( modelDepthHack != 0.0 ) {
		localProjectionMatrix[14] = backEnd.viewDef->projectionMatrix[14] - modelDepthHack;
	}

	memcpy(projection, localProjectionMatrix, sizeof(localProjectionMatrix));
}

/*
=================
RB_CalculateProjection

Compute which projection matrix buffer should be used
=================
*/

GLuint RB_CalculateProjection(const drawSurf_t * const surf) {
	GLuint result = NORMAL_PROJECTION;
	if ( surf->space->weaponDepthHack ) {
		result = WEAPON_PROJECTION;
	} else if ( surf->space->modelDepthHack > 0.0 &&
				surf->space->modelDepthHack <= 1.0 ) {
		result = DEPTH_HACK_PROJECTION + (GLuint)(surf->space->modelDepthHack * (float)NUM_DEPTH_HACK_PROJECTIONS);
	} else if (
	        //Is this set up as an orthographic projection?
            backEnd.viewDef->projectionMatrix[0] == 2.0f / 640.0f &&
			backEnd.viewDef->projectionMatrix[5] == -2.0f / 480.0f &&
			backEnd.viewDef->projectionMatrix[10] == -2.0f / 1.0f &&
			backEnd.viewDef->projectionMatrix[12] == -1.0f &&
			backEnd.viewDef->projectionMatrix[13] == 1.0f &&
			backEnd.viewDef->projectionMatrix[14] == -1.0f &&
			backEnd.viewDef->projectionMatrix[15] == 1.0f
			)
	{
		result = ORTHO_PROJECTION;
	}

	return result;
}

/*
==================
RB_GLSL_DrawInteraction
==================
*/
static void RB_GLSL_DrawInteraction(const drawInteraction_t* din) {
	static const GLfloat zero[1] = { 0 };
	static const GLfloat one[1] = { 1 };
	static const GLfloat oneScaled[1] = { 1 / 255.0f };
	static const GLfloat negOneScaled[1] = { -1.0f / 255.0f };

	// load all the vertex program parameters
	GL_Uniform4fv(offsetof(shaderProgram_t, localLightOrigin), din->localLightOrigin.ToFloatPtr());
	GL_Uniform4fv(offsetof(shaderProgram_t, localViewOrigin), din->localViewOrigin.ToFloatPtr());
	GL_UniformMatrix4fv(offsetof(shaderProgram_t, lightProjection), din->lightProjection.ToFloatPtr());
	GL_Uniform4fv(offsetof(shaderProgram_t, bumpMatrixS), din->bumpMatrix[0].ToFloatPtr());
	GL_Uniform4fv(offsetof(shaderProgram_t, bumpMatrixT), din->bumpMatrix[1].ToFloatPtr());
	GL_Uniform4fv(offsetof(shaderProgram_t, diffuseMatrixS), din->diffuseMatrix[0].ToFloatPtr());
	GL_Uniform4fv(offsetof(shaderProgram_t, diffuseMatrixT), din->diffuseMatrix[1].ToFloatPtr());
	GL_Uniform4fv(offsetof(shaderProgram_t, specularMatrixS), din->specularMatrix[0].ToFloatPtr());
	GL_Uniform4fv(offsetof(shaderProgram_t, specularMatrixT), din->specularMatrix[1].ToFloatPtr());

	switch ( din->vertexColor ) {
	case SVC_MODULATE: {
		GL_Uniform1fv(offsetof(shaderProgram_t, colorModulate), oneScaled);
		GL_Uniform1fv(offsetof(shaderProgram_t, colorAdd), zero);
		break;
	}
	case SVC_INVERSE_MODULATE: {
		GL_Uniform1fv(offsetof(shaderProgram_t, colorModulate), negOneScaled);
		GL_Uniform1fv(offsetof(shaderProgram_t, colorAdd), one);
		break;
	}
	default:
	case SVC_IGNORE:
		// This is already the default values (zero, one)
		break;
	}

	// set the constant colors
	GL_Uniform4fv(offsetof(shaderProgram_t, diffuseColor), din->diffuseColor.ToFloatPtr());
	GL_Uniform4fv(offsetof(shaderProgram_t, specularColor), din->specularColor.ToFloatPtr());

	// set the textures

	// texture 0 will be the per-surface bump map
	// NB: Texture 0 is expected to be active at this point
	din->bumpImage->Bind();

	// texture 1 will be the light falloff texture
	GL_SelectTexture(1);
	din->lightFalloffImage->Bind();

	// texture 2 will be the light projection texture
	GL_SelectTexture(2);
	din->lightImage->Bind();

	// texture 3 is the per-surface diffuse map
	GL_SelectTexture(3);
	din->diffuseImage->Bind();

	// texture 4 is the per-surface specular map
	GL_SelectTexture(4);
	din->specularImage->Bind();

	// Be sure to activate Texture 0 for next interaction, or next pass
	GL_SelectTexture(0);

	// draw it
	RB_DrawElementsWithCounters(din->surf);

	// Restore color modulation state to default values
	if ( din->vertexColor != SVC_IGNORE ) {
		GL_Uniform1fv(offsetof(shaderProgram_t, colorModulate), zero);
		GL_Uniform1fv(offsetof(shaderProgram_t, colorAdd), one);
	}
}

/*
=============
RB_CreateSingleDrawInteractions

This can be used by different draw_* backends to decompose a complex light / surface
interaction into primitive interactions
=============
*/
static void
RB_GLSL_CreateSingleDrawInteractions(const drawSurf_t* surf, void (* DrawInteraction)(const drawInteraction_t*), const viewLight_t* vLight) {
	const idMaterial* surfaceShader = surf->material;
	const float* surfaceRegs = surf->shaderRegisters;
	const idMaterial* lightShader = vLight->lightShader;
	const float* lightRegs = vLight->shaderRegisters;
	drawInteraction_t inter;

	if ( r_skipInteractions.GetBool() || !surf->geoFrontEnd || !surf->ambientCache ) {
		return;
	}

	// change the scissor if needed
	if ( r_useScissor.GetBool() && !backEnd.currentScissor.Equals(surf->scissorRect)) {
		backEnd.currentScissor = surf->scissorRect;
		if (( backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1 ) < 0.0f ||
		        ( backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1 ) < 0.0f ) {
			backEnd.currentScissor = backEnd.viewDef->scissor;
		}
		qglScissor(backEnd.viewDef->viewport.x1 + backEnd.currentScissor.x1,
		           backEnd.viewDef->viewport.y1 + backEnd.currentScissor.y1,
		           backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1,
		           backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1);
	}

	inter.surf = surf;
	inter.lightFalloffImage = vLight->falloffImage;

	R_GlobalPointToLocal(surf->space->modelMatrix, vLight->globalLightOrigin, inter.localLightOrigin.ToVec3());
	R_GlobalPointToLocal(surf->space->modelMatrix, backEnd.viewDef->renderView.vieworg, inter.localViewOrigin.ToVec3());
	inter.localLightOrigin[3] = 0;
	inter.localViewOrigin[3] = 1;
	inter.ambientLight = lightShader->IsAmbientLight();

	// the base projections may be modified by texture matrix on light stages
	idPlane lightProject[4];

	for ( int i = 0; i < 4; i++ ) {
		R_GlobalPlaneToLocal(surf->space->modelMatrix, vLight->lightProject[i], lightProject[i]);
	}

	for ( int lightStageNum = 0; lightStageNum < lightShader->GetNumStages(); lightStageNum++ ) {
		const shaderStage_t* lightStage = lightShader->GetStage(lightStageNum);

		// ignore stages that fail the condition
		if ( !lightRegs[lightStage->conditionRegister] ) {
			continue;
		}

		inter.lightImage = lightStage->texture.image;

		inter.lightProjection[0] = lightProject[0].ToVec4(); // S
		inter.lightProjection[1] = lightProject[1].ToVec4(); // T
		inter.lightProjection[2] = lightProject[3].ToVec4(); // CAUTION! this is the 4th vector. R = Falloff
		inter.lightProjection[3] = lightProject[2].ToVec4(); // CAUTION! this is the 3rd vector. Q

		// now multiply the texgen by the light texture matrix
		if ( lightStage->texture.hasMatrix ) {
			float lightTextureMatrix[16];
			RB_GetShaderTextureMatrix(lightRegs, &lightStage->texture, lightTextureMatrix);
			RB_BakeTextureMatrixIntoTexgen(inter.lightProjection, lightTextureMatrix);
		}

		inter.bumpImage = NULL;
		inter.specularImage = NULL;
		inter.diffuseImage = NULL;
		inter.diffuseColor[0] = inter.diffuseColor[1] = inter.diffuseColor[2] = inter.diffuseColor[3] = 0;
		inter.specularColor[0] = inter.specularColor[1] = inter.specularColor[2] = inter.specularColor[3] = 0;

		float lightColor[4];

		const float lightscale = r_lightScale.GetFloat();
		lightColor[0] = lightscale * lightRegs[lightStage->color.registers[0]];
		lightColor[1] = lightscale * lightRegs[lightStage->color.registers[1]];
		lightColor[2] = lightscale * lightRegs[lightStage->color.registers[2]];
		lightColor[3] = lightRegs[lightStage->color.registers[3]];

		// go through the individual stages
		for ( int surfaceStageNum = 0; surfaceStageNum < surfaceShader->GetNumStages(); surfaceStageNum++ ) {
			const shaderStage_t* surfaceStage = surfaceShader->GetStage(surfaceStageNum);

			switch ( surfaceStage->lighting ) {
			case SL_AMBIENT: {
				// ignore ambient stages while drawing interactions
				break;
			}
			case SL_BUMP: {
				// ignore stage that fails the condition
				if ( !surfaceRegs[surfaceStage->conditionRegister] ) {
					break;
				}

				// draw any previous interaction
				RB_SubmittInteraction(&inter, DrawInteraction);
				inter.diffuseImage = NULL;
				inter.specularImage = NULL;
				RB_SetDrawInteraction(surfaceStage, surfaceRegs, &inter.bumpImage, inter.bumpMatrix, NULL);
				break;
			}
			case SL_DIFFUSE: {
				// ignore stage that fails the condition
				if ( !surfaceRegs[surfaceStage->conditionRegister] ) {
					break;
				}

				if ( inter.diffuseImage ) {
					RB_SubmittInteraction(&inter, DrawInteraction);
				}

				RB_SetDrawInteraction(surfaceStage, surfaceRegs, &inter.diffuseImage,
				                      inter.diffuseMatrix, inter.diffuseColor.ToFloatPtr());
				inter.diffuseColor[0] *= lightColor[0];
				inter.diffuseColor[1] *= lightColor[1];
				inter.diffuseColor[2] *= lightColor[2];
				inter.diffuseColor[3] *= lightColor[3];
				inter.vertexColor = surfaceStage->vertexColor;
				break;
			}
			case SL_SPECULAR: {
				// ignore stage that fails the condition
				if ( !surfaceRegs[surfaceStage->conditionRegister] ) {
					break;
				}

				if ( inter.specularImage ) {
					RB_SubmittInteraction(&inter, DrawInteraction);
				}

				RB_SetDrawInteraction(surfaceStage, surfaceRegs, &inter.specularImage,
				                      inter.specularMatrix, inter.specularColor.ToFloatPtr());
				inter.specularColor[0] *= lightColor[0];
				inter.specularColor[1] *= lightColor[1];
				inter.specularColor[2] *= lightColor[2];
				inter.specularColor[3] *= lightColor[3];
				inter.vertexColor = surfaceStage->vertexColor;
				break;
			}
			}
		}

		// draw the final interaction
		RB_SubmittInteraction(&inter, DrawInteraction);
	}
}

/*
=============
RB_GLSL_CreateDrawInteractions

=============
*/
static void RB_GLSL_CreateDrawInteractions(const drawSurf_t* surf, const viewLight_t* vLight, const int depthFunc = GLS_DEPTHFUNC_EQUAL) {
	if ( !surf ) {
		return;
	}

	// perform setup here that will be constant for all interactions
	GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHMASK | depthFunc);

	// bind the vertex and fragment shader
	if ( r_usePhong.GetBool()) {
		GL_UseProgram(&interactionPhongShader);

		// Set the specular exponent now (NB: it could be cached instead)
		const float f = r_specularExponent.GetFloat();
		GL_Uniform1fv(offsetof(shaderProgram_t, specularExponent), &f);
	} else {
		GL_UseProgram(&interactionShader);
	}

	glBindBufferBase(
		GL_UNIFORM_BUFFER,
		backEnd.glState.currentProgram->viewMatricesBinding,
		viewMatricesBuffer);


	// Setup attributes arrays
	// Vertex attribute is always enabled
	// Color attribute is always enabled
	// TexCoord attribute is always enabled
	// Enable the rest
	GL_EnableVertexAttribArray(ATTR_TANGENT);
	GL_EnableVertexAttribArray(ATTR_BITANGENT);
	GL_EnableVertexAttribArray(ATTR_NORMAL);

	backEnd.currentSpace = NULL;

	for ( ; surf; surf = surf->nextOnLight ) {
		// perform setup here that will not change over multiple interaction passes

		if ( surf->space != backEnd.currentSpace ) {
			GL_UniformMatrix4fv(offsetof(shaderProgram_t, modelMatrix), surf->space->modelMatrix);

			glBindBufferBase(
					GL_UNIFORM_BUFFER,
					backEnd.glState.currentProgram->projectionMatrixBinding,
					projectionMatricesBuffer[RB_CalculateProjection(surf)]);
		}

		// Hack Depth Range if necessary
		bool bNeedRestoreDepthRange = false;
		if (surf->space->weaponDepthHack && surf->space->modelDepthHack == 0.0f) {
			qglDepthRangef(0.0f, 0.5f);
			bNeedRestoreDepthRange = true;
		}

		// set the vertex pointers
		idDrawVert* ac = (idDrawVert*) vertexCache.Position(surf->ambientCache);

		GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Normal), 3, GL_FLOAT, false, sizeof(idDrawVert),
		                       ac->normal.ToFloatPtr());
		GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Bitangent), 3, GL_FLOAT, false, sizeof(idDrawVert),
		                       ac->tangents[1].ToFloatPtr());
		GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Tangent), 3, GL_FLOAT, false, sizeof(idDrawVert),
		                       ac->tangents[0].ToFloatPtr());
		GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_TexCoord), 2, GL_FLOAT, false, sizeof(idDrawVert),
		                       ac->st.ToFloatPtr());
		GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Vertex), 3, GL_FLOAT, false, sizeof(idDrawVert),
		                       ac->xyz.ToFloatPtr());
		GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Color), 4, GL_UNSIGNED_BYTE, false, sizeof(idDrawVert),
		                       (void*) &ac->color);

		// this may cause RB_GLSL_DrawInteraction to be exacuted multiple
		// times with different colors and images if the surface or light have multiple layers
		RB_GLSL_CreateSingleDrawInteractions(surf, RB_GLSL_DrawInteraction, vLight);

		// Restore the Depth Range in case it have been hacked
		if ( bNeedRestoreDepthRange ) {
			qglDepthRangef( 0.0f, 1.0f );
		}

		backEnd.currentSpace = surf->space;
	}

	backEnd.currentSpace = NULL;

	// Restore attributes arrays
	// Vertex attribute is always enabled
	// Color attribute is always enabled
	// TexCoord attribute is always enabled
	GL_DisableVertexAttribArray(ATTR_TANGENT);
	GL_DisableVertexAttribArray(ATTR_BITANGENT);
	GL_DisableVertexAttribArray(ATTR_NORMAL);
}


/*
=====================
RB_T_GLSL_Shadow

the shadow volumes face INSIDE
=====================
*/
static void RB_T_GLSL_Shadow(const drawSurf_t* surf, const viewLight_t* vLight) {

	//const srfTriangles_t* tri = surf->geo;

	if ( !surf->shadowCache ) {
		return;
	}

	// set the light position for the vertex program to project the rear surfaces
	if ( surf->space != backEnd.currentSpace ) {
		idVec4 localLight;

		R_GlobalPointToLocal(surf->space->modelMatrix, vLight->globalLightOrigin, localLight.ToVec3());
		localLight.w = 0.0f;
		GL_Uniform4fv(offsetof(shaderProgram_t, localLightOrigin), localLight.ToFloatPtr());
	}

	GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Vertex), 4, GL_FLOAT, false, sizeof(shadowCache_t),
	                       vertexCache.Position(surf->shadowCache));

	// we always draw the sil planes, but we may not need to draw the front or rear caps
	int numIndexes;
	bool external = false;

	if ( !r_useExternalShadows.GetInteger()) {
		numIndexes = surf->numIndexes;
	} else if ( r_useExternalShadows.GetInteger() == 2 ) { // force to no caps for testing
		numIndexes = surf->numShadowIndexesNoCaps;
	} else if ( !( surf->dsFlags & DSF_VIEW_INSIDE_SHADOW )) {
		// if we aren't inside the shadow projection, no caps are ever needed needed
		numIndexes = surf->numShadowIndexesNoCaps;
		external = true;
	} else if ( !vLight->viewInsideLight && !( surf->shadowCapPlaneBits & SHADOW_CAP_INFINITE )) {
		// if we are inside the shadow projection, but outside the light, and drawing
		// a non-infinite shadow, we can skip some caps
		if ( vLight->viewSeesShadowPlaneBits & surf->shadowCapPlaneBits ) {
			// we can see through a rear cap, so we need to draw it, but we can skip the
			// caps on the actual surface
			numIndexes = surf->numShadowIndexesNoFrontCaps;
		} else {
			// we don't need to draw any caps
			numIndexes = surf->numShadowIndexesNoCaps;
		}

		external = true;
	} else {
		// must draw everything
		numIndexes = surf->numIndexes;
	}

	// depth-fail stencil shadows
	if ( !external ) {
		qglStencilOpSeparate(backEnd.viewDef->isMirror ? GL_FRONT : GL_BACK, GL_KEEP, GL_DECR, GL_KEEP);
		qglStencilOpSeparate(backEnd.viewDef->isMirror ? GL_BACK : GL_FRONT, GL_KEEP, GL_INCR, GL_KEEP);
	} else {
		// traditional depth-pass stencil shadows
		qglStencilOpSeparate(backEnd.viewDef->isMirror ? GL_FRONT : GL_BACK, GL_KEEP, GL_KEEP, GL_INCR);
		qglStencilOpSeparate(backEnd.viewDef->isMirror ? GL_BACK : GL_FRONT, GL_KEEP, GL_KEEP, GL_DECR);
	}
	RB_DrawShadowElementsWithCounters(surf, numIndexes);

	// patent-free work around
	/*
	if (!external) {
	  // "preload" the stencil buffer with the number of volumes
	  // that get clipped by the near or far clip plane
	  qglStencilOp(GL_KEEP, GL_DECR, GL_DECR);
	  GL_Cull(CT_FRONT_SIDED);
	  RB_DrawShadowElementsWithCounters(tri, numIndexes);
	  qglStencilOp(GL_KEEP, GL_INCR, GL_INCR);
	  GL_Cull(CT_BACK_SIDED);
	  RB_DrawShadowElementsWithCounters(tri, numIndexes);
	}

	// traditional depth-pass stencil shadows
	qglStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
	GL_Cull(CT_FRONT_SIDED);
	RB_DrawShadowElementsWithCounters(tri, numIndexes);

	qglStencilOp(GL_KEEP, GL_KEEP, GL_DECR);
	GL_Cull(CT_BACK_SIDED);
	RB_DrawShadowElementsWithCounters(tri, numIndexes);
	*/
}


/*
======================
RB_RenderDrawSurfChainWithFunction
======================
*/
void RB_GLSL_RenderDrawSurfChainWithFunction(const drawSurf_t* drawSurfs,
        void (* triFunc_)(const drawSurf_t*, const viewLight_t*), const viewLight_t* vLight) {
	const drawSurf_t* drawSurf;

	backEnd.currentSpace = NULL;

	for ( drawSurf = drawSurfs; drawSurf; drawSurf = drawSurf->nextOnLight ) {

		// Change the MVP matrix if needed
		if ( drawSurf->space != backEnd.currentSpace ) {
			GL_UniformMatrix4fv(offsetof(shaderProgram_t, modelMatrix), drawSurf->space->modelMatrix);

			glBindBufferBase(
					GL_UNIFORM_BUFFER,
					backEnd.glState.currentProgram->projectionMatrixBinding,
					projectionMatricesBuffer[RB_CalculateProjection(drawSurf)]);
		}

		// Hack Depth Range if necessary
		bool bNeedRestoreDepthRange = false;
		if (drawSurf->space->weaponDepthHack && drawSurf->space->modelDepthHack == 0.0f) {
			qglDepthRangef(0.0f, 0.5f);
			bNeedRestoreDepthRange = true;
		}

		// change the scissor if needed
		if ( r_useScissor.GetBool() && !backEnd.currentScissor.Equals(drawSurf->scissorRect)) {
			backEnd.currentScissor = drawSurf->scissorRect;
			if (( backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1 ) < 0.0f ||
			        ( backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1 ) < 0.0f ) {
				backEnd.currentScissor = backEnd.viewDef->scissor;
			}
			qglScissor(backEnd.viewDef->viewport.x1 + backEnd.currentScissor.x1,
			           backEnd.viewDef->viewport.y1 + backEnd.currentScissor.y1,
			           backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1,
			           backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1);

		}

		// render it
		triFunc_(drawSurf, vLight);

		// Restore the Depth Range in case it have been hacked
		if ( bNeedRestoreDepthRange ) {
			qglDepthRangef( 0.0f, 1.0f );
		}

		backEnd.currentSpace = drawSurf->space;
	}

	backEnd.currentSpace = NULL;
}

/*
=====================
RB_GLSL_StencilShadowPass

Stencil test should already be enabled, and the stencil buffer should have
been set to 128 on any surfaces that might receive shadows
=====================
*/
void RB_GLSL_StencilShadowPass(const drawSurf_t* drawSurfs, const viewLight_t* vLight) {

	//////////////
	// Skip cases
	//////////////

	if ( !r_shadows.GetBool()) {
		return;
	}

	if ( !drawSurfs ) {
		return;
	}

	//////////////////
	// Setup GL state
	//////////////////

	// Expected client GL state:
	// Vertex attribute enabled
	// Color attribute enabled

	// Use the stencil shadow shader
	GL_UseProgram(&stencilShadowShader);

	glBindBufferBase(
            GL_UNIFORM_BUFFER,
			backEnd.glState.currentProgram->viewMatricesBinding,
			viewMatricesBuffer);

	// Setup attributes arrays
	// Vertex attribute is always enabled
	// Disable Color attribute (as it is enabled by default)
	// Disable TexCoord attribute (as it is enabled by default)
	GL_DisableVertexAttribArray(ATTR_COLOR);
	GL_DisableVertexAttribArray(ATTR_TEXCOORD);

	// don't write to the color buffer, just the stencil buffer
	GL_State(GLS_DEPTHMASK | GLS_COLORMASK | GLS_ALPHAMASK | GLS_DEPTHFUNC_LESS);

	if ( r_shadowPolygonFactor.GetFloat() || r_shadowPolygonOffset.GetFloat()) {
		qglPolygonOffset(r_shadowPolygonFactor.GetFloat(), -r_shadowPolygonOffset.GetFloat());
		qglEnable(GL_POLYGON_OFFSET_FILL);
	}

	qglStencilFunc(GL_ALWAYS, 1, 255);

	// Culling will be done two side for shadows
	GL_Cull(CT_TWO_SIDED);

	RB_GLSL_RenderDrawSurfChainWithFunction(drawSurfs, RB_T_GLSL_Shadow, vLight);

	// Restore culling
	GL_Cull(CT_FRONT_SIDED);

	if ( r_shadowPolygonFactor.GetFloat() || r_shadowPolygonOffset.GetFloat()) {
		qglDisable(GL_POLYGON_OFFSET_FILL);
	}

	qglStencilFunc(GL_GEQUAL, 128, 255);
	qglStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);


	// Restore attributes arrays
	// Vertex attribute is always enabled
	// Re-enable Color attribute (as it is enabled by default)
	// Re-enable TexCoord attribute (as it is enabled by default)
	GL_EnableVertexAttribArray(ATTR_COLOR);
	GL_EnableVertexAttribArray(ATTR_TEXCOORD);
}

/*
==================
RB_GLSL_DrawInteractions
==================
*/
void RB_GLSL_DrawInteractions(void) {

	///////////////////////
	// For each light loop
	///////////////////////

	const viewLight_t* vLight;
	//
	// for each light, perform adding and shadowing
	//
	for ( vLight = backEnd.viewDef->viewLights; vLight; vLight = vLight->next ) {

		//////////////
		// Skip Cases
		//////////////

		// do fogging later
		if ( vLight->lightShader->IsFogLight()) {
			continue;
		}

		if ( vLight->lightShader->IsBlendLight()) {
			continue;
		}

		if ( !vLight->localInteractions && !vLight->globalInteractions
		        && !vLight->translucentInteractions ) {
			continue;
		}

		//////////////////
		// Setup GL state
		//////////////////

		// clear the stencil buffer if needed
		if ( vLight->globalShadows || vLight->localShadows ) {

			if ( r_useScissor.GetBool() && !backEnd.currentScissor.Equals(vLight->scissorRect)) {
				backEnd.currentScissor = vLight->scissorRect;
				if (( backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1 ) < 0.0f ||
				        ( backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1 ) < 0.0f ) {
					backEnd.currentScissor = backEnd.viewDef->scissor;
				}
				qglScissor(backEnd.viewDef->viewport.x1 + backEnd.currentScissor.x1,
				           backEnd.viewDef->viewport.y1 + backEnd.currentScissor.y1,
				           backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1,
				           backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1);
			}

			qglClear(GL_STENCIL_BUFFER_BIT);
		} else {
			// no shadows, so no need to read or write the stencil buffer
			// we might in theory want to use GL_ALWAYS instead of disabling
			// completely, to satisfy the invarience rules
			qglStencilFunc(GL_ALWAYS, 128, 255);
		}

		RB_GLSL_StencilShadowPass(vLight->globalShadows, vLight);
		RB_GLSL_CreateDrawInteractions(vLight->localInteractions, vLight);
		RB_GLSL_StencilShadowPass(vLight->localShadows, vLight);
		RB_GLSL_CreateDrawInteractions(vLight->globalInteractions, vLight);

		// translucent surfaces never get stencil shadowed
		if ( r_skipTranslucent.GetBool()) {
			continue;
		}

		qglStencilFunc(GL_ALWAYS, 128, 255);
		RB_GLSL_CreateDrawInteractions(vLight->translucentInteractions, vLight, GLS_DEPTHFUNC_LESS);
	}

	// disable stencil shadow test
	qglStencilFunc(GL_ALWAYS, 128, 255);
}

// NB: oh, a nice global variable. Argh....
static idPlane fogPlanes[4];

/*
=====================
RB_T_BasicFog
=====================
*/
static void RB_T_GLSL_BasicFog(const drawSurf_t* surf, const viewLight_t* vLight) {
	if ( backEnd.currentSpace != surf->space ) {
		idPlane transfoFogPlane[4];

		//S
		R_GlobalPlaneToLocal(surf->space->modelMatrix, fogPlanes[0], transfoFogPlane[0]);
		transfoFogPlane[0][3] += 0.5;
		//T
		transfoFogPlane[1][0] = transfoFogPlane[1][1] = transfoFogPlane[1][2] = 0;
		transfoFogPlane[1][3] = 0.5;
		//T
		R_GlobalPlaneToLocal(surf->space->modelMatrix, fogPlanes[2], transfoFogPlane[2]);
		transfoFogPlane[2][3] += FOG_ENTER;
		//S
		R_GlobalPlaneToLocal(surf->space->modelMatrix, fogPlanes[3], transfoFogPlane[3]);

		idMat4 fogMatrix;
		fogMatrix[0] = transfoFogPlane[0].ToVec4();
		fogMatrix[1] = transfoFogPlane[1].ToVec4();
		fogMatrix[2] = transfoFogPlane[2].ToVec4();
		fogMatrix[3] = transfoFogPlane[3].ToVec4();

		GL_UniformMatrix4fv(offsetof(shaderProgram_t, fogMatrix), fogMatrix.ToFloatPtr());
	}

	idDrawVert* ac = (idDrawVert*) vertexCache.Position(surf->ambientCache);

	GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Vertex), 3, GL_FLOAT, false, sizeof(idDrawVert),
	                       ac->xyz.ToFloatPtr());

	RB_DrawElementsWithCounters(surf);
}

/*
==================
RB_FogPass
==================
*/
void RB_GLSL_FogPass(const drawSurf_t* drawSurfs, const drawSurf_t* drawSurfs2, const viewLight_t* vLight) {

	drawSurf_t ds;
	const idMaterial* lightShader;
	const shaderStage_t* stage;
	const float* regs;

	// create a surface for the light frustom triangles, which are oriented drawn side out
	const srfTriangles_t* frustumTris = vLight->frustumTris;

	// if we ran out of vertex cache memory, skip it
	if ( !frustumTris->ambientCache ) {
		return;
	}

	// Initial expected GL state:
	// Texture 0 is active, and bound to NULL
	// Vertex attribute array is enabled
	// All other attributes array are disabled
	// No shaders active

	GL_UseProgram(&fogShader);

	glBindBufferBase(
			GL_UNIFORM_BUFFER,
			backEnd.glState.currentProgram->viewMatricesBinding,
			viewMatricesBuffer);

	// Setup attributes arrays
	// Vertex attribute is always enabled
	// Disable Color attribute (as it is enabled by default)
	// Disable TexCoord attribute (as it is enabled by default)
	GL_DisableVertexAttribArray(ATTR_COLOR);
	GL_DisableVertexAttribArray(ATTR_TEXCOORD);

	memset(&ds, 0, sizeof(ds));
	ds.space = &backEnd.viewDef->worldSpace;
	//ds.geo = frustumTris;
	ds.ambientCache = frustumTris->ambientCache;
    ds.indexCache = frustumTris->indexCache;
    ds.shadowCache = frustumTris->shadowCache;
    ds.numIndexes = frustumTris->numIndexes;
	ds.scissorRect = backEnd.viewDef->scissor;

	// find the current color and density of the fog
	lightShader = vLight->lightShader;
	regs = vLight->shaderRegisters;
	// assume fog shaders have only a single stage
	stage = lightShader->GetStage(0);

	float lightColor[4];

	lightColor[0] = regs[stage->color.registers[0]];
	lightColor[1] = regs[stage->color.registers[1]];
	lightColor[2] = regs[stage->color.registers[2]];
	lightColor[3] = regs[stage->color.registers[3]];

	// FogColor
	GL_Uniform4fv(offsetof(shaderProgram_t, fogColor), lightColor);

	// calculate the falloff planes
	const float a = ( lightColor[3] <= 1.0 ) ? -0.5f / DEFAULT_FOG_DISTANCE : -0.5f / lightColor[3];

	// texture 0 is the falloff image
	// It is expected to be already active
	globalImages->fogImage->Bind();

	fogPlanes[0][0] = a * backEnd.viewDef->worldSpace.eyeViewMatrix[2][2];
	fogPlanes[0][1] = a * backEnd.viewDef->worldSpace.eyeViewMatrix[2][6];
	fogPlanes[0][2] = a * backEnd.viewDef->worldSpace.eyeViewMatrix[2][10];
	fogPlanes[0][3] = a * backEnd.viewDef->worldSpace.eyeViewMatrix[2][14];

	fogPlanes[1][0] = a * backEnd.viewDef->worldSpace.eyeViewMatrix[2][0];
	fogPlanes[1][1] = a * backEnd.viewDef->worldSpace.eyeViewMatrix[2][4];
	fogPlanes[1][2] = a * backEnd.viewDef->worldSpace.eyeViewMatrix[2][8];
	fogPlanes[1][3] = a * backEnd.viewDef->worldSpace.eyeViewMatrix[2][12];

	// texture 1 is the entering plane fade correction
	GL_SelectTexture(1);
	globalImages->fogEnterImage->Bind();
	// reactive texture 0 for next passes
	GL_SelectTexture(0);

	// T will get a texgen for the fade plane, which is always the "top" plane on unrotated lights
	fogPlanes[2][0] = 0.001f * vLight->fogPlane[0];
	fogPlanes[2][1] = 0.001f * vLight->fogPlane[1];
	fogPlanes[2][2] = 0.001f * vLight->fogPlane[2];
	fogPlanes[2][3] = 0.001f * vLight->fogPlane[3];

	// S is based on the view origin
	const float s = backEnd.viewDef->renderView.vieworg * fogPlanes[2].Normal() + fogPlanes[2][3];
	fogPlanes[3][0] = 0;
	fogPlanes[3][1] = 0;
	fogPlanes[3][2] = 0;
	fogPlanes[3][3] = FOG_ENTER + s;

	// draw it
	GL_State(GLS_DEPTHMASK | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_EQUAL);
	RB_GLSL_RenderDrawSurfChainWithFunction(drawSurfs, RB_T_GLSL_BasicFog, vLight);
	RB_GLSL_RenderDrawSurfChainWithFunction(drawSurfs2, RB_T_GLSL_BasicFog, vLight);

	// the light frustum bounding planes aren't in the depth buffer, so use depthfunc_less instead
	// of depthfunc_equal
	GL_State(GLS_DEPTHMASK | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_LESS);
	GL_Cull(CT_BACK_SIDED);
	RB_GLSL_RenderDrawSurfChainWithFunction(&ds, RB_T_GLSL_BasicFog, vLight);
	// Restore culling
	GL_Cull(CT_FRONT_SIDED);
	GL_State(GLS_DEPTHMASK | GLS_DEPTHFUNC_EQUAL); // Restore DepthFunc

	// Restore attributes arrays
	// Vertex attribute is always enabled
	// Re-enable Color attribute (as it is enabled by default)
	// Re-enable TexCoord attribute (as it is enabled by default)
	GL_EnableVertexAttribArray(ATTR_COLOR);
	GL_EnableVertexAttribArray(ATTR_TEXCOORD);
}

/*
==================
RB_T_FillDepthBuffer
==================
*/
void RB_T_GLSL_FillDepthBuffer(const drawSurf_t* surf) {

	const idMaterial* const shader = surf->material;

	//////////////
	// Skip cases
	//////////////

	if ( !shader->IsDrawn()) {
		return;
	}

	//const srfTriangles_t* const tri = surf->geo;

	// some deforms may disable themselves by setting numIndexes = 0
	if ( !surf->numIndexes ) {
		return;
	}

	// translucent surfaces don't put anything in the depth buffer and don't
	// test against it, which makes them fail the mirror clip plane operation
	if ( shader->Coverage() == MC_TRANSLUCENT ) {
		return;
	}

	if ( !surf->ambientCache ) {
		return;
	}

	// get the expressions for conditionals / color / texcoords
	const float* const regs = surf->shaderRegisters;

	// if all stages of a material have been conditioned off, don't do anything
	int stage;
	for ( stage = 0; stage < shader->GetNumStages(); stage++ ) {
		const shaderStage_t* pStage = shader->GetStage(stage);

		// check the stage enable condition
		if ( regs[pStage->conditionRegister] != 0 ) {
			break;
		}
	}

	if ( stage == shader->GetNumStages()) {
		return;
	}

	///////////////////////////////////////////
	// GL Shader setup for the current surface
	///////////////////////////////////////////

	// update the clip plane if needed
	if ( backEnd.viewDef->numClipPlanes && surf->space != backEnd.currentSpace ) {
		idPlane plane;

		R_GlobalPlaneToLocal(surf->space->modelMatrix, backEnd.viewDef->clipPlanes[0], plane);
		plane[3] += 0.5;  // the notch is in the middle

		GL_Uniform4fv(offsetof(shaderProgram_t, clipPlane), plane.ToFloatPtr());
	}

	// set polygon offset if necessary
	// NB: will be restored at the end of the process
	if ( shader->TestMaterialFlag(MF_POLYGONOFFSET)) {
		qglEnable(GL_POLYGON_OFFSET_FILL);
		qglPolygonOffset(r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * shader->GetPolygonOffset());
	}

	// Color
	// black by default
	float color[4] = { 0, 0, 0, 1 };
	// subviews will just down-modulate the color buffer by overbright
	// NB: will be restored at end of the process
	if ( shader->GetSort() == SS_SUBVIEW ) {
		GL_State(GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO | GLS_DEPTHFUNC_LESS);
		color[0] = color[1] = color[2] = 1.0f; // NB: was 1.0 / backEnd.overBright );
	}
	GL_Uniform4fv(offsetof(shaderProgram_t, glColor), color);

	// Get vertex data
	idDrawVert* ac = (idDrawVert*) vertexCache.Position(surf->ambientCache);

	// Setup attribute pointers
	GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Vertex), 3, GL_FLOAT, false, sizeof(idDrawVert),
	                       ac->xyz.ToFloatPtr());
	GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_TexCoord), 2, GL_FLOAT, false, sizeof(idDrawVert),
	                       ac->st.ToFloatPtr());

	bool drawSolid = false;

	if ( shader->Coverage() == MC_OPAQUE ) {
		drawSolid = true;
	}

	////////////////////////////////
	// Perforated surfaces handling
	////////////////////////////////

	// we may have multiple alpha tested stages
	if ( shader->Coverage() == MC_PERFORATED ) {
		// if the only alpha tested stages are condition register omitted,
		// draw a normal opaque surface
		bool didDraw = false;

		///////////////////////
		// For each stage loop
		///////////////////////

		// perforated surfaces may have multiple alpha tested stages
		for ( stage = 0; stage < shader->GetNumStages(); stage++ ) {
			const shaderStage_t* pStage = shader->GetStage(stage);

			//////////////
			// Skip cases
			//////////////

			if ( !pStage->hasAlphaTest ) {
				continue;
			}

			// check the stage enable condition
			if ( regs[pStage->conditionRegister] == 0 ) {
				continue;
			}

			// if we at least tried to draw an alpha tested stage,
			// we won't draw the opaque surface
			didDraw = true;

			// set the alpha modulate
			color[3] = regs[pStage->color.registers[3]];

			// skip the entire stage if alpha would be black
			if ( color[3] <= 0 ) {
				continue;
			}

			//////////////////////////
			// GL Setup for the stage
			//////////////////////////

			// Color
			// alpha testing
			GL_Uniform4fv(offsetof(shaderProgram_t, glColor), color);
			GL_Uniform1fv(offsetof(shaderProgram_t, alphaTest), &regs[pStage->alphaTestRegister]);

			// bind the texture
			pStage->texture.image->Bind();

			// Setup the texture matrix if needed
			// NB: will be restored to identity
			if ( pStage->texture.hasMatrix ) {
				float matrix[16];
				RB_GetShaderTextureMatrix(surf->shaderRegisters, &pStage->texture, matrix);
				GL_UniformMatrix4fv(offsetof(shaderProgram_t, textureMatrix), matrix);
			}

			///////////
			// Draw it
			///////////
			RB_DrawElementsWithCounters(surf);

			////////////////////////////////////////////////////////////
			// Restore everything to an acceptable state for next stage
			////////////////////////////////////////////////////////////

			// Restore identity matrix
			if ( pStage->texture.hasMatrix ) {
				GL_UniformMatrix4fv(offsetof(shaderProgram_t, textureMatrix), mat4_identity.ToFloatPtr());
			}
		}

		if ( !didDraw ) {
			drawSolid = true;
		}

		///////////////////////////////////////////////////////////
		// Restore everything to an acceptable state for next step
		///////////////////////////////////////////////////////////

		// Restore color alpha to opaque
		color[3] = 1;
		GL_Uniform4fv(offsetof(shaderProgram_t, glColor), color);

		// Restore alphatest always passing
		static const float one[1] = { 1 };
		GL_Uniform1fv(offsetof(shaderProgram_t, alphaTest), one);

		// Restore white image binding to Tex0
		globalImages->whiteImage->Bind();
	}

	////////////////////////////////////////
	// Normal surfaces case (non perforated)
	////////////////////////////////////////

	// draw the entire surface solid
	if ( drawSolid ) {
		///////////
		// Draw it
		///////////
		RB_DrawElementsWithCounters(surf);
	}

	/////////////////////////////////////////////
	// Restore everything to an acceptable state
	/////////////////////////////////////////////

	// reset polygon offset
	if ( shader->TestMaterialFlag(MF_POLYGONOFFSET)) {
		qglDisable(GL_POLYGON_OFFSET_FILL);
	}

	// Restore blending
	if ( shader->GetSort() == SS_SUBVIEW ) {
		GL_State(GLS_DEPTHFUNC_LESS);
	}
}

/*
=====================
RB_GLSL_FillDepthBuffer

If we are rendering a subview with a near clip plane, use a second texture
to force the alpha test to fail when behind that clip plane
=====================
*/
void RB_GLSL_FillDepthBuffer(drawSurf_t** drawSurfs, int numDrawSurfs) {

	//////////////
	// Skip cases
	//////////////

	// if we are just doing 2D rendering, no need to fill the depth buffer
	if ( !backEnd.viewDef->viewEntitys ) {
		return;
	}

	////////////////////////////////////////
	// GL Shader setup for the current pass
	// (ie. common to each surface)
	////////////////////////////////////////

	// Expected client GL State at this point
	// Tex0 active
	// Vertex attribute enabled
	// Color attribute enabled
	// Shader AlphaTest is one
	// Shader Texture Matrix is Identity

	// If clip planes are enabled in the view, use he "Clip" version of zfill shader
	// and enable the second texture for mirror plane clipping if needed
	if ( backEnd.viewDef->numClipPlanes ) {
		// Use he zfillClip shader
		GL_UseProgram(&zfillClipShader);

		// Bind the Texture 1 to alphaNotchImage
		GL_SelectTexture(1);
		globalImages->alphaNotchImage->Bind();

		// Be sure to reactivate Texture 0, as it will be bound right after
		GL_SelectTexture(0);
	}
	// If no clip planes, just use the regular zfill shader
	else {
		GL_UseProgram(&zfillShader);
	}

	glBindBufferBase(
			GL_UNIFORM_BUFFER,
			backEnd.glState.currentProgram->viewMatricesBinding,
			viewMatricesBuffer);

	// Setup attributes arrays
	// Vertex attribute is always enabled
	// TexCoord attribute is always enabled
	// Disable Color attribute (as it is enabled by default)
	GL_DisableVertexAttribArray(ATTR_COLOR);

	// Texture 0 will be used for alpha tested surfaces. It should be already active.
	// Bind it to white image by default
	globalImages->whiteImage->Bind();

	// Decal surfaces may enable polygon offset
	// GAB Note: Looks like it is not needed, because in case of offsetted surface we will use the offset value of the surface
	// qglPolygonOffset(r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat());

	// Depth func to LESS
	GL_State(GLS_DEPTHFUNC_LESS);

	// Enable stencil test if we are going to be using it for shadows.
	// If we didn't do this, it would be legal behavior to get z fighting
	// from the ambient pass and the light passes.
	qglEnable(GL_STENCIL_TEST);
	qglStencilFunc(GL_ALWAYS, 1, 255);

	//////////////////////////
	// For each surfaces loop
	//////////////////////////

	// Optimization to only change MVP matrix when needed
	backEnd.currentSpace = NULL;

	for ( int i = 0; i < numDrawSurfs; i++ ) {

		const drawSurf_t* const drawSurf = drawSurfs[i];

		///////////////////////////////////////////
		// GL shader setup for the current surface
		///////////////////////////////////////////

		// Change the MVP matrix if needed
		if ( drawSurf->space != backEnd.currentSpace ) {
			GL_UniformMatrix4fv(offsetof(shaderProgram_t, modelMatrix), drawSurf->space->modelMatrix);

			glBindBufferBase(
					GL_UNIFORM_BUFFER,
					backEnd.glState.currentProgram->projectionMatrixBinding,
					projectionMatricesBuffer[RB_CalculateProjection(drawSurf)]);
		}

		// Hack Depth Range if necessary
		bool bNeedRestoreDepthRange = false;
		if (drawSurf->space->weaponDepthHack && drawSurf->space->modelDepthHack == 0.0f) {
			qglDepthRangef(0, 0.5);
			bNeedRestoreDepthRange = true;
		}

		// change the scissor if needed
		if ( r_useScissor.GetBool() && !backEnd.currentScissor.Equals(drawSurf->scissorRect)) {
			backEnd.currentScissor = drawSurf->scissorRect;
			if (( backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1 ) < 0.0f ||
			        ( backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1 ) < 0.0f ) {
				backEnd.currentScissor = backEnd.viewDef->scissor;
			}
			qglScissor(backEnd.viewDef->viewport.x1 + backEnd.currentScissor.x1,
			           backEnd.viewDef->viewport.y1 + backEnd.currentScissor.y1,
			           backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1,
			           backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1);
		}

		////////////////////
		// Do the real work
		////////////////////
		RB_T_GLSL_FillDepthBuffer(drawSurf);

		/////////////////////////////////////////////
		// Restore everything to an acceptable state
		/////////////////////////////////////////////

		if (bNeedRestoreDepthRange) {
			qglDepthRangef(0.0f, 1.0f);
		}

		// Let's change space for next iteration
		backEnd.currentSpace = drawSurf->space;
	}

	/////////////////////////////////////////////
	// Restore everything to an acceptable state
	/////////////////////////////////////////////
	// Restore current space to NULL
	backEnd.currentSpace = NULL;

	// Restore attributes arrays
	// Vertex attribute is always enabled
	// TexCoord attribute is always enabled
	// Re-enable Color attribute (as it is enabled by default)
	GL_EnableVertexAttribArray(ATTR_COLOR);
}

/*
==================
RB_GLSL_T_RenderShaderPasses

This is also called for the generated 2D rendering
==================
*/
void RB_GLSL_T_RenderShaderPasses(const drawSurf_t* surf, GLuint projection) {

	// global constants
	static const GLfloat zero[1] = { 0 };
	static const GLfloat one[1] = { 1 };
	static const GLfloat oneScaled[1] = { 1 / 255.0f };
	static const GLfloat negOneScaled[1] = { -1.0f / 255.0f };

	// usefull pointers
	const idMaterial* const shader = surf->material;
//	const srfTriangles_t* const tri = surf->geo;

	//////////////
	// Skip cases
	//////////////

#ifdef NO_LIGHT
	if ( !r_noLight.GetBool() )
#endif
	if ( !shader->HasAmbient()) {
		return;
	}

	if ( shader->IsPortalSky()) {
		return;
	}

	// some deforms may disable themselves by setting numIndexes = 0
	if ( !surf->numIndexes ) {
		return;
	}

	if ( !surf->ambientCache ) {
		return;
	}


	///////////////////////////////////
	// GL shader setup for the surface
	// (ie. common to each Stage)
	///////////////////////////////////

	// change the scissor if needed
	if ( r_useScissor.GetBool() &&
	    !backEnd.currentScissor.Equals(surf->scissorRect)) {
		backEnd.currentScissor = surf->scissorRect;
		if (( backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1 ) < 0.0f ||
		        ( backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1 ) < 0.0f ) {
			backEnd.currentScissor = backEnd.viewDef->scissor;
		}
		qglScissor(backEnd.viewDef->viewport.x1 + backEnd.currentScissor.x1,
		           backEnd.viewDef->viewport.y1 + backEnd.currentScissor.y1,
		           backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1,
		           backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1);

	}

	// set polygon offset if necessary
	// NB: must be restored at end of process
	if ( shader->TestMaterialFlag(MF_POLYGONOFFSET)) {
		qglEnable(GL_POLYGON_OFFSET_FILL);
		qglPolygonOffset(r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * shader->GetPolygonOffset());
	}

	// set face culling appropriately
	GL_Cull(shader->GetCullType());

	// Location of vertex attributes data
	const idDrawVert* const ac = (const idDrawVert* const) vertexCache.Position(surf->ambientCache);

	// get the expressions for conditionals / color / texcoords
	const float* const regs = surf->shaderRegisters;

	// Caches to set per surface shader GL state only when necessary
	bool bMVPSet[TG_GLASSWARP-TG_EXPLICIT];
	memset(bMVPSet, 0, (TG_GLASSWARP-TG_EXPLICIT)*sizeof(bool));
	bool bVASet [TG_GLASSWARP-TG_EXPLICIT];
	memset(bVASet, 0, (TG_GLASSWARP-TG_EXPLICIT)*sizeof(bool));

	// precompute the local view origin (might be needed for some texgens)
	idVec4 localViewOrigin;
	R_GlobalPointToLocal(surf->space->modelMatrix, backEnd.viewDef->renderView.vieworg, localViewOrigin.ToVec3());
	localViewOrigin.w = 1.0f;

	///////////////////////
	// For each stage loop
	///////////////////////
	for ( int stage = 0; stage < shader->GetNumStages(); stage++ ) {

		const shaderStage_t* const pStage = shader->GetStage(stage);

		///////////////
		// Skip cases
		///////////////

		// check the enable condition
		if ( regs[pStage->conditionRegister] == 0 ) {
			continue;
		}

		// skip the stages involved in lighting
		if ( pStage->lighting != SL_AMBIENT ) {
			continue;
		}

		// skip if the stage is ( GL_ZERO, GL_ONE ), which is used for some alpha masks
		if (( pStage->drawStateBits & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS )) ==
		        ( GLS_SRCBLEND_ZERO | GLS_DSTBLEND_ONE )) {
			continue;
		}

		// see if we are a new-style stage
		const newShaderStage_t* const newStage = pStage->newStage;

		if ( newStage ) {
			// new style stages: Not implemented in GLSL yet!
			continue;
		} else {

			// old style stages

			/////////////////////////
			// Additional skip cases
			/////////////////////////

			// precompute the color
			const float color[4] = {
				regs[pStage->color.registers[0]],
				regs[pStage->color.registers[1]],
				regs[pStage->color.registers[2]],
				regs[pStage->color.registers[3]]
			};

			// skip the entire stage if an add would be black
			if (
			    ( pStage->drawStateBits & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS )) == ( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE )
			    && color[0] <= 0 && color[1] <= 0 && color[2] <= 0 ) {
				continue;
			}

			// skip the entire stage if a blend would be completely transparent
			if (( pStage->drawStateBits & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS )) ==
			        ( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA )
			        && color[3] <= 0 ) {
				continue;
			}

			/////////////////////////////////
			// GL shader setup for the stage
			/////////////////////////////////
			// The very first thing we need to do before going down into GL is to choose he correct GLSL shader depending on
			// the associated TexGen. Then, setup its specific uniforms/attribs, and then only we can setup the common uniforms/attribs

			if ( pStage->texture.texgen == TG_DIFFUSE_CUBE ) {
				// Not used in game, but implemented because trivial

				// This is diffuse cube mapping
				GL_UseProgram(&diffuseCubeShader);

				glBindBufferBase(
						GL_UNIFORM_BUFFER,
						backEnd.glState.currentProgram->viewMatricesBinding,
						viewMatricesBuffer);

				// Possible that normals should be transformed by a normal matrix in the shader ? I am not sure...

				// Setup texcoord array to use the normals
				GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_TexCoord), 3, GL_FLOAT, false, sizeof(idDrawVert),
				                       ac->normal.ToFloatPtr());

				// Setup the texture matrix
				if ( pStage->texture.hasMatrix ) {
					float matrix[16];
					RB_GetShaderTextureMatrix(surf->shaderRegisters, &pStage->texture, matrix);
					GL_UniformMatrix4fv(offsetof(shaderProgram_t, textureMatrix), matrix);
				}
			} else if ( pStage->texture.texgen == TG_SKYBOX_CUBE ) {
				// This is skybox cube mapping
				GL_UseProgram(&skyboxCubeShader);

				glBindBufferBase(
						GL_UNIFORM_BUFFER,
						backEnd.glState.currentProgram->viewMatricesBinding,
						viewMatricesBuffer);

				// Disable TexCoord attribute
				GL_DisableVertexAttribArray(ATTR_TEXCOORD);

				// Setup the local view origin uniform
				GL_Uniform4fv(offsetof(shaderProgram_t, localViewOrigin), localViewOrigin.ToFloatPtr());

				// Setup the texture matrix
				if ( pStage->texture.hasMatrix ) {
					float matrix[16];
					RB_GetShaderTextureMatrix(surf->shaderRegisters, &pStage->texture, matrix);
					GL_UniformMatrix4fv(offsetof(shaderProgram_t, textureMatrix), matrix);
				}
			} else if ( pStage->texture.texgen == TG_WOBBLESKY_CUBE ) {
				// This is skybox cube mapping, with special texture matrix
				GL_UseProgram(&skyboxCubeShader);

				glBindBufferBase(
						GL_UNIFORM_BUFFER,
						backEnd.glState.currentProgram->viewMatricesBinding,
						viewMatricesBuffer);

				// Disable TexCoord attribute
				GL_DisableVertexAttribArray(ATTR_TEXCOORD);

				// Setup the local view origin uniform
				GL_Uniform4fv(offsetof(shaderProgram_t, localViewOrigin), localViewOrigin.ToFloatPtr());

				// Setup the texture matrix
				// Note: here, we combine the shader texturematrix and precomputed wobblesky matrix
				if ( pStage->texture.hasMatrix ) {
					float texturematrix[16];
					RB_GetShaderTextureMatrix(surf->shaderRegisters, &pStage->texture, texturematrix);
					float finalmatrix[16];
					myGlMultMatrix(texturematrix, surf->wobbleTransform, finalmatrix);
					GL_UniformMatrix4fv(offsetof(shaderProgram_t, textureMatrix), finalmatrix);
				} else {
					GL_UniformMatrix4fv(offsetof(shaderProgram_t, textureMatrix), surf->wobbleTransform);
				}
			} else if ( pStage->texture.texgen == TG_SCREEN ) {
				// Not used in game, so not implemented
				continue;
			} else if ( pStage->texture.texgen == TG_SCREEN2 ) {
				// Not used in game, so not implemented
				continue;
			} else if ( pStage->texture.texgen == TG_GLASSWARP ) {
				// Not used in game, so not implemented. The shader code is even not present in original D3 data
				continue;
			} else if ( pStage->texture.texgen == TG_REFLECT_CUBE ) {
				// This is reflection cubemapping
				GL_UseProgram(&reflectionCubeShader);

				glBindBufferBase(
						GL_UNIFORM_BUFFER,
						backEnd.glState.currentProgram->viewMatricesBinding,
						viewMatricesBuffer);

				// NB: in original D3, if the surface had a bump map it would lead to the "Bumpy reflection cubemaping" shader being used.
				// This is not implemented for now, we only do standard reflection cubemaping. Visual difference is really minor.

				// Setup texcoord array to use the normals
				GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_TexCoord), 3, GL_FLOAT, false, sizeof(idDrawVert),
				                       ac->normal.ToFloatPtr());

				// Setup the viewMatrix, we will need it to compute the reflection
				GL_UniformMatrix4fv(offsetof(shaderProgram_t, modelViewMatrix), surf->space->eyeViewMatrix[2]);

				// Setup the texture matrix like original D3 code does: using the transpose viewMatrix of the view
				// NB: this is curious, not sure why this is done like this....
				float mat[16];
				R_TransposeGLMatrix(backEnd.viewDef->worldSpace.eyeViewMatrix[2], mat);
				GL_UniformMatrix4fv(offsetof(shaderProgram_t, textureMatrix), mat);
			} else { // TG_EXPLICIT
				// Otherwise, this is just regular surface shader with explicit texcoords
				GL_UseProgram(&diffuseMapShader);

				glBindBufferBase(
						GL_UNIFORM_BUFFER,
						backEnd.glState.currentProgram->viewMatricesBinding,
						viewMatricesBuffer);

				// Setup the TexCoord pointer
				GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_TexCoord), 2, GL_FLOAT, false, sizeof(idDrawVert),
				                       ac->st.ToFloatPtr());

				// Setup the texture matrix
				if ( pStage->texture.hasMatrix ) {
					float matrix[16];
					RB_GetShaderTextureMatrix(surf->shaderRegisters, &pStage->texture, matrix);
					GL_UniformMatrix4fv(offsetof(shaderProgram_t, textureMatrix), matrix);
				}
			}

			// Now we have a shader, we can setup the uniforms and attribute pointers common to all kind of shaders
			// The specifics have already been done in the shader selection code (see above)

			// Non-stage dependent state (per drawsurf, may be done once per GL shader)
			{
				// Vertex Attributes
				if ( !bVASet[pStage->texture.texgen] ) {

					// Setup the Vertex Attrib pointer
					GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Vertex), 3, GL_FLOAT, false, sizeof(idDrawVert),
					                       ac->xyz.ToFloatPtr());

					// Setup the Color pointer
					GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Color), 4, GL_UNSIGNED_BYTE, false, sizeof(idDrawVert),
					                       (void*) &ac->color);

					bVASet[pStage->texture.texgen] = true;
				}

				// MVP
				if ( !bMVPSet[pStage->texture.texgen] ) {
					GL_UniformMatrix4fv(offsetof(shaderProgram_t, modelMatrix), surf->space->modelMatrix);

					glBindBufferBase(
							GL_UNIFORM_BUFFER,
							backEnd.glState.currentProgram->projectionMatrixBinding,
                            projectionMatricesBuffer[projection]);

					bMVPSet[pStage->texture.texgen] = true;
				}
			}

			// Stage dependent state

			// Setup the Color uniform
			GL_Uniform4fv(offsetof(shaderProgram_t, glColor), color);

			// Setup the Color modulation
			switch ( pStage->vertexColor ) {
			case SVC_MODULATE: {
				GL_Uniform1fv(offsetof(shaderProgram_t, colorModulate), oneScaled);
				GL_Uniform1fv(offsetof(shaderProgram_t, colorAdd), zero);
				break;
			}
			case SVC_INVERSE_MODULATE: {
				GL_Uniform1fv(offsetof(shaderProgram_t, colorModulate), negOneScaled);
				GL_Uniform1fv(offsetof(shaderProgram_t, colorAdd), one);
				break;
			}
			default:
			case SVC_IGNORE:
				// This is already the default values (zero, one)
				break;
			}

			// bind the texture (this will be either a dynamic texture, or a static one)
			RB_BindVariableStageImage(&pStage->texture, regs);

			// set the state
			GL_State(pStage->drawStateBits);

			// set privatePolygonOffset if necessary
			if ( pStage->privatePolygonOffset ) {
				qglEnable(GL_POLYGON_OFFSET_FILL);
				qglPolygonOffset(r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * pStage->privatePolygonOffset);
			}

#ifdef NO_LIGHT
			if (r_noLight.GetBool() )
			{
				if (pStage->drawStateBits!=9000)
	                GL_State(pStage->drawStateBits);
	            else
	            {
	                if (shader->TestMaterialFlag(MF_POLYGONOFFSET))
	                    GL_State(GLS_SRCBLEND_ONE|GLS_DSTBLEND_ONE|GLS_DEPTHFUNC_LESS);
	                else
	                    GL_State(GLS_SRCBLEND_ONE|GLS_DSTBLEND_ONE|GLS_DEPTHFUNC_LESS);
	            }
            }
#endif
			/////////////////////
			// Draw the surface!
			/////////////////////
			RB_DrawElementsWithCounters(surf);

			/////////////////////////////////////////////
			// Restore everything to an acceptable state
			/////////////////////////////////////////////

			// Disable the other attributes array
			if ( pStage->texture.texgen == TG_DIFFUSE_CUBE ) {
				// Restore identity to the texture matrix
				if ( pStage->texture.hasMatrix) {
					GL_UniformMatrix4fv(offsetof(shaderProgram_t, textureMatrix), mat4_identity.ToFloatPtr());
				}
			} else if ( pStage->texture.texgen == TG_SKYBOX_CUBE ) {
				// Reenable TexCoord attribute
				GL_EnableVertexAttribArray(ATTR_TEXCOORD);

				// Restore identity to the texture matrix
				if ( pStage->texture.hasMatrix) {
					GL_UniformMatrix4fv(offsetof(shaderProgram_t, textureMatrix), mat4_identity.ToFloatPtr());
				}
			} else if ( pStage->texture.texgen == TG_WOBBLESKY_CUBE ) {
				// Reenable TexCoord attribute
				GL_EnableVertexAttribArray(ATTR_TEXCOORD);

				// Restore identity to the texture matrix (shall be done each time, as there is the wobblesky transform combined inside)
				GL_UniformMatrix4fv(offsetof(shaderProgram_t, textureMatrix), mat4_identity.ToFloatPtr());
			} else if ( pStage->texture.texgen == TG_SCREEN ) {
			} else if ( pStage->texture.texgen == TG_SCREEN2 ) {
			} else if ( pStage->texture.texgen == TG_GLASSWARP ) {
			} else if ( pStage->texture.texgen == TG_REFLECT_CUBE ) {
				// Restore identity to the texture matrix (shall be done each time)
				GL_UniformMatrix4fv(offsetof(shaderProgram_t, textureMatrix), mat4_identity.ToFloatPtr());
			} else {
				// Restore identity to the texture matrix
				if ( pStage->texture.hasMatrix) {
					GL_UniformMatrix4fv(offsetof(shaderProgram_t, textureMatrix), mat4_identity.ToFloatPtr());
				}
			}

			// unset privatePolygonOffset if necessary
			if ( pStage->privatePolygonOffset && !surf->material->TestMaterialFlag(MF_POLYGONOFFSET)) {
				qglDisable(GL_POLYGON_OFFSET_FILL);
			} else if ( pStage->privatePolygonOffset && surf->material->TestMaterialFlag(MF_POLYGONOFFSET)) {
				qglPolygonOffset(r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * shader->GetPolygonOffset());
			}

			// Restore color modulation state to default values
			if ( pStage->vertexColor != SVC_IGNORE ) {
				GL_Uniform1fv(offsetof(shaderProgram_t, colorModulate), zero);
				GL_Uniform1fv(offsetof(shaderProgram_t, colorAdd), one);
			}

			// Don't touch the rest, as this will either reset by the next stage, or handled by end of this method
		}
	}

	/////////////////////////////////////////////
	// Restore everything to an acceptable state
	/////////////////////////////////////////////

	// reset polygon offset
	if ( shader->TestMaterialFlag(MF_POLYGONOFFSET)) {
		qglDisable(GL_POLYGON_OFFSET_FILL);
	}
}

/*
=====================
RB_GLSL_DrawShaderPasses

Draw non-light dependent passes
=====================
*/
int RB_GLSL_DrawShaderPasses(drawSurf_t** drawSurfs, int numDrawSurfs) {

	//////////////
	// Skip cases
	//////////////

	// only obey skipAmbient if we are rendering a view
	if ( backEnd.viewDef->viewEntitys && r_skipAmbient.GetBool()) {
		return numDrawSurfs;
	}

	// if we are about to draw the first surface that needs
	// the rendering in a texture, copy it over
	if ( drawSurfs[0]->material->GetSort() >= SS_POST_PROCESS ) {
		if ( r_skipPostProcess.GetBool()) {
			return 0;
		}

		// only dump if in a 3d view
		if ( backEnd.viewDef->viewEntitys ) {
			//globalImages->currentRenderImage->CopyFramebuffer(backEnd.viewDef->viewport.x1,
			//                                                  backEnd.viewDef->viewport.y1,
			//                                                  backEnd.viewDef->viewport.x2 - backEnd.viewDef->viewport.x1 + 1,
			//                                                  backEnd.viewDef->viewport.y2 - backEnd.viewDef->viewport.y1 + 1,
			//                                                  true);
		}

		backEnd.currentRenderCopied = true;
	}

	////////////////////////////////////////
	// GL shader setup for the current pass
	// (ie. common to each surface)
	////////////////////////////////////////

	// Texture 0 is expected to be active

	// Setup attributes arrays
	// Vertex attribute is always enabled
	// Color attribute is always enabled
	// Texcoord attribute is always enabled

	/////////////////////////
	// For each surface loop
	/////////////////////////

	GLuint projection = -1;
	backEnd.currentSpace = NULL;

	int i;
	for ( i = 0; i < numDrawSurfs; i++ ) {

		//////////////
		// Skip cases
		//////////////

		if ( drawSurfs[i]->material->SuppressInSubview()) {
			continue;
		}

		if ( backEnd.viewDef->isXraySubview && drawSurfs[i]->space->entityDef ) {
			if ( drawSurfs[i]->space->entityDef->parms.xrayIndex != 2 ) {
				continue;
			}
		}

		// we need to draw the post process shaders after we have drawn the fog lights
		if ( drawSurfs[i]->material->GetSort() >= SS_POST_PROCESS
		        && !backEnd.currentRenderCopied ) {
			break;
		}


		// Change the MVP matrix if needed
		if ( drawSurfs[i]->space != backEnd.currentSpace ) {
			projection = RB_CalculateProjection(drawSurfs[i]);
			// We can't set the uniform now, as we still don't know which shader to use
		}

		// Hack Depth Range if necessary
		bool bNeedRestoreDepthRange = false;
		if (drawSurfs[i]->space->weaponDepthHack && drawSurfs[i]->space->modelDepthHack == 0.0f) {
			qglDepthRangef(0.0f, 0.5f);
			bNeedRestoreDepthRange = true;
		}

		////////////////////
		// Do the real work
		////////////////////
		RB_GLSL_T_RenderShaderPasses(drawSurfs[i], projection);

		if (bNeedRestoreDepthRange) {
			qglDepthRangef(0.0f, 1.0f);
		}

		backEnd.currentSpace = drawSurfs[i]->space;
	}

	/////////////////////////////////////////////
	// Restore everything to an acceptable state
	/////////////////////////////////////////////

	backEnd.currentSpace = NULL;

	// Restore culling
	GL_Cull(CT_FRONT_SIDED);

	// Restore attributes arrays
	// Vertex attribute is always enabled
	// Color attribute is always enabled
	// Texcoord attribute is always enabled

	// Trashed state:
	//   Current Program
	//   Tex0 binding

	// Return the counter of drawn surfaces
	return i;
}

/*
=====================
RB_T_BlendLight

=====================
*/
static void RB_T_GLSL_BlendLight(const drawSurf_t *surf, const viewLight_t* vLight) {
//	const srfTriangles_t *tri = surf->geo;

	////////////
	// GL setup
	////////////

	// Shader uniforms

	// Setup the fogMatrix as being the local Light Projection
	// Only do this once per space
	if (backEnd.currentSpace != surf->space) {
		idPlane lightProject[4];

		int i;
		for (i = 0; i < 4; i++) {
			R_GlobalPlaneToLocal(surf->space->modelMatrix, vLight->lightProject[i], lightProject[i]);
		}

		idMat4 fogMatrix;
		fogMatrix[0] = lightProject[0].ToVec4();
		fogMatrix[1] = lightProject[1].ToVec4();
		fogMatrix[2] = lightProject[2].ToVec4();
		fogMatrix[3] = lightProject[3].ToVec4();
		GL_UniformMatrix4fv(offsetof(shaderProgram_t, fogMatrix), fogMatrix.ToFloatPtr());
	}

	// Attributes pointers

	// This gets used for both blend lights and shadow draws
	if (surf->ambientCache) {
		idDrawVert *ac = (idDrawVert *) vertexCache.Position(surf->ambientCache);
		GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Vertex), 3, GL_FLOAT, false, sizeof(idDrawVert), ac->xyz.ToFloatPtr());
	} else if (surf->shadowCache) {
		shadowCache_t *sc = (shadowCache_t *) vertexCache.Position(surf->shadowCache);
		GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Vertex), 3, GL_FLOAT, false, sizeof(idDrawVert), sc->xyz.ToFloatPtr());
	}

	////////////////////
	// Draw the surface
	////////////////////
	RB_DrawElementsWithCounters(surf);
}

/*
=====================
RB_GLSL BlendLight

Dual texture together the falloff and projection texture with a blend
mode to the framebuffer, instead of interacting with the surface texture
=====================
*/
void RB_GLSL_BlendLight(const drawSurf_t *drawSurfs, const drawSurf_t *drawSurfs2, const viewLight_t* vLight) {
	const idMaterial * const lightShader = vLight->lightShader;
	const float * const regs = vLight->shaderRegisters;

	//////////////
	// Skip Cases
	//////////////

	if (!drawSurfs) {
		return;
	}

	if (r_skipBlendLights.GetBool()) {
		return;
	}

	////////////////////////////////////
	// GL setup for the current pass
	// (ie. common to all Light Stages)
	////////////////////////////////////

	// Use blendLight shader
	GL_UseProgram(&blendLightShader);

	glBindBufferBase(
			GL_UNIFORM_BUFFER,
			backEnd.glState.currentProgram->viewMatricesBinding,
			viewMatricesBuffer);

	// Texture 1 will get the falloff texture
	GL_SelectTexture(1);
	vLight->falloffImage->Bind();

	// Texture 0 will get the projected texture
	GL_SelectTexture(0);

	////////////////////////
	// For each Light Stage
	////////////////////////

	int i;
	for (i = 0; i < lightShader->GetNumStages(); i++) {
		const shaderStage_t *stage = lightShader->GetStage(i);

		//////////////
		// Skip Cases
		//////////////

		if (!regs[stage->conditionRegister]) {
			continue;
		}

		////////////////////////////////////////
		// GL setup for the current Light Stage
		// (ie. common to all surfaces)
		////////////////////////////////////////

		// Global GL state

		// Setup the drawState
		GL_State(GLS_DEPTHMASK | stage->drawStateBits | GLS_DEPTHFUNC_EQUAL);

		// Bind the projected texture
		stage->texture.image->Bind();

		// Shader Uniforms

		// Setup the texture matrix
		if ( stage->texture.hasMatrix ) {
			float matrix[16];
			RB_GetShaderTextureMatrix(regs, &stage->texture, matrix);
			GL_UniformMatrix4fv(offsetof(shaderProgram_t, textureMatrix), matrix);
		}

		// Setup the Fog Color
		float lightColor[4];
		lightColor[0] = regs[stage->color.registers[0]];
		lightColor[1] = regs[stage->color.registers[1]];
		lightColor[2] = regs[stage->color.registers[2]];
		lightColor[3] = regs[stage->color.registers[3]];
		GL_Uniform4fv(offsetof(shaderProgram_t, fogColor), lightColor);

		////////////////////
		// Do the Real Work
		////////////////////

		RB_GLSL_RenderDrawSurfChainWithFunction(drawSurfs, RB_T_GLSL_BlendLight, vLight);
		RB_GLSL_RenderDrawSurfChainWithFunction(drawSurfs2, RB_T_GLSL_BlendLight, vLight);

		////////////////////
		// GL state restore
		////////////////////

		// Restore texture matrix to identity
		if (stage->texture.hasMatrix) {
			GL_UniformMatrix4fv(offsetof(shaderProgram_t, textureMatrix), mat4_identity.ToFloatPtr());
		}
	}
}

/*
==================
RB_FogAllLights
==================
*/
void RB_GLSL_FogAllLights(void) {

	//////////////
	// Skip Cases
	//////////////

	//GB Never do this on Doom3Quest
	return;

	if (r_skipFogLights.GetBool() || backEnd.viewDef->isXraySubview /* dont fog in xray mode*/ ) {
		return;
	}

	/////////////////////////////////////////////
	// GL setup for the current pass
	// (ie. common to both fog and blend lights)
	/////////////////////////////////////////////

	// Disable Stencil Test
	qglDisable(GL_STENCIL_TEST);

	// Disable TexCoord array
	// Disable Color array
	GL_DisableVertexAttribArray(ATTR_TEXCOORD);
	GL_DisableVertexAttribArray(ATTR_COLOR);

	//////////////////
	// For each Light
	//////////////////

	const viewLight_t *vLight;
	for (vLight = backEnd.viewDef->viewLights; vLight; vLight = vLight->next) {

		//////////////
		// Skip Cases
		//////////////

		// We are only interested in Fog and Blend lights
		if (!vLight->lightShader->IsFogLight() && !vLight->lightShader->IsBlendLight()) {
			continue;
		}

		///////////////////////
		// Do the Light passes
		///////////////////////

		if (vLight->lightShader->IsFogLight()) {
			RB_GLSL_FogPass(vLight->globalInteractions, vLight->localInteractions, vLight);
		} else if (vLight->lightShader->IsBlendLight()) {
			RB_GLSL_BlendLight(vLight->globalInteractions, vLight->localInteractions, vLight);
		}
	}

	////////////////////
	// GL state restore
	////////////////////

	// Re-enable TexCoord array
	// Re-enable Color array
	GL_EnableVertexAttribArray(ATTR_TEXCOORD);
	GL_EnableVertexAttribArray(ATTR_COLOR);

	// Re-enable Stencil Test
	qglEnable(GL_STENCIL_TEST);
}


/*
=================
RB_BeginGLSLShaderPasses
=================
*/
void RB_GLSL_PrepareShaders(void) {

	// No shaders set by default
	GL_UseProgram(NULL);

	//Set up the buffers that won't change this frame
	GL_ViewMatricesUniformBuffer(backEnd.viewDef->worldSpace.viewMatrix);

	static bool first = true;
	static float defaultProjection[16];
	if (first) {
        memset(defaultProjection, 0, 16 * sizeof(float));
        first = false;
    }

    //We only need to do the following if the default projection changes
	if (memcmp(defaultProjection, backEnd.viewDef->projectionMatrix, 16 * sizeof(float)) != 0)
    {
	    //Take a copy of the default projection
        memcpy(defaultProjection, backEnd.viewDef->projectionMatrix, 16 * sizeof(float));

        float orthoProjectionMatrix[16];
        memset(orthoProjectionMatrix, 0, sizeof(orthoProjectionMatrix));
        orthoProjectionMatrix[0] = 2.0f / 640.0f;
        orthoProjectionMatrix[5] = -2.0f / 480.0f;
        orthoProjectionMatrix[10] = -2.0f / 1.0f;
        orthoProjectionMatrix[12] = -1.0f;
        orthoProjectionMatrix[13] = 1.0f;
        orthoProjectionMatrix[14] = -1.0f;
        orthoProjectionMatrix[15] = 1.0f;

        //0 is ortho projection matrix
        GL_ProjectionMatricesUniformBuffer(projectionMatricesBuffer[ORTHO_PROJECTION],
                                           orthoProjectionMatrix);

        //1 is unadjusted projection matrix
        GL_ProjectionMatricesUniformBuffer(projectionMatricesBuffer[NORMAL_PROJECTION],
                                           backEnd.viewDef->projectionMatrix);

        //2 is weapon depth hack projection
        float projection[16];
        RB_ComputeProjection(true, 0.0, projection);
        GL_ProjectionMatricesUniformBuffer(projectionMatricesBuffer[WEAPON_PROJECTION], projection);

        //3+ ore model depth hack projections
        for (int i = 0; i <= NUM_DEPTH_HACK_PROJECTIONS; ++i) {
            float depthHack = (float)(i+1) / float(NUM_DEPTH_HACK_PROJECTIONS+1);
            RB_ComputeProjection(false, depthHack, projection);
            GL_ProjectionMatricesUniformBuffer(projectionMatricesBuffer[DEPTH_HACK_PROJECTION + i],
                                               projection);
        }

        projectionMatricesSet = true;
    }

	// Always enable the vertex, color and texcoord attributes arrays
	GL_EnableVertexAttribArray(ATTR_VERTEX);
	GL_EnableVertexAttribArray(ATTR_COLOR);
	GL_EnableVertexAttribArray(ATTR_TEXCOORD);
	// Disable the other arrays
	GL_DisableVertexAttribArray(ATTR_NORMAL);
	GL_DisableVertexAttribArray(ATTR_TANGENT);
	GL_DisableVertexAttribArray(ATTR_BITANGENT);
}
