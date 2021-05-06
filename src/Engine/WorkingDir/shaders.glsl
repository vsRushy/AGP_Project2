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
	vec3 position;
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
	vec3 position;
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
out float gl_FragDepth;

vec3 CalculateDirectionalLight()
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

	vec3 result = vec3(0.0);
	for(int i = 0; i < uLightCount; ++i)
	{
		switch(uLight[i].type)
		{
		case 0: // Directional
		{
			result += CalculateDirectionalLight();
		}
		break;

		case 1: // Point
		{
			result += CalculatePointLight(uLight[i]);
		}
		break;

		default:
		{
			break;
		}
		}
	}

	oFinalRender = vec4(result, 1.0) * objectColor;
	oNormals = vec4(1.0, 0.0, 0.0, 1.0);

	gl_FragDepth = gl_FragCoord.z;
}


#endif
#endif


// NOTE: You can write several shaders in the same file if you want as
// long as you embrace them within an #ifdef block (as you can see above).
// The third parameter of the LoadProgram function in engine.cpp allows
// chosing the shader you want to load by name.