#version 320 es
precision mediump float;
in vec3 fragPos;
in vec3 normal;
out vec4 FragColor;
  
uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPos;

void main(){
  float ambientStrength=0.1;
  float specularStrength=0.5;
  vec3 ambient=ambientStrength*lightColor;

  vec3 norm=normalize(normal);
  vec3 lightDir=normalize(lightPos-fragPos);
  float diff=max(dot(norm,lightDir),0.0);
  vec3 diffuse=diff*lightColor;

  vec3 viewDir=normalize(viewPos-fragPos);
  vec3 reflectDir=reflect(-lightDir,norm);
  float spec=pow(max(dot(viewDir,reflectDir),0.0),32.0);
  vec3 specular=specularStrength*spec*lightColor;

  vec3 result=(ambient+diffuse+specular)*objectColor;
  FragColor=vec4(result,1.0);
}
