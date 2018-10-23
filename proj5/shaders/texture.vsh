#version 410

layout (location = 0) in vec4 position;

uniform mat4 viewMat;
uniform mat4 projMat;

uniform vec4 scaling;
uniform vec3 Opos;
uniform int tileWidth;
uniform float fogDensity;
uniform bool noFog;
uniform int tCol;
uniform int tRow;

out vec2 f_tCoord;
out float f_fog;
out float f_camDist;

void main(){

    vec3 CamRelativeWorldSpace = vec3(position.x * scaling.x,
                                      position.y * scaling.y + position.w * scaling.w,
                                      position.z * scaling.z);

    CamRelativeWorldSpace = CamRelativeWorldSpace + Opos;

    f_camDist = length(CamRelativeWorldSpace);

    gl_Position = projMat * viewMat * vec4(CamRelativeWorldSpace, 1.0);

    f_tCoord = vec2((position.x-tCol)/tileWidth,
                    (position.z-tRow)/tileWidth);

    if(!noFog){
      float d = length(viewMat * vec4(CamRelativeWorldSpace, 1.0));
      f_fog = exp2(-1.442695 * pow(fogDensity * d, 2));
    }
    else
      f_fog = 1;

}
