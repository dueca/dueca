// https://open.gl/drawing

#version 150 core

in vec2 position;
uniform mat4 projection;

void main()
{
    gl_Position = projection * vec4(position, 0.0, 1.0);
}