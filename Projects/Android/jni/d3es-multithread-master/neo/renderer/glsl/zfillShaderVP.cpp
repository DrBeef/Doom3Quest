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

const char * const zfillShaderVP = R"(
#version 300 es

// Multiview
#define NUM_VIEWS 2
#extension GL_OVR_multiview2 : enable
layout(num_views=NUM_VIEWS) in;

precision mediump float;

// In
in highp vec4 attr_Vertex;
in vec4 attr_TexCoord;
        
// Uniforms
uniform ShaderMatrices
{
    uniform highp mat4 modelViewProjectionMatrix[NUM_VIEWS];
} u_shaderMatrices;
uniform mat4 u_textureMatrix;
        
// Out
// gl_Position
out vec2 var_TexDiffuse;
        
void main()
{
  var_TexDiffuse = (u_textureMatrix * attr_TexCoord).xy;  // Homogeneous coordinates of textureMatrix supposed to be 1

  gl_Position = u_shaderMatrices.modelViewProjectionMatrix[gl_ViewID_OVR] * attr_Vertex;
}
)";
