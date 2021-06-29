#version 150

in vec3 position;
in vec3 normal;
in vec2 texcoord;

uniform mat4 mWorld, mView, mProjection;

out vec3 norm_out;
out vec2 tex_coords;

out vec3 norm_Model;
//out vec3 norm_Camera;

out vec4 outPosition;
out vec3 pos_View;

void main() {    
  mat4 mvp = mProjection * mView * mWorld;
  gl_Position = mvp * vec4(position, 1.0);

  vec3 pos_World = (mWorld * vec4(position, 1.0)).xyz;
  vec3 pos_Camera = (mView * mWorld * vec4(position, 1.0)).xyz;
  vec3 eyeDir_Camera = vec3(0,0,0) - pos_Camera;

  norm_Model = (mWorld * vec4(normal, 0.0)).xyz;
  vec3 normal_Camera = (mView * mWorld * vec4(normal, 0.0)).xyz;

  outPosition = vec4(pos_World, 1.0);
  
  //norm_Camera = normal_Camera;
  tex_coords = texcoord;
}
