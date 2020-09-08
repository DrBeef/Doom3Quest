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

const char * const zfillShaderFP = R"(
#version 100
precision mediump float;
        
// In
varying vec2 var_TexDiffuse;
        
// Uniforms
uniform sampler2D u_fragmentMap0;
uniform lowp float u_alphaTest;
uniform lowp vec4 u_glColor;
        
// Out
// gl_FragCoord
        
void main(void)
{
    if (u_alphaTest > texture2D(u_fragmentMap0, var_TexDiffuse).a) {
      discard;
    }
        
  gl_FragColor = u_glColor;
}
)";
