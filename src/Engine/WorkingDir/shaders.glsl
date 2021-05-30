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
	float radius;
};

layout(binding = 0, std140) uniform GlobalParams
{
	vec3 uCameraPosition;
	unsigned int uLightCount;
	Light uLight[50];
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
	float radius;
};

layout(binding = 0, std140) uniform GlobalParams
{
	vec3 uCameraPosition;
	unsigned int uLightCount;
	Light uLight[50];
};

uniform sampler2D uTexture;
uniform samplerCube uSkybox;

layout(location = 0) out vec4 oFinalRender;

out float gl_FragDepth;

vec3 CalculateDirectionalLight(Light light)
{
	return vec3(1.0);
}

vec3 CalculatePointLight(Light light)
{
	vec3 lightVector = normalize(light.position - vPosition);
	float brightness = dot(lightVector, vNormal);

	return vec3(brightness) * light.color;
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

	vec3 I = normalize(vPosition - uCameraPosition);
	vec3 R = reflect(I, normalize(vNormal));

	vec4 reflections = vec4(texture(uSkybox, R).rgb, 1.0);
	oFinalRender = mix(vec4(lightFactor, 1.0) * objectColor, reflections, 0.5);

	gl_FragDepth = gl_FragCoord.z - 0.2;
}


#endif
#endif

#ifdef SHOW_TEXTURED_MESH_WITH_CLIPPING

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
	float radius;
};

layout(binding = 0, std140) uniform GlobalParams
{
	vec3 uCameraPosition;
	unsigned int uLightCount;
	Light uLight[50];
};

layout(binding = 1, std140) uniform LocalParams
{
	mat4 uWorldMatrix;
	mat4 uWorldViewProjectionMatrix;
};

uniform mat4 uProjection;
uniform mat4 uView;
uniform mat4 uModel;

uniform vec4 uClippingPlane;

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

	vec3 positionWorldSpace = vPosition;
	gl_ClipDistance[0] = dot(vec4(positionWorldSpace, 1.0), uClippingPlane);

	gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);
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
	float radius;
};

layout(binding = 0, std140) uniform GlobalParams
{
	vec3 uCameraPosition;
	unsigned int uLightCount;
	Light uLight[50];
};

uniform sampler2D uTexture;
uniform samplerCube uSkybox;

layout(location = 0) out vec4 oFinalRender;

out float gl_FragDepth;

vec3 CalculateDirectionalLight(Light light)
{
	return vec3(1.0);
}

vec3 CalculatePointLight(Light light)
{
	vec3 lightVector = normalize(light.position - vPosition);
	float brightness = dot(lightVector, vNormal);

	return vec3(brightness) * light.color;
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

	vec3 I = normalize(vPosition - uCameraPosition);
	vec3 R = reflect(I, normalize(vNormal));

	vec4 reflections = vec4(texture(uSkybox, R).rgb, 1.0);
	oFinalRender = mix(vec4(lightFactor, 1.0) * objectColor, reflections, 0.5);

	gl_FragDepth = gl_FragCoord.z - 0.2;
}


#endif
#endif

#ifdef WATER_MESH

#if defined(VERTEX) ///////////////////////////////////////////////////

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

uniform mat4 uProjection;
uniform mat4 uView;
uniform mat4 uModel;

uniform vec3 uCameraPosition;

out vec4 vClipSpace;
out vec2 vTexCoords;
out vec3 vToCameraVector;

const float tiling = 1.0;

