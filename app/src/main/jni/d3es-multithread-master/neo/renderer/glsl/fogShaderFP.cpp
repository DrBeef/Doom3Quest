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

const char * const fogShaderFP = R"(
#version 300 es
precision mediump float;
  
// In
in vec2 var_TexFog;            // input Fog TexCoord
in vec2 var_TexFogEnter;       // input FogEnter TexCoord
  
// Uniforms
uniform sampler2D u_fragmentMap0;   // Fog Image
uniform sampler2D u_fragmentMap1;   // Fog Enter Image
uniform lowp vec4 u_fogColor;       // Fog Color
  
// Out
layout(location = 0) out vec4 fragColor;
  
void main()
{
  fragColor = texture( u_fragmentMap0, var_TexFog ) * texture( u_fragmentMap1, var_TexFogEnter ) * vec4(u_fogColor.rgb, 1.0);
}
)";
