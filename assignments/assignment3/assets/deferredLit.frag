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

uniform vec3 _EyePos;

vec3 calculateLighting(vec3 worldNormal, vec3 worldPos, vec3 albedo) {
    vec3 normal = normalize(worldNormal);
	vec3 toLight = -_Light.LightDirection;
	float diffuseFactor = max(dot(normal,toLight),0.0);
	vec3 toEye = normalize(_EyePos - worldPos);
	//Blinn-phong uses half angle
	vec3 h = normalize(toLight + toEye);
	float specularFactor = pow(max(dot(normal,h),0.0),_Material.Shininess);
	//Combination of specular and diffuse reflection
	vec3 lightColor = (_Material.Kd * diffuseFactor + _Material.Ks * specularFactor) * _Light.LightColor;
	lightColor+=_Light.AmbientColor * _Material.Ka;

	return lightColor;
}

void main(){
	//Sample surface properties for this screen pixel
	vec3 normal = texture(_gNormals,UV).xyz;
	vec3 worldPos = texture(_gPositions,UV).xyz;
	vec3 albedo = texture(_gAlbedo,UV).xyz;

	//Worldspace lighting calculations, same as in forward shading
	vec3 lightColor = calculateLighting(normal,worldPos,albedo);
	FragColor = vec4(albedo * lightColor,1.0);
}