void main()
{
	vec4 worldPosition =  uModel * vec4(aPosition, 1.0);

	vClipSpace = uProjection * uView * worldPosition;
	vTexCoords = vec2(aPosition.x / 2.0 + 0.5, aPosition.y / 2.0 + 0.5) * tiling;
	vToCameraVector = uCameraPosition - worldPosition.xyz;

	gl_Position = vClipSpace;
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

layout(location = 0) out vec4 oFinalRender;

uniform sampler2D uReflectionTexture;
uniform sampler2D uRefractionTexture;

uniform sampler2D uDudvMap;

uniform float uMoveFactor;

in vec4 vClipSpace;
in vec2 vTexCoords;
in vec3 vToCameraVector;

const float waveStrength = 0.02;

void main()
{
	vec2 ndc = (vClipSpace.xy / vClipSpace.w) / 2.0 + 0.5;
	
	vec2 reflectTexCoords = vec2(ndc.x, -ndc.y);
	vec2 refractTexCoords = vec2(ndc.x, ndc.y);

	vec2 distortion01 = (texture(uDudvMap, vec2(vTexCoords.x + uMoveFactor, vTexCoords.y)).rg * 2.0 - 1.0) * waveStrength;
	vec2 distortion02 = (texture(uDudvMap, vec2(-vTexCoords.x + uMoveFactor, vTexCoords.y + uMoveFactor)).rg * 2.0 - 1.0) * waveStrength;
	vec2 totalDistortion = distortion01 + distortion02;

	reflectTexCoords += totalDistortion;
	refractTexCoords += totalDistortion;

	vec4 reflectColor = texture(uReflectionTexture, reflectTexCoords);
	vec4 refractColor = texture(uRefractionTexture, refractTexCoords);

	vec3 viewVector = normalize(vToCameraVector);
	float refractiveFactor = dot(viewVector, vec3(0.0, 1.0, 0.0));
	refractiveFactor = pow(refractiveFactor, 0.5);

	oFinalRender = mix(reflectColor, refractColor, refractiveFactor);
	oFinalRender = mix(oFinalRender, vec4(0.0, 0.3, 0.5, 1.0), 0.25);
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

#ifdef DEFERRED_LIGHTING_PASS

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
	Light uLight[50];
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
	Light uLight[50];
};

uniform sampler2D uGPosition;
uniform sampler2D uGNormals;
uniform sampler2D uGDiffuse;

layout(location = 0) out vec4 oFinalRender;

vec3 CalculateDirectionalLight(Light light, vec3 Normal, vec3 Diffuse)
{
	/*vec3 N = normalize(Normal);
    vec3 L = normalize(light.direction);

	// Hardcoded specular parameter
    vec3 specularMat = vec3(0.5);

	// Diffuse
    float diffuseIntensity = max(0.0, dot(N,L));

	// Specular
    float specularIntensity = pow(max(0.0, dot(N, L)), 1.0);
    vec3 specular = specularMat * specularIntensity;


	return (diffuseIntensity + specular) * light.intensity * light.color;*/
	float cosAngle = max(dot(Normal, -light.direction), 0.0); 
    vec3 ambient = 0.1 * light.color;
    vec3 diffuse = 0.9 * light.color * cosAngle;

    return (ambient + diffuse) * Diffuse;
}

vec3 CalculatePointLight(Light light, vec3 FragPos, vec3 Normal)
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


	// Specular
    float specularIntensity = pow(max(0.0, dot(N, L)), 1.0);
    vec3 specular = specularMat * specularIntensity;

	// Diffuse
    float diffuseIntensity = max(0.0, dot(N, L));

	return vec3(specular + diffuseIntensity) * shadowIntensity * light.intensity * light.color;
}

void main()
{
	vec3 FragPos = texture(uGPosition, vTexCoord).rgb;
    vec3 Normal = texture(uGNormals, vTexCoord).rgb;
    vec3 Diffuse = texture(uGDiffuse, vTexCoord).rgb;

	vec3 viewDir = normalize(uCameraPosition - FragPos);

	vec3 lighting = Diffuse * 0.1;
    for(int i = 0; i < uLightCount; ++i)
    {
		switch(uLight[i].type)
		{
			case 0: // Directional
			{
                lighting += CalculateDirectionalLight(uLight[i], Normal, Diffuse);
			}
			break;

			case 1: // Point
			{
				float distance = length(uLight[i].position - FragPos);
				if(distance < uLight[i].radius)
				{
					lighting += CalculatePointLight(uLight[i], FragPos, Normal);
				}
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

#ifdef SKYBOX

#if defined(VERTEX) ///////////////////////////////////////////////////

layout (location = 0) in vec3 aPosition;

uniform mat4 uProjection;
uniform mat4 uView;

out vec3 vTexCoord;

void main()
{
	vTexCoord = aPosition;
	vec4 pos = uProjection * uView * vec4(aPosition, 1.0);
	gl_Position = pos.xyww;
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

uniform samplerCube uSkybox;

in vec3 vTexCoord;

layout(location = 0) out vec4 oFragColor;

void main()
{
	oFragColor = texture(uSkybox, vTexCoord);
}


#endif
#endif


// NOTE: You can write several shaders in the same file if you want as
// long as you embrace them within an #ifdef block (as you can see above).
// The third parameter of the LoadProgram function in engine.cpp allows
// chosing the shader you want to load by name.