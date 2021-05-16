///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
#ifdef TEXTURED_GEOMETRY

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location = 0) in vec3 aPosition;
layout(location = 2) in vec2 aTexCoord;

out vec2 vTexCoord;

void main()
{
	vTexCoord = aTexCoord;
	gl_Position = vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec2 vTexCoord;

uniform sampler2D uTexture;

layout(location = 0) out vec4 oColor;

void main()
{
	oColor = texture(uTexture, vTexCoord);
}

#endif
#endif

#ifdef SHOW_TEXTURED_MESH

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
// layout(location = 3) in vec3 aTangent;
// layout(location = 4) in vec3 aBitangent;

struct Light
{
	unsigned int type;
	vec3 color;
	vec3 direction;
	float intensity;
	vec3 position;
	int radius;
};

layout(binding = 0, std140) uniform GlobalParams
{
	vec3 uCameraPosition;
	unsigned int uLightCount;
	Light uLight[16];
};

layout(binding = 1, std140) uniform LocalParams
{
	mat4 uWorldMatrix;
	mat4 uWorldViewProjectionMatrix;
};

out vec2 vTexCoord;
out vec3 vPosition; // In worldspace
out vec3 vNormal; // In worldspace
out vec3 vViewDir; // In worldspace

void main()
{
	vTexCoord = aTexCoord;
	vPosition = vec3(uWorldMatrix * vec4(aPosition, 1.0));
	vNormal = vec3(transpose(inverse(uWorldMatrix)) * vec4(aNormal, 1.0));
	vViewDir = uCameraPosition - vPosition;

	gl_Position = uWorldViewProjectionMatrix * vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec2 vTexCoord;
in vec3 vPosition;
in vec3 vNormal;
in vec3 vViewDir;

struct Light
{
	unsigned int type;
	vec3 color;
	vec3 direction;
	float intensity;
	vec3 position;
	int radius;
};

layout(binding = 0, std140) uniform GlobalParams
{
	vec3 uCameraPosition;
	unsigned int uLightCount;
	Light uLight[16];
};

uniform sampler2D uTexture;

layout(location = 0) out vec4 oFinalRender;
layout(location = 1) out vec4 oNormals;
layout(location = 2) out vec4 oDiffuse;

out float gl_FragDepth;

vec3 CalculateDirectionalLight(Light light)
{
	vec3 N = normalize(vNormal);
    vec3 L = normalize(light.direction);

	// Hardcoded specular parameter
    vec3 specularMat = vec3(0.5);

	// Diffuse
    float diffuseIntensity = max(0.0, dot(N,L));

	// Specular
    float specularIntensity = pow(max(0.0, dot(N, L)), 1.0);
    vec3 specular = light.color * specularMat * specularIntensity;


	return (diffuseIntensity + specular) * light.intensity;
}

vec3 CalculatePointLight(Light light)
{
	vec3 N = normalize(vNormal);
	vec3 L = normalize(light.position - vPosition);

	float threshold = 1.0;

	float shadowIntensity = 1.0;

	float dist = distance(light.position, vPosition);

	if(dist > light.radius)
		shadowIntensity = 1.0 - ((dist - light.radius) / threshold);


	// Hardcoded specular parameter
    vec3 specularMat = vec3(1.0);

	// brightness
	float brightness = dot(L, vNormal);

	// Specular
    float specularIntensity = pow(max(0.0, dot(N, L)), 1.0);
    vec3 specular = light.color * specularMat * specularIntensity;

	// Diffuse
    float diffuseIntensity = max(0.0, dot(N,L));

	return vec3(brightness) * (specular + diffuseIntensity) * shadowIntensity * light.intensity;
}

void main()
{
	vec4 objectColor = texture(uTexture, vTexCoord);
	vec4 spec = vec4(0.0);

	vec3 lightFactor = vec3(0.0);
	for(int i = 0; i < uLightCount; ++i)
	{
		switch(uLight[i].type)
		{
			case 0: // Directional
			{
				lightFactor += CalculateDirectionalLight(uLight[i]);
			}
			break;

			case 1: // Point
			{
				lightFactor += CalculatePointLight(uLight[i]);
			}
			break;

			default:
			{
				break;
			}
		}
	}


	oFinalRender = vec4(lightFactor, 1.0) * objectColor;
	oNormals = vec4(normalize(vNormal), 1.0);
	oDiffuse = objectColor;

	gl_FragDepth = gl_FragCoord.z-0.2;
}


#endif
#endif

#ifdef DEFERRED_GEOMETRY_PASS

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
// layout(location = 3) in vec3 aTangent;
// layout(location = 4) in vec3 aBitangent;

layout(binding = 1, std140) uniform LocalParams
{
	mat4 uWorldMatrix;
	mat4 uWorldViewProjectionMatrix;
};

out vec2 vTexCoord;
out vec3 vPosition; // In worldspace
out vec3 vNormal; // In worldspace

void main()
{
	vTexCoord = aTexCoord;
	vPosition = vec3(uWorldMatrix * vec4(aPosition, 1.0));
	vNormal = vec3(transpose(inverse(uWorldMatrix)) * vec4(aNormal, 1.0));

	gl_Position = uWorldViewProjectionMatrix * vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec2 vTexCoord;
in vec3 vPosition;
in vec3 vNormal;

uniform sampler2D uTexture;

layout(location = 0) out vec4 oPosition;
layout(location = 1) out vec4 oNormals;
layout(location = 2) out vec4 oDiffuse;

out float gl_FragDepth;

void main()
{
	vec3 objectColor = texture(uTexture, vTexCoord).rgb;

	oPosition = vec4(vPosition, 1.0);
	oNormals = vec4(normalize(vNormal), 1.0);
	oDiffuse = vec4(objectColor, 1.0);

	gl_FragDepth = gl_FragCoord.z - 0.2;
}


#endif
#endif

#ifdef DEFERRED_LIGHTING_PASS_POINT

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;

struct Light
{
	unsigned int type;
	vec3 color;
	vec3 direction;
	float intensity;
	vec3 position;
	float radius;
};

layout(binding = 0, std140) uniform GlobalParams
{
	vec3 uCameraPosition;
	unsigned int uLightCount;
	Light uLight[16];
};

out vec2 vTexCoord;

void main()
{
	vTexCoord = aTexCoord;

	gl_Position = vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec2 vTexCoord;

struct Light
{
	unsigned int type;
	vec3 color;
	vec3 direction;
	float intensity;
	vec3 position;
	float radius;
};

layout(binding = 0, std140) uniform GlobalParams
{
	vec3 uCameraPosition;
	unsigned int uLightCount;
	Light uLight[16];
};

uniform sampler2D uGPosition;
uniform sampler2D uGNormals;
uniform sampler2D uGDiffuse;

layout(location = 0) out vec4 oFinalRender;

vec3 CalculatePointLight(Light light, vec3 FragPos, vec3 Normal, vec3 Diffuse)
{
	vec3 N = normalize(Normal);
	vec3 L = normalize(light.position - FragPos);

	float threshold = 1.0;

	float shadowIntensity = 1.0;

	float dist = distance(light.position, FragPos);

	if(dist > light.radius)
		shadowIntensity = 1.0 - ((dist - light.radius) / threshold);

	// Hardcoded specular parameter
    vec3 specularMat = vec3(1.0);

	// brightness
	float brightness = dot(L, Normal);

	// Specular
    float specularIntensity = pow(max(0.0, dot(N, L)), 1.0);
    vec3 specular = light.color * specularMat * specularIntensity;

	// Diffuse
    float diffuseIntensity = max(0.0, dot(N, L));

	return vec3(brightness) * (specular + diffuseIntensity) * shadowIntensity * light.intensity;
}

void main()
{
	vec3 FragPos = texture(uGPosition, vTexCoord).rgb;
    vec3 Normal = texture(uGNormals, vTexCoord).rgb;
    vec3 Diffuse = texture(uGDiffuse, vTexCoord).rgb;

	//vec3 lighting = Diffuse * 0.1;
	vec3 lighting = vec3(0.0);
	/*vec3 viewPos = uCameraPosition;
    vec3 viewDir  = normalize(viewPos - FragPos);*/

    for(int i = 0; i < uLightCount; ++i)
    {
		switch(uLight[i].type)
		{
			case 0: // Directional
			{
				float cosAngle = max(dot(Normal, -uLight[i].direction), 0.0); 
                vec3 ambient = 0.25 * uLight[i].color;
                vec3 diffuse = 0.75 * uLight[i].color * cosAngle;

                lighting += (ambient + diffuse) * Diffuse;
			}
			break;

			case 1: // Point
			{
				// diffuse
				/*vec3 lightDir = normalize(uLight[i].position - FragPos);
				vec3 diffuse = max(dot(Normal, lightDir), 0.0) * Diffuse * uLight[i].color;
				// specular
				vec3 halfwayDir = normalize(lightDir + viewDir);  
				float spec = pow(max(dot(Normal, halfwayDir), 0.0), 16.0);
				vec3 specular = uLight[i].color * spec;
				// attenuation
				float distance = length(uLight[i].position - FragPos);
				float attenuation = 1.0 / (1.0 + 0.1 * distance + 0.01 * distance * distance);
				diffuse *= attenuation;
				specular *= attenuation;
				lighting += (diffuse + specular) * uLight[i].intensity;*/

				lighting += CalculatePointLight(uLight[i], FragPos, Normal, Diffuse);
			}
			break;

			default:
			{
				
			}
			break;
		}
    }

	oFinalRender = vec4(lighting * Diffuse, 1.0);
}


#endif
#endif

#ifdef DEFERRED_LIGHTING_PASS_DIRECTIONAL

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;

struct Light
{
	unsigned int type;
	vec3 color;
	vec3 direction;
	float intensity;
	vec3 position;
	float radius;
};

layout(binding = 0, std140) uniform GlobalParams
{
	vec3 uCameraPosition;
	unsigned int uLightCount;
	Light uLight[16];
};

out vec2 vTexCoord;

void main()
{
	vTexCoord = aTexCoord;

	gl_Position = vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec2 vTexCoord;

struct Light
{
	unsigned int type;
	vec3 color;
	vec3 direction;
	float intensity;
	vec3 position;
	float radius;
};

layout(binding = 0, std140) uniform GlobalParams
{
	vec3 uCameraPosition;
	unsigned int uLightCount;
	Light uLight[16];
};

uniform sampler2D uGPosition;
uniform sampler2D uGNormals;
uniform sampler2D uGDiffuse;

layout(location = 0) out vec4 oFinalRender;

vec3 CalculatePointLight(Light light, vec3 FragPos, vec3 Normal, vec3 Diffuse)
{
	vec3 N = normalize(Normal);
	vec3 L = normalize(light.position - FragPos);

	float threshold = 1.0;

	float shadowIntensity = 1.0;

	float dist = distance(light.position, FragPos);

	if(dist > light.radius)
		shadowIntensity = 1.0 - ((dist - light.radius) / threshold);

	// Hardcoded specular parameter
    vec3 specularMat = vec3(1.0);

	// brightness
	float brightness = dot(L, Normal);

	// Specular
    float specularIntensity = pow(max(0.0, dot(N, L)), 1.0);
    vec3 specular = light.color * specularMat * specularIntensity;

	// Diffuse
    float diffuseIntensity = max(0.0, dot(N, L));

	return vec3(brightness) * (specular + diffuseIntensity) * shadowIntensity * light.intensity;
}

void main()
{
	vec3 FragPos = texture(uGPosition, vTexCoord).rgb;
    vec3 Normal = texture(uGNormals, vTexCoord).rgb;
    vec3 Diffuse = texture(uGDiffuse, vTexCoord).rgb;

	//vec3 lighting = Diffuse * 0.1;
	vec3 lighting = vec3(0.0);
	/*vec3 viewPos = uCameraPosition;
    vec3 viewDir  = normalize(viewPos - FragPos);*/

    for(int i = 0; i < uLightCount; ++i)
    {
		switch(uLight[i].type)
		{
			case 0: // Directional
			{
				float cosAngle = max(dot(Normal, -uLight[i].direction), 0.0); 
                vec3 ambient = 0.25 * uLight[i].color;
                vec3 diffuse = 0.75 * uLight[i].color * cosAngle;

                lighting += (ambient + diffuse) * Diffuse;
			}
			break;

			case 1: // Point
			{
				// diffuse
				/*vec3 lightDir = normalize(uLight[i].position - FragPos);
				vec3 diffuse = max(dot(Normal, lightDir), 0.0) * Diffuse * uLight[i].color;
				// specular
				vec3 halfwayDir = normalize(lightDir + viewDir);  
				float spec = pow(max(dot(Normal, halfwayDir), 0.0), 16.0);
				vec3 specular = uLight[i].color * spec;
				// attenuation
				float distance = length(uLight[i].position - FragPos);
				float attenuation = 1.0 / (1.0 + 0.1 * distance + 0.01 * distance * distance);
				diffuse *= attenuation;
				specular *= attenuation;
				lighting += (diffuse + specular) * uLight[i].intensity;*/

				lighting += CalculatePointLight(uLight[i], FragPos, Normal, Diffuse);
			}
			break;

			default:
			{
				
			}
			break;
		}
    }

	oFinalRender = vec4(lighting * Diffuse, 1.0);
}


#endif
#endif

#ifdef LIGHT_VOLUME

#if defined(VERTEX) ///////////////////////////////////////////////////

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

uniform mat4 uProjection;
uniform mat4 uView;
uniform mat4 uModel;

void main()
{
	gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

uniform vec3 uLightColor;

layout(location = 0) out vec4 oFinalRender;

void main()
{
	oFinalRender = vec4(uLightColor, 1.0);
}


#endif
#endif


// NOTE: You can write several shaders in the same file if you want as
// long as you embrace them within an #ifdef block (as you can see above).
// The third parameter of the LoadProgram function in engine.cpp allows
// chosing the shader you want to load by name.