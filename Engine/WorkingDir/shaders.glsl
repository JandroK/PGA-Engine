///////////////////////////////////////////////////////////////////////

#ifdef TEXTURED_GEOMETRY

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location=0) in vec3 aPosition;
layout(location=1) in vec2 aTexCoord;

out vec2 vTexCoord;

void main()
{
	vTexCoord = aTexCoord;
	gl_Position = vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec2 vTexCoord;

uniform sampler2D uTexture;

layout(location=0) out vec4 oColor;

void main()
{
	oColor = texture(uTexture, vTexCoord);
}

#endif
#endif

///////////////////////////////////////////////////////////////////////
///////////////////////////// DEFERRED ////////////////////////////////
///////////////////////////////////////////////////////////////////////

#ifdef SHOW_GEOMETRY_PASS

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
//layout(location = 3) in vec3 aTangent;
//layout(location = 4) in vec3 aBitangent;

layout(binding = 1, std140) uniform LocalParams
{
	mat4 uWorldMatrix;
	mat4 uWorldViewProjectionMatrix;
};

layout(binding = 2, std140) uniform ClippingPlane
{
	mat4 camView;
	vec4 clippingPlane;
};

out vec2 vTexCoord;
out vec3 vPosition;	// In worldspace
out vec3 vNormal;	// In worldspace

void main()
{
	vTexCoord = aTexCoord;

	vPosition = vec3(uWorldMatrix * vec4(aPosition, 1.0));
	vNormal   = vec3(uWorldMatrix * vec4(aNormal, 0.0));

	vec4 clipDistanceDisplacement = vec4(0.0, 0.0, 0.0, length(camView * vec4(aPosition, 1.0)) / 100);
	gl_ClipDistance[0] = dot(vec4(vPosition, 1.0), clippingPlane + clipDistanceDisplacement);
	gl_Position = uWorldViewProjectionMatrix * vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec2 vTexCoord;
in vec3 vPosition;
in vec3 vNormal;

uniform sampler2D uTexture;

layout(location = 0) out vec4 oColor;
layout(location = 1) out vec4 oPosition;
layout(location = 2) out vec4 oNormal;
layout(location = 3) out vec4 oDepth;

// Same values of camara parameters
float near = 0.1; 
float far  = 300.0; 
  
float LinearizeDepth(float depth) 
{
    float z = depth * 2.0 - 1.0; // back to NDC 
    return (2.0 * near * far) / (far + near - z * (far - near));	
}

void main()
{
	// Albedo Texture
	oColor = texture(uTexture, vTexCoord);
	// Position texture
	oPosition = vec4(vPosition, 1.0);
	// Normal texture
	oNormal = vec4(normalize(vNormal), 1.0);
	// Depth texture
	float depth = LinearizeDepth(gl_FragCoord.z) / far;
    oDepth = vec4(vec3(depth), 1.0);
}

#endif
#endif

#ifdef SHOW_LIGHTING_PASS

struct Light
{
    unsigned int type;
    vec3         color;
    vec3         direction;
    vec3         position;
	float 		 radius;
	float        intensity;
};

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;
//layout(location = 3) in vec3 aTangent;
//layout(location = 4) in vec3 aBitangent;

layout(binding = 0, std140) uniform GlobalParams
{
	vec3 			uCameraPosition;
	unsigned int 	uLightCount;
	Light 			uLight[16];
};

out vec2 vTexCoord;

void main()
{
	vTexCoord = aTexCoord;
	gl_Position = vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec2 vTexCoord;

uniform sampler2D uGAlbedo;
uniform sampler2D uGPosition;
uniform sampler2D uGNormal;

layout(location = 0) out vec4 oFinal;

layout(binding = 0, std140) uniform GlobalParams
{
	vec3 			uCameraPosition;
	unsigned int 	uLightCount;
	Light 			uLight[16];
};

vec3 ComputeDirectionalLight(vec3 lightDir, vec3 color, vec3 Normal)
{
	lightDir = normalize(lightDir);
	float diff = max(dot(Normal, lightDir), 0.0f);
	float ambient = 0.1;

	return vec3(diff + ambient) * color;
}

vec3 ComputePointLight(vec3 lightDir, Light light, vec3 Normal, float dist)
{
	lightDir = normalize(lightDir);
	float diff = max(dot(Normal, lightDir), 0.0f);
	float shadowIntensity = 1 - (dist / light.radius);

	return vec3(diff) * light.color * light.intensity * shadowIntensity;
}

void main()
{
	// Retrieve data from G-buffer
    vec3 Albedo = texture(uGAlbedo, vTexCoord).rgb;
    vec3 FragPos = texture(uGPosition, vTexCoord).rgb;
    vec3 Normal = normalize(texture(uGNormal, vTexCoord).rgb);
	float alpha = texture(uGAlbedo, vTexCoord).a;

	if(alpha == 0.0)
		discard;
	else
	{
		// lightColor = the sum of all light, if there aren't any,
		// complete darkness = vec(0.0f);
		vec3 lightColor = vec3(0.0f);

		for(int i = 0; i < uLightCount; ++i)
		{
			switch(uLight[i].type)
			{
			// Directional Light
			case 0:
			{
				lightColor += ComputeDirectionalLight(uLight[i].direction, uLight[i].color, Normal) * Albedo;
			}
			break;
			// Point Light
			case 1:
			{
				vec3 lightDir = uLight[i].position - FragPos;
				float dist = length(lightDir);
				if(dist < uLight[i].radius)
					lightColor += ComputePointLight(lightDir, uLight[i], Normal, dist) * Albedo;
			}
			break;
			}
		}
		oFinal = vec4(lightColor, 1.0f);
	}
}

#endif
#endif

#ifdef SHOW_LIGHT_DEBUG

#if defined(VERTEX) ///////////////////////////////////////////////////

layout (location = 0) in vec3 aPosition;
uniform mat4 worldViewProjection;

void main()
{
	gl_Position = worldViewProjection * vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

layout(location=0) out vec4 oColor;
uniform vec3 uLightColor;

void main()
{
	oColor = vec4(vec3(uLightColor), 1.0);
}

#endif
#endif


///////////////////////////////////////////////////////////////////////
///////////////////////////// FORWARD /////////////////////////////////
///////////////////////////////////////////////////////////////////////

#ifdef SHOW_FORWARD

struct Light
{
    unsigned int type;
    vec3         color;
    vec3         direction;
    vec3         position;
	float 		 radius;
	float        intensity;
};

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
//layout(location = 3) in vec3 aTangent;
//layout(location = 4) in vec3 aBitangent;

layout(binding = 0, std140) uniform GlobalParams
{
	vec3 			uCameraPosition;
	unsigned int 	uLightCount;
	Light 			uLight[16];
};

layout(binding = 1, std140) uniform LocalParams
{
	mat4 uWorldMatrix;
	mat4 uWorldViewProjectionMatrix;
};

layout(binding = 2, std140) uniform ClippingPlane
{
	mat4 camView;
	vec4 clippingPlane;
};

out vec2 vTexCoord;
out vec3 vPosition;	// In worldspace
out vec3 vNormal;	// In worldspace

void main()
{
	vTexCoord = aTexCoord;

	vPosition = vec3(uWorldMatrix * vec4(aPosition, 1.0));
	vNormal   = vec3(uWorldMatrix * vec4(aNormal, 0.0));	

	vec4 clipDistanceDisplacement = vec4(0.0, 0.0, 0.0, length(camView * vec4(aPosition, 1.0)) / 100);
	gl_ClipDistance[0] = dot(vec4(vPosition, 1.0), clippingPlane + clipDistanceDisplacement);
	gl_Position = uWorldViewProjectionMatrix * vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec2 vTexCoord;
in vec3 vPosition;
in vec3 vNormal;

uniform sampler2D uTexture;

layout(binding = 0, std140) uniform GlobalParams
{
	vec3 			uCameraPosition;
	unsigned int 	uLightCount;
	Light 			uLight[16];
};

layout(location = 0) out vec4 oColor;

vec3 ComputeDirectionalLight(vec3 lightDir, vec3 color, vec3 Normal)
{
	lightDir = normalize(lightDir);
	float diff = max(dot(Normal, lightDir), 0.0f);
	float ambient = 0.1;

	return vec3(diff + ambient) * color;
}

vec3 ComputePointLight(vec3 lightDir, Light light, vec3 Normal, float dist)
{
	lightDir = normalize(lightDir);
	float diff = max(dot(Normal, lightDir), 0.0f);
	float shadowIntensity = 1 - (dist / light.radius);

	return vec3(diff) * light.color * light.intensity * shadowIntensity;
}

void main()
{
	// finalColor = texture color
	vec4 finalColor = texture(uTexture, vTexCoord);

	// lightColor = the sum of all light, if there aren't any
	vec3 lightColor = vec3(0.0f);

	for(int i = 0; i < uLightCount; ++i)
	{
		switch(uLight[i].type)
		{
		// Directional Light
		case 0:
		{
			lightColor += ComputeDirectionalLight(uLight[i].direction, uLight[i].color, normalize(vNormal));
		}
		break;
		// Point Light
		case 1:
		{
			vec3 lightDir = uLight[i].position - vPosition;
			float dist = length(lightDir);
			if(dist < uLight[i].radius)
				lightColor += ComputePointLight(lightDir, uLight[i], normalize(vNormal), dist);
		}
		break;
		}
	}

	oColor = finalColor * vec4(lightColor, 1.0f);
}

#endif
#endif

///////////////////////////////////////////////////////////////////////
///////////////////////////// CUBEMAP /////////////////////////////////
///////////////////////////////////////////////////////////////////////

#ifdef SHOW_CUBE_MAP

#if defined(VERTEX) ///////////////////////////////////////////////////

layout (location = 0) in vec3 aPosition;
out vec3 TexCoords;
uniform mat4 worldViewProjection;

void main()
{
	TexCoords = -aPosition;
	vec4 pos = worldViewProjection * vec4(aPosition, 1.0);
	gl_Position = pos.xyww;
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

out vec4 FragColor;
in vec3 TexCoords;

uniform samplerCube skybox;

void main()
{
	FragColor = texture(skybox, TexCoords);
}

#endif
#endif

///////////////////////////////////////////////////////////////////////
///////////////////////////// WATER ///////////////////////////////////
///////////////////////////////////////////////////////////////////////

#ifdef SHOW_WATER

#if defined(VERTEX) ///////////////////////////////////////////////////

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;

uniform mat4 projectionMatrix;
uniform mat4 worldViewMatrix;
uniform mat4 uWorldMatrix;

out vec4 clipSpace;
out vec2 textureCoords;

out vec3 vPosition;	// In worldspace
out vec3 vNormal;	// In worldspace

const float tiling = 2;

void main(void)
{
	vPosition = vec3(uWorldMatrix * vec4(aPosition, 1.0));
	vNormal   = vec3(uWorldMatrix * vec4(aNormal, 0.0));

	clipSpace = projectionMatrix * worldViewMatrix * vec4(aPosition, 1.0);
	gl_Position = clipSpace;
	textureCoords = vec2(aPosition.x / 2.0 + 0.5, aPosition.z / 2.0 + 0.5);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

uniform int RTT;
uniform float moveFactor;
uniform sampler2D reflectionMap;
uniform sampler2D refractionMap;
uniform sampler2D dudvMap;

in vec4 clipSpace;
in vec2 textureCoords;
in vec3 vPosition;
in vec3 vNormal;

const float waveStrength = 0.02;

out vec4 oColor;

// Same values of camara parameters
float near = 0.1; 
float far  = 300.0; 
  
float LinearizeDepth(float depth) 
{
    float z = depth * 2.0 - 1.0; // back to NDC 
    return (2.0 * near * far) / (far + near - z * (far - near));	
}

void main(void)
{
	switch(RTT)
	{
		// Albedo
		case 0:
		oColor = vec4(0.25, 0.4, 0.6, 1.0);
		break;
		// Normal
		case 1:
		oColor = vec4(normalize(vNormal), 1.0);
		break;
		// Position
		case 2:
		oColor = vec4(vPosition, 1.0);
		break;
		// Depth
		case 3:
		{
			float depth = LinearizeDepth(gl_FragCoord.z) / far;
    		oColor = vec4(vec3(depth), 1.0);
		}		
		break;
		// Final
		default:
		{
			vec2 NDC = (clipSpace.xy / clipSpace.w) / 2.0 + 0.5;
			vec2 refractTexCoords = vec2(NDC.x, NDC.y);
			vec2 reflectTexCoords = vec2(NDC.x, -NDC.y);

			vec2 distortion1 = (texture(dudvMap, vec2(textureCoords.x + moveFactor, textureCoords.y)).rg * 2.0 - 1.0) * waveStrength;

			refractTexCoords += distortion1;
			refractTexCoords = clamp(refractTexCoords, 0.001, 0.999);

			reflectTexCoords += distortion1;
			reflectTexCoords.x = clamp(reflectTexCoords.x, 0.001, 0.999);
			reflectTexCoords.y = clamp(reflectTexCoords.y, -0.999, -0.001);

			vec4 reflectColour = texture(reflectionMap, reflectTexCoords);
			vec4 refractColour = texture(refractionMap, refractTexCoords);

			oColor = mix(reflectColour, refractColour, 0.5);
		}
		break;
	}	
}

#endif
#endif