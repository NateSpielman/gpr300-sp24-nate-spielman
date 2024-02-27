//deferredLit.frag
#version 450 core
out vec4 FragColor; 
in vec2 UV; //From fsTriangle.vert

//All your material and lighting uniforms go here!
struct Light{
	vec3 LightDirection;
	vec3 LightColor;
	vec3 AmbientColor;
};
uniform Light _Light;

struct Material{
	float Ka; //Ambient coefficient (0-1)
	float Kd; //Diffuse coefficient (0-1)
	float Ks; //Specular coefficient (0-1)
	float Shininess; //Affects size of specular highlight
};
uniform Material _Material;

//layout(binding = i) can be used as an alternative to shader.setInt()
//Each sampler will always be bound to a specific texture unit
uniform layout(binding = 0) sampler2D _gPositions;
uniform layout(binding = 1) sampler2D _gNormals;
uniform layout(binding = 2) sampler2D _gAlbedo;

void main(){
	//Sample surface properties for this screen pixel
	vec3 normal = texture(_gNormals,UV).xyz;
	vec3 worldPos = texture(_gPositions,UV).xyz;
	vec3 albedo = texture(_gAlbedo,UV).xyz;

	//Worldspace lighting calculations, same as in forward shading
	vec3 lightColor = calculateLighting(normal,worldPos,albedo);
	FragColor = vec4(albedo * lightColor,1.0);
}
