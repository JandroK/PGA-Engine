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
///////////////////////////////////////////////////////////////////////

#ifdef SHOW_TEXTURED_MESH

struct Light
{
    unsigned int type;
    vec3         color;
    vec3         direction;
    vec3         position;
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

out vec2 vTexCoord;
out vec3 vPosition;	// In worldspace
out vec3 vNormal;	// In worldspace
out vec3 vViewDir;

void main()
{
	vTexCoord = aTexCoord;

	vPosition = vec3(uWorldMatrix * vec4(aPosition, 1.0));
	vNormal   = vec3(uWorldMatrix * vec4(aNormal, 0.0));
	vViewDir  = uCameraPosition - vPosition;
	gl_Position = uWorldViewProjectionMatrix * vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec2 vTexCoord;
in vec3 vPosition;
in vec3 vNormal;
in vec3 vViewDir;

uniform sampler2D uTexture;

layout(binding = 0, std140) uniform GlobalParams
{
	vec3 			uCameraPosition;
	unsigned int 	uLightCount;
	Light 			uLight[16];
};

layout(location = 0) out vec4 oColor;

vec3 ComputeLight(vec3 lightDir, vec3 color)
{
	float diff = max(dot(vNormal, lightDir), 0.0f);
	return vec3(diff) * color;
}

void main()
{
	// finalColor = texture color
	vec4 finalColor = texture(uTexture, vTexCoord);

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
				lightColor += ComputeLight(uLight[i].direction, uLight[i].color);
			}
			break;
			// Point Light
			case 1:
			{
				vec3 lightDir = normalize(uLight[i].position - vPosition);
				lightColor += ComputeLight(lightDir, uLight[i].color);
			}
			break;
		}
	}

	oColor = finalColor * vec4(lightColor, 1.0f);
}

#endif
#endif


// NOTE: You can write several shaders in the same file if you want as
// long as you embrace them within an #ifdef block (as you can see above).
// The third parameter of the LoadProgram function in engine.cpp allows
// chosing the shader you want to load by name.
