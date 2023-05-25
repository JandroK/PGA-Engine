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
float far  = 500.0; 
  
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

out Data
{
	vec3 positionViewspace;
	vec3 normalViewspace;
} VSOut;

void main(void)
{
	VSOut.positionViewspace = vec3(worldViewMatrix * vec4(aPosition, 1.0));
	VSOut.normalViewspace = vec3(worldViewMatrix * vec4(aNormal, 1.0));
	gl_Position = projectionMatrix * vec4(VSOut.positionViewspace, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

uniform vec2 viewportSize;
uniform mat4 viewMatrixInv;
uniform mat4 projectionMatrixInv;

uniform sampler2D reflectionMap;
uniform sampler2D reflectionDepth;

uniform sampler2D refractionMap;
uniform sampler2D refractionDepth;

uniform sampler2D normalMap;
uniform sampler2D dudvMap;

in Data {
	vec3 positionViewspace;
	vec3 normalViewspace;
} FSIn;

out vec4 oColor;

vec3 fresnelSchlick(float cosThete, vec3 F0){
	return F0 + (1.0 - F0) * pow(1.0 - cosThete, 5.0);
}

vec3 reconstructPixelPosition(float depth) {
	vec2 texCoords = gl_FragCoord.xy / viewportSize;
	vec3 positionNDC = vec3(texCoords * 2.0 - vec2(1.0), depth * 2.0 - 1.0);
	vec4 positionEyespace = projectionMatrixInv * vec4(positionNDC, 1.0);
	positionEyespace.xyz /= positionEyespace.w;
	return positionEyespace.xyz;
}

void main(void)
{
	vec3 N = normalize(FSIn.normalViewspace);
	vec3 V = normalize (-FSIn.positionViewspace);
	vec3 Pw = vec3(viewMatrixInv * vec4(FSIn.positionViewspace, 1.0)); // position in world space
	vec2 texCoord = gl_FragCoord.xy / viewportSize;

	const vec2 waveLength = vec2(2.0);
	const vec2 waveStrength = vec2(0.05);
	const float turbidityDistance = 10.0;

	//vec2 distortion = (2.0 * texture(dudvMap, Pw.xz / waveLength).rg - vec2(1.0)) * waveStrength + waveStrength / 7;

	// Distorted reflection and refraction
	vec2 reflectionTexCoord = vec2(texCoord.s, 1.0 - texCoord.t);// + distortion;
	vec2 refractionTexCoord = texCoord;// + distortion;
	vec3 reflectionColor = texture(reflectionMap, reflectionTexCoord).rgb;
	vec3 refractionColor = texture(refractionMap, refractionTexCoord).rgb;

	// Water tint
	float distortedGroundDepth = texture(refractionDepth, refractionTexCoord).x;
	vec3 distortedGroundPosViewspace = reconstructPixelPosition(distortedGroundDepth);
	float distortedWaterDepth = FSIn.positionViewspace.z - distortedGroundPosViewspace.z;
	float tintFactor = clamp(distortedWaterDepth / turbidityDistance, 0.0, 1.0);
	vec3 waterColor = vec3(0.25, 0.4, 0.6);
	refractionColor = mix(refractionColor, waterColor, tintFactor);

	// Fresnel
	vec3 F0 = vec3(0.1);
	vec3 F = fresnelSchlick(max (0.0, dot(V, N)), F0);
	oColor.rgb = mix(refractionColor, reflectionColor, F);
	oColor.a = 1.0;
}

#endif
#endif