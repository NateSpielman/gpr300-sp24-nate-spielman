#version 450
out vec4 FragColor;
in vec2 UV;

uniform sampler2D _ColorBuffer;
uniform float rOffset;
uniform float gOffset;
uniform float bOffset;
uniform int effectOn;

void main(){
    vec3 color = texture(_ColorBuffer, UV).rgb;

    float r = texture(_ColorBuffer, UV + vec2(rOffset, 0)).x;
    float g = texture(_ColorBuffer, UV + vec2(gOffset, 0)).y;
    float b = texture(_ColorBuffer, UV + vec2(bOffset, 0)).z;

    if(effectOn != 1) {
   	 FragColor = vec4(color, 1.0);
   	 return;
    }

    FragColor = vec4(r, g, b, 1.0);
}
