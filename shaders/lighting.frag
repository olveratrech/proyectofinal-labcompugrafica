#version 330 core

#define NUMBER_OF_POINT_LIGHTS 4

struct Material
{
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
    vec3 diffuseColor;
    vec3 specularColor;
    float opacity;
};

struct DirLight
{
    vec3 direction;
    
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight
{
    vec3 position;
    
    float constant;
    float linear;
    float quadratic;
    
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct SpotLight
{
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;
    
    float constant;
    float linear;
    float quadratic;
    
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

out vec4 color;

uniform vec3 viewPos;
uniform DirLight dirLight;
uniform PointLight pointLights[NUMBER_OF_POINT_LIGHTS];
uniform SpotLight spotLight;
uniform Material material;
uniform int transparency;
uniform float objectAlpha;
uniform bool hasDiffuseMap;
uniform bool hasSpecularMap;
uniform vec3 objectTint;

vec4 surfaceColor()
{
    vec4 texel = hasDiffuseMap ? texture(material.diffuse, TexCoords) : vec4(1.0);
    return texel * vec4(material.diffuseColor * objectTint, 1.0);
}

vec3 specularColor()
{
    return material.specularColor *
        (hasSpecularMap ? texture(material.specular, TexCoords).rgb : vec3(1.0));
}

// Function prototypes
vec3 CalcDirLight( DirLight light, vec3 normal, vec3 viewDir );
vec3 CalcPointLight( PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir );
vec3 CalcSpotLight( SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir );

void main( )
{
    // Properties
    vec3 norm = normalize( Normal );
    if (!gl_FrontFacing) norm = -norm;
    vec3 viewDir = normalize( viewPos - FragPos );
    
    // Directional lighting
    vec3 result = CalcDirLight( dirLight, norm, viewDir );
    
    // Point lights
    for ( int i = 0; i < NUMBER_OF_POINT_LIGHTS; i++ )
    {
        result += CalcPointLight( pointLights[i], norm, FragPos, viewDir );
    }
    
    // Spot light
    result += CalcSpotLight( spotLight, norm, FragPos, viewDir );
 	
    float alpha = clamp(objectAlpha * material.opacity * surfaceColor().a, 0.0, 1.0);
    if (transparency == 1 && alpha < 0.1) discard;
    color = vec4(result, alpha);

}

// Calculates the color when using a directional light.
vec3 CalcDirLight( DirLight light, vec3 normal, vec3 viewDir )
{
    vec3 lightDir = normalize( -light.direction );
    
    // Diffuse shading
    float diff = max( dot( normal, lightDir ), 0.0 );
    
    // Specular shading
    vec3 reflectDir = reflect( -lightDir, normal );
    float spec = diff > 0.0 ? pow(max(dot(viewDir, reflectDir), 0.0), material.shininess) : 0.0;
    
    // Combine results
    vec3 ambient = light.ambient * surfaceColor().rgb;
    vec3 diffuse = light.diffuse * diff * surfaceColor().rgb;
    vec3 specular = light.specular * spec * specularColor();
    
    return ( ambient + diffuse + specular );
}

// Calculates the color when using a point light.
vec3 CalcPointLight( PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir )
{
    vec3 lightDir = normalize( light.position - fragPos );
    
    // Diffuse shading
    float diff = max( dot( normal, lightDir ), 0.0 );
    
    // Specular shading
    vec3 reflectDir = reflect( -lightDir, normal );
    float spec = diff > 0.0 ? pow(max(dot(viewDir, reflectDir), 0.0), material.shininess) : 0.0;
    
    // Attenuation
    float distance = length( light.position - fragPos );
    float attenuation = 1.0f / ( light.constant + light.linear * distance + light.quadratic * ( distance * distance ) );
    
    // Combine results
    vec3 ambient = light.ambient * surfaceColor().rgb;
    vec3 diffuse = light.diffuse * diff * surfaceColor().rgb;
    vec3 specular = light.specular * spec * specularColor();
    
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    
    return ( ambient + diffuse + specular );
}

// Calculates the color when using a spot light.
vec3 CalcSpotLight( SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir )
{
    vec3 lightDir = normalize( light.position - fragPos );
    
    // Diffuse shading
    float diff = max( dot( normal, lightDir ), 0.0 );
    
    // Specular shading
    vec3 reflectDir = reflect( -lightDir, normal );
    float spec = diff > 0.0 ? pow(max(dot(viewDir, reflectDir), 0.0), material.shininess) : 0.0;
    
    // Attenuation
    float distance = length( light.position - fragPos );
    float attenuation = 1.0f / ( light.constant + light.linear * distance + light.quadratic * ( distance * distance ) );
    
    // Spotlight intensity
    float theta = dot( lightDir, normalize( -light.direction ) );
    // Centro iluminado y transicion suave hacia el borde circular del cono.
    float intensity = smoothstep(light.outerCutOff, light.cutOff, theta);
    
    // Combine results
    vec3 ambient = light.ambient * surfaceColor().rgb;
    vec3 diffuse = light.diffuse * diff * surfaceColor().rgb;
    vec3 specular = light.specular * spec * specularColor();
    
    ambient *= attenuation * intensity;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;
    
    return ( ambient + diffuse + specular );
}
