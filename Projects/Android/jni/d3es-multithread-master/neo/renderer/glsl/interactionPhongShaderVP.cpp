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

#include "glsl_shaders.h"

const char * const interactionPhongShaderVP = R"(
#version 300 es

// Multiview
#define NUM_VIEWS 2
#extension GL_OVR_multiview2 : enable
layout(num_views=NUM_VIEWS) in;

precision highp float;
  
// In
in highp vec4 attr_Vertex;
in lowp vec4 attr_Color;
in vec4 attr_TexCoord;
in vec3 attr_Tangent;
in vec3 attr_Bitangent;
in vec3 attr_Normal;
  
// Uniforms
layout(shared) uniform ViewMatrices
{
    uniform highp mat4 u_viewMatrices[NUM_VIEWS];
};
layout(shared) uniform ProjectionMatrix
{
    uniform highp mat4 u_projectionMatrix;
};
uniform highp mat4 u_modelMatrix;
uniform mat4 u_lightProjection;
uniform lowp float u_colorModulate;
uniform lowp float u_colorAdd;
uniform vec4 u_lightOrigin;
uniform vec4 u_viewOrigin;
uniform vec4 u_bumpMatrixS;
uniform vec4 u_bumpMatrixT;
uniform vec4 u_diffuseMatrixS;
uniform vec4 u_diffuseMatrixT;
uniform vec4 u_specularMatrixS;
uniform vec4 u_specularMatrixT;
  
// Out
// gl_Position
out vec2 var_TexDiffuse;
out vec2 var_TexNormal;
out vec2 var_TexSpecular;
out vec4 var_TexLight;
out lowp vec4 var_Color;
out vec3 var_L;
out vec3 var_V;
  
void main()
{
  mat3 M = mat3(attr_Tangent, attr_Bitangent, attr_Normal);
  
  var_TexNormal.x = dot(u_bumpMatrixS, attr_TexCoord);
  var_TexNormal.y = dot(u_bumpMatrixT, attr_TexCoord);
  
  var_TexDiffuse.x = dot(u_diffuseMatrixS, attr_TexCoord);
  var_TexDiffuse.y = dot(u_diffuseMatrixT, attr_TexCoord);
  
  var_TexSpecular.x = dot(u_specularMatrixS, attr_TexCoord);
  var_TexSpecular.y = dot(u_specularMatrixT, attr_TexCoord);
  
  var_TexLight.x = dot(u_lightProjection[0], attr_Vertex);
  var_TexLight.y = dot(u_lightProjection[1], attr_Vertex);
  var_TexLight.z = dot(u_lightProjection[2], attr_Vertex);
  var_TexLight.w = dot(u_lightProjection[3], attr_Vertex);
  
  vec3 L = u_lightOrigin.xyz - attr_Vertex.xyz;
  vec3 V = u_viewOrigin.xyz - attr_Vertex.xyz;
  
  var_L = L * M;
  var_V = V * M;

  if (u_colorModulate == 0.0) {
    var_Color = vec4(u_colorAdd);
  } else {
    var_Color = (attr_Color * u_colorModulate) + vec4(u_colorAdd);
  }
  
  gl_Position = u_projectionMatrix * (u_viewMatrices[gl_ViewID_OVR] * (u_modelMatrix * attr_Vertex));
}
)";
