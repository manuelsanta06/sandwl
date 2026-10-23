#version 320 es
precision mediump float;
in vec3 fragPos;
in vec3 normal;
in vec2 texCoord;
out vec4 FragColor;

uniform vec3 lightColor;
uniform vec3 viewPos;

struct Material{
  sampler2D diffuse;
  sampler2D specular;
  float shininess;
};
uniform Material material;

struct DirectionalLight{
  vec3 direction;

  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
};
uniform DirectionalLight dirLight;

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


vec3 dirLighting(DirectionalLight light,vec3 normal,vec3 viewDir);
vec3 pointLighting(PointLight light,vec3 normal,vec3 fragPos,vec3 viewDir);
vec3 spotLighting(SpotLight light,vec3 normal,vec3 fragPos,vec3 viewPos);

void main(){
  vec3 norm=normalize(normal);
  vec3 viewDir=normalize(viewPos-fragPos);

  vec3 colorAcumulator=vec3(0.0,0.0,0.0);
  colorAcumulator+=dirLighting(dirLight,normal,viewDir);
  colorAcumulator+=pointLighting(pointLight,norm,fragPos,viewDir);
  colorAcumulator+=spotLighting(spotLight,norm,fragPos,viewDir);
  FragColor=vec4(colorAcumulator,0.0);
}

vec3 dirLighting(DirectionalLight light,vec3 normal,vec3 viewDir){
  vec3 lightDir=normalize(-light.direction);
  // diffuse shading
  float diff=max(dot(normal,lightDir),0.0);
  // specular shading
  vec3 reflectDir=reflect(-lightDir,normal);
  float spec=pow(max(dot(viewDir,reflectDir),0.0),material.shininess);
  
  vec3 ambient  =light.ambient  *     vec3(texture(material.diffuse,texCoord));
  vec3 diffuse  =light.diffuse  *diff*vec3(texture(material.diffuse,texCoord));
  vec3 specular =light.specular *spec*vec3(texture(material.specular,texCoord));
  return ambient+diffuse+specular;
}

vec3 pointLighting(PointLight light,vec3 normal,vec3 fragPos,vec3 viewDir){
  vec3 lightDir=normalize(light.position-fragPos);
  //diffuse shading
  float diff=max(dot(normal,lightDir),0.0);
  // specular shading
  vec3 reflectDir=reflect(-lightDir,normal);
  float spec=pow(max(dot(viewDir,reflectDir),0.0),material.shininess);
  //attenuation
  float distance=length(light.position-fragPos);
  float attenuation=1.0/(light.constant+light.linear*distance+light.quadratic*(distance*distance));
  
  vec3 ambient  =light.ambient  *     vec3(texture(material.diffuse,texCoord));
  vec3 diffuse  =light.diffuse  *diff*vec3(texture(material.diffuse,texCoord));
  vec3 specular =light.specular *spec*vec3(texture(material.specular,texCoord));
  ambient  *= attenuation;
  diffuse  *= attenuation;
  specular *= attenuation;
  return ambient+diffuse+specular;
}

vec3 spotLighting(SpotLight light,vec3 normal,vec3 fragPos,vec3 viewPos){
  vec3 lightDir=normalize(spotLight.position-fragPos);
  
  float diff=max(dot(normal,lightDir),0.0);
  
  vec3 viewDir=normalize(viewPos-fragPos);
  vec3 reflectDir=reflect(-lightDir,normal);
  float spec=pow(max(dot(viewDir,reflectDir),0.0),material.shininess);

  //spotlight
  float theta=dot(lightDir,normalize(-spotLight.direction));
  float epsilon=spotLight.cutOff-spotLight.outerCutOff;
  float intensity=clamp((theta-spotLight.outerCutOff)/epsilon,0.0,1.0);
  
  vec3 ambient  =spotLight.ambient*      vec3(texture(material.diffuse,texCoord));
  vec3 diffuse  =spotLight.diffuse*diff *vec3(texture(material.diffuse,texCoord));
  vec3 specular =spotLight.specular*spec*vec3(texture(material.specular,texCoord));
  diffuse *=intensity;
  specular*=intensity;
  return ambient+diffuse+specular;
}
