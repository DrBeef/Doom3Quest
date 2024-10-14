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

const char * const interactionShaderFP = R"(
#version 300 es
precision highp float;

// In
in vec2 var_TexDiffuse;
in vec2 var_TexNormal;
in vec2 var_TexSpecular;
in vec4 var_TexLight;
in vec3 var_Normal;
in lowp vec4 var_Color;
in vec3 var_L;
in vec3 var_V;
in vec3 var_H;

// Uniforms
uniform lowp vec4 u_diffuseColor;
uniform lowp vec4 u_specularColor;
uniform float u_specularExponent;
uniform sampler2D u_fragmentMap0; // u_bumpTexture
uniform sampler2D u_fragmentMap1; // u_lightFalloffTexture
uniform sampler2D u_fragmentMap2; // u_lightProjectionTexture
uniform sampler2D u_fragmentMap3; // u_diffuseTexture
uniform sampler2D u_fragmentMap4; // u_specularTexture

// Out
layout(location = 0) out vec4 fragColor;

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
    float PI = 3.14159265359;
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;
    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return num / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

void main()
{
  vec3 L = normalize(var_L);
  vec3 V = normalize(var_V);
  vec3 H = normalize(var_H);
  vec3 N = normalize(2.0 * texture(u_fragmentMap0, var_TexNormal.st).agb - 1.0);

  float NdotL = clamp(dot(N, L), 0.0, 1.0);
  float NdotH = clamp(dot(N, H), 0.0, 1.0);

  vec3 lightProjection = textureProj(u_fragmentMap2, var_TexLight.xyw).rgb;
  vec3 lightFalloff = texture(u_fragmentMap1, vec2(var_TexLight.z, 0.5)).rgb;
  vec3 diffuseColor = texture(u_fragmentMap3, var_TexDiffuse).rgb * u_diffuseColor.rgb;

  //PBR
  vec3 AN = normalize(mix(normalize(var_Normal), N, 1.0));
  vec4 Cd = vec4(diffuseColor.rgb, 1.0);
  vec4 specTex = texture(u_fragmentMap4, var_TexSpecular);
  vec4 roughness = vec4(specTex.r, specTex.r, specTex.r, specTex.r);
  vec4 metallic = vec4(specTex.g, specTex.g, specTex.g, specTex.g);

  vec4 Cl = vec4(lightProjection * lightFalloff, 1.0);
  vec3 F0 = vec3(0.04);
  F0 = mix(F0, Cd.xyz, metallic.xyz);

  // cook-torrance brdf
  float NDF = DistributionGGX(AN, H, roughness.x);
  float G   = GeometrySmith(AN, V, L, roughness.x);
  vec3 F    = fresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0); //max

  vec3 kS = F;
  vec3 kD = vec3(1.0) - kS;
  kD *= 1.0 - metallic.r;

  vec3 numerator    = NDF * G * F;
  float denominator = 4.0 * max(dot(AN, V), 0.0) * max(dot(AN, L), 0.0);
  vec3 pbr     = numerator / max(denominator, 0.1);

  vec3 color;
  color = diffuseColor;
  //Lubos BEGIN
  float smoothing = 0.33;
  float scale = u_specularExponent + u_specularExponent;
  color.r += u_specularColor.r * pow(pbr.r, smoothing) * scale;
  color.g += u_specularColor.g * pow(pbr.g, smoothing) * scale;
  color.b += u_specularColor.b * pow(pbr.b, smoothing) * scale;
  //Lubos END
  color *= NdotL * lightProjection;
  color *= lightFalloff;

  fragColor = vec4(color, 1.0) * var_Color;
}
)";
