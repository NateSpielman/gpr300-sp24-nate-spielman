#version 450
//Vertex attributes
layout(location = 0) in vec3 vPos; 
layout(location = 1) in vec3 vNormal; 
layout(location = 2) in vec2 vTexCoord; 

uniform mat4 _Model;
uniform mat4 _ViewProjection; 

//This whole block will be passed to the next shader stage.
out Surface{
	vec3 WorldPos; //Vertex position in world space
	vec3 WorldNormal; //Vertex normal in world space
	vec2 TexCoord;
}vs_out;

uniform mat4 _LightViewProj; //view + projection of light source camera
out vec4 LightSpacePos; //Sent to fragment shader

void main(){
	//Transform vertex position to World Space.
	vs_out.WorldPos = vec3(_Model * vec4(vPos,1.0));
	//Transform vertex normal to world space using Normal Matrix
	vs_out.WorldNormal = transpose(inverse(mat3(_Model))) * vNormal;
	vs_out.TexCoord = vTexCoord;
	//Set vertex position in homogeneous clip space
	gl_Position = _ViewProjection * _Model * vec4(vPos,1.0);

	LightSpacePos = _LightViewProj * _Model * vec4(vPos, 1.0);
}
