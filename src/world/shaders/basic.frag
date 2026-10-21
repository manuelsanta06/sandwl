#version 320 es
precision mediump float;
in vec3 fragPos;
in vec3 normal;
in vec2 texCoord;
out vec4 FragColor;

struct Material{
  sampler2D diffuse;
  sampler2D specular;
  float shininess;
};
uniform Material material;

struct Light{
  vec3 position;

  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
};
uniform Light light;

uniform vec3 lightColor;
uniform vec3 viewPos;

void main(){
  //ambient
  vec3 ambient=light.ambient*vec3(texture(material.diffuse,texCoord));
  
  // diffuse
  vec3 norm=normalize(normal);
  vec3 lightDir=normalize(light.position-fragPos);
  float diff=max(dot(norm,lightDir),0.0);
  vec3 diffuse=light.diffuse*diff*vec3(texture(material.diffuse,texCoord));
  
  // specular
  vec3 viewDir=normalize(viewPos-fragPos);
  vec3 reflectDir=reflect(-lightDir,norm);
  float spec=pow(max(dot(viewDir,reflectDir),0.0),material.shininess);
  vec3 specular=light.specular*spec*vec3(texture(material.specular,texCoord));

  FragColor=vec4(ambient+diffuse+specular,1.0);
}
