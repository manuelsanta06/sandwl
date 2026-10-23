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

struct SimplisticPointLight{
  vec3 position;

  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
};
uniform SimplisticPointLight light;

struct DirectionalLight{
  vec3 direction;

  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
};
uniform DirectionalLight directionalLight;

struct PointLight{
  vec3 position;
  float constant;
  float linear;
  float quadratic;

  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
};
uniform PointLight pointLight;

struct SpotLight{
  vec3 position;
  vec3 direction;
  float cutOff; //cosine of the actual angle(in radians)
  float outerCutOff;

  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
};
uniform SpotLight spotLight;


uniform vec3 lightColor;
uniform vec3 viewPos;

vec4 basicLighting(){
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

  return vec4(ambient+diffuse+specular,1.0);
}
vec4 directionalLighting(){
  //ambient
  vec3 ambient=directionalLight.ambient*vec3(texture(material.diffuse,texCoord));
  // diffuse
  vec3 norm=normalize(normal);
  vec3 lightDir=normalize(-directionalLight.direction);
  float diff=max(dot(norm,lightDir),0.0);
  vec3 diffuse=directionalLight.diffuse*diff*vec3(texture(material.diffuse,texCoord));
  // specular
  vec3 viewDir=normalize(viewPos-fragPos);
  vec3 reflectDir=reflect(-lightDir,norm);
  float spec=pow(max(dot(viewDir,reflectDir),0.0),material.shininess);
  vec3 specular=directionalLight.specular*spec*vec3(texture(material.specular,texCoord));

  return vec4(ambient+diffuse+specular,1.0);
}
vec4 pointLighting(){
  //ambient
  vec3 ambient=pointLight.ambient*vec3(texture(material.diffuse,texCoord));
  // diffuse
  vec3 norm=normalize(normal);
  vec3 lightDir=normalize(pointLight.position-fragPos);
  float diff=max(dot(norm,lightDir),0.0);
  vec3 diffuse=pointLight.diffuse*diff*vec3(texture(material.diffuse,texCoord));
  // specular
  vec3 viewDir=normalize(viewPos-fragPos);
  vec3 reflectDir=reflect(-lightDir,norm);
  float spec=pow(max(dot(viewDir,reflectDir),0.0),material.shininess);
  vec3 specular=pointLight.specular*spec*vec3(texture(material.specular,texCoord));
  // atenuation
  float distance=length(pointLight.position-fragPos);
  float attenuation=1.0/(pointLight.constant+pointLight.linear*distance+pointLight.quadratic*(distance*distance));
  ambient *=attenuation; 
  diffuse *=attenuation;
  specular*=attenuation;

  return vec4(ambient+diffuse+specular,1.0);
}
vec4 spotLighting(){
  vec3 ambient=spotLight.ambient*vec3(texture(material.diffuse,texCoord));

  vec3 norm=normalize(normal);
  vec3 lightDir=normalize(spotLight.position-fragPos);
  
  // Diffuse
  float diff=max(dot(norm,lightDir),0.0);
  vec3 diffuse=spotLight.diffuse*diff*vec3(texture(material.diffuse,texCoord));
  
  // Specular
  vec3 viewDir=normalize(viewPos-fragPos);
  vec3 reflectDir=reflect(-lightDir,norm);
  float spec=pow(max(dot(viewDir,reflectDir),0.0),material.shininess);
  vec3 specular=spotLight.specular*spec*vec3(texture(material.specular,texCoord));

  // Spotlight
  float theta=dot(lightDir,normalize(-spotLight.direction));
  float epsilon=spotLight.cutOff-spotLight.outerCutOff;
  float intensity=clamp((theta-spotLight.outerCutOff)/epsilon,0.0,1.0);
  
  diffuse *=intensity;
  specular*=intensity;
  
  return vec4(ambient+diffuse+specular,1.0);
}

void main(){
  // FragColor=basicLighting();
  // FragColor=directionalLighting();
  // FragColor=pointLighting();
  FragColor=spotLighting();
}
