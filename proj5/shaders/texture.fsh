#version 410

uniform sampler2D tMap;
uniform sampler2D normMap;
uniform sampler2D detailMap;
uniform vec3 direction;
uniform vec3 ambient;
uniform vec3 intensity;
uniform bool noLight;
uniform bool noFog;
uniform vec3 fogColor;
uniform bool rain;

in vec2 f_tCoord;
in float f_fog;
in float f_camDist;

out vec3 fragColor;

void main(){

    vec3 norm = normalize(texture(normMap, f_tCoord).xyz);
    norm = normalize((norm * 2) - vec3(1.0, 1.0, 1.0)).xzy;

    if(noLight){
      fragColor = vec3(texture(tMap,f_tCoord));
    }
    else{
      fragColor = (ambient + (max(0, dot(normalize(direction), norm)) * intensity)) *
                   vec3(texture(tMap,f_tCoord));

    }

    fragColor = ((1 - f_fog) * fogColor) + (f_fog * fragColor);

    if(rain)
      fragColor = fragColor * 0.5;

    float alpha = max(0, (50000.0 - f_camDist)/50000.0);


    fragColor = alpha * fragColor * vec3(texture(detailMap, f_tCoord)) +
                (1 - alpha) * fragColor;

}
