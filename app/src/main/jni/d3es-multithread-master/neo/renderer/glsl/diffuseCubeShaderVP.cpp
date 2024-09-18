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

const char * const diffuseCubeShaderVP = R"(
#version 300 es

// Multiview
#define NUM_VIEWS 2
#extension GL_OVR_multiview2 : enable
layout(num_views=NUM_VIEWS) in;

precision mediump float;
  
// In
in highp vec4 attr_Vertex;
in lowp vec4 attr_Color;
in vec3 attr_TexCoord;
  
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
uniform mat4 u_textureMatrix;
uniform lowp float u_colorAdd;
uniform lowp float u_colorModulate;
  
// Out
// gl_Position
out vec3 var_TexCoord;
out lowp vec4 var_Color;
  
void main()
{
  var_TexCoord = (u_textureMatrix * vec4(attr_TexCoord, 0.0)).xyz;

  if (u_colorModulate == 0.0) {
    var_Color = vec4(u_colorAdd);
  } else {
    var_Color = (attr_Color * u_colorModulate) + vec4(u_colorAdd);
  }

  gl_Position = u_projectionMatrix * (u_viewMatrices[gl_ViewID_OVR] * (u_modelMatrix * attr_Vertex));
}
)";
