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
uniform mat4 _LightViewProj; //view + projection of light source camera
uniform float _MinBias;
uniform float _MaxBias;
uniform sampler2D _ShadowMap;

float calculateShadow(sampler2D shadowMap, vec4 lightSpacePos, float bias){
	//Homogeneous Clip space to NDC [-w,w] to [-1,1]
    vec3 sampleCoord = lightSpacePos.xyz / lightSpacePos.w;
    //Convert from [-1,1] to [0,1]
    sampleCoord = sampleCoord * 0.5 + 0.5;
	//Include bias in depth
	float myDepth = sampleCoord.z - bias; 
	float shadowMapDepth = texture(shadowMap, sampleCoord.xy).r;

	//PCF 
	float totalShadow = 0.0;
	vec2 texelOffset = 1.0 /  textureSize(_ShadowMap,0);
	for(int y = -1; y <=1; y++){
		for(int x = -1; x <=1; x++){
			vec2 uv = sampleCoord.xy + vec2(x * texelOffset.x, y * texelOffset.y);
			totalShadow += step(texture(_ShadowMap,uv).r,myDepth);
		}
	}
	totalShadow /= 9.0;

	//shadow outside of far plane of frustum stays at 0.0
	if(sampleCoord.z > 1.0)
        totalShadow = 0.0;

	return totalShadow;
}

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

	//Light space position
	vec4 LightSpacePos;
	LightSpacePos = _LightViewProj * vec4(worldPos, 1.0);

	//shadow
	float bias = max(_MaxBias * (1.0 - dot(normal,toLight)),_MinBias);
	float shadow = calculateShadow(_ShadowMap, LightSpacePos, bias);
	lightColor *= 1.0 - shadow;

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
