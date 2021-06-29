//  -*- mode: glsl-mode;-*-

struct LightInfo {
    vec3 position;
    vec3 color;
    float strength;
};

// Get the color resulted by the light refleting into the object
vec3 get_light_color(vec3 diffusecolor, vec3 lightColor, float lightPower,
                     vec3 lightDirection) {

    //Cosine of angle between normal and light direction
    vec3 n = normalize(norm_Model);

    vec3 l = normalize(lightDirection);
    float cosTheta = clamp(dot(n, l), 0, 1);

    return (diffusecolor + cosTheta * lightPower * lightColor);
}

// Get the color resulted by the main directional light refleting
// into the object
vec3 get_directional_light_color(vec3 diffusecolor, vec3 lightColor, float lightPower,
                                 vec3 lightDirection) {

    //Cosine of angle between normal and light direction
    vec3 n = normalize(norm_Model);

    vec3 l = normalize(lightDirection);
    float cosTheta = clamp(dot(n, l), 0, 1);

    return (diffusecolor * cosTheta * lightPower * lightColor);
}

vec3 get_point_light_color(LightInfo[4] lights, int lightCount, vec4 vertexPos) {
    vec3 finalColor = vec3(0, 0, 0);

    for (int i = 0; i < lightCount; i++) {
        vec3 ldirection = lights[i].position - vertexPos.xyz;
        
        float attenConstant = 1;
	    float attenLinear = 0.5;
        float attenExp = 0.25;
        float dist = length(ldirection);

        float lightPower =  (attenConstant + attenLinear * dist +
                             attenExp * dist * dist);

        vec3 lightColor = get_light_color(finalColor, lights[i].color,
                                          lights[i].strength, ldirection);

	    finalColor += (lightColor / lightPower);
    }

    return finalColor;
}
