// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec3 inCol;

layout (location = 0) out vec3 outColor;

void main()
{
	outColor = 0.5 * inCol + 0.5 * vec3(1.0, 1.0, 1.0);
	gl_Position = vec4(inPos, 0.0, 1.0);
}
