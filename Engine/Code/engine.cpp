//
// engine.cpp : Put all your graphics stuff in this file. This is kind of the graphics module.
// In here, you should type all your OpenGL commands, and you can also type code to handle
// input platform events (e.g to move the camera or react to certain shortcuts), writing some
// graphics related GUI options, and so on.
//

#include "buffer_management.h"
#include <imgui.h>
#include <stb_image.h>
#include <stb_image_write.h>
#include <math.h>

#include "assimp_model_loading.h"

#define BINDING(b) b

GLuint CreateProgramFromSource(String programSource, const char* shaderName)
{
	GLchar  infoLogBuffer[1024] = {};
	GLsizei infoLogBufferSize = sizeof(infoLogBuffer);
	GLsizei infoLogSize;
	GLint   success;

	char versionString[] = "#version 430\n";
	char shaderNameDefine[128];
	sprintf(shaderNameDefine, "#define %s\n", shaderName);
	char vertexShaderDefine[] = "#define VERTEX\n";
	char fragmentShaderDefine[] = "#define FRAGMENT\n";

	const GLchar* vertexShaderSource[] = {
		versionString,
		shaderNameDefine,
		vertexShaderDefine,
		programSource.str
	};
	const GLint vertexShaderLengths[] = {
		(GLint)strlen(versionString),
		(GLint)strlen(shaderNameDefine),
		(GLint)strlen(vertexShaderDefine),
		(GLint)programSource.len
	};
	const GLchar* fragmentShaderSource[] = {
		versionString,
		shaderNameDefine,
		fragmentShaderDefine,
		programSource.str
	};
	const GLint fragmentShaderLengths[] = {
		(GLint)strlen(versionString),
		(GLint)strlen(shaderNameDefine),
		(GLint)strlen(fragmentShaderDefine),
		(GLint)programSource.len
	};

	GLuint vshader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vshader, ARRAY_COUNT(vertexShaderSource), vertexShaderSource, vertexShaderLengths);
	glCompileShader(vshader);
	glGetShaderiv(vshader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vshader, infoLogBufferSize, &infoLogSize, infoLogBuffer);
		ELOG("glCompileShader() failed with vertex shader %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
		assert(success);
	}

	GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fshader, ARRAY_COUNT(fragmentShaderSource), fragmentShaderSource, fragmentShaderLengths);
	glCompileShader(fshader);
	glGetShaderiv(fshader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fshader, infoLogBufferSize, &infoLogSize, infoLogBuffer);
		ELOG("glCompileShader() failed with fragment shader %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
		assert(success);
	}

	GLuint programHandle = glCreateProgram();
	glAttachShader(programHandle, vshader);
	glAttachShader(programHandle, fshader);
	glLinkProgram(programHandle);
	glGetProgramiv(programHandle, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(programHandle, infoLogBufferSize, &infoLogSize, infoLogBuffer);
		ELOG("glLinkProgram() failed with program %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
		assert(success);
	}

	glUseProgram(0);

	glDetachShader(programHandle, vshader);
	glDetachShader(programHandle, fshader);
	glDeleteShader(vshader);
	glDeleteShader(fshader);

	return programHandle;
}

u32 LoadProgram(App* app, const char* filepath, const char* programName)
{
	String programSource = ReadTextFile(filepath);

	Program program = {};
	program.handle = CreateProgramFromSource(programSource, programName);
	program.filepath = filepath;
	program.programName = programName;
	program.lastWriteTimestamp = GetFileLastWriteTimestamp(filepath);
	app->programs.push_back(program);

	return app->programs.size() - 1;
}

Image LoadImage(const char* filename)
{
	Image img = {};
	stbi_set_flip_vertically_on_load(true);
	img.pixels = stbi_load(filename, &img.size.x, &img.size.y, &img.nchannels, 0);
	if (img.pixels)
	{
		img.stride = img.size.x * img.nchannels;
	}
	else
	{
		ELOG("Could not open file %s", filename);
	}
	return img;
}

void FreeImage(Image image)
{
	stbi_image_free(image.pixels);
}

GLuint CreateTexture2DFromImage(Image image)
{
	GLenum internalFormat = GL_RGB8;
	GLenum dataFormat = GL_RGB;
	GLenum dataType = GL_UNSIGNED_BYTE;

	switch (image.nchannels)
	{
	case 3: dataFormat = GL_RGB; internalFormat = GL_RGB8; break;
	case 4: dataFormat = GL_RGBA; internalFormat = GL_RGBA8; break;
	default: ELOG("LoadTexture2D() - Unsupported number of channels");
	}

	GLuint texHandle;
	glGenTextures(1, &texHandle);
	glBindTexture(GL_TEXTURE_2D, texHandle);
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, image.size.x, image.size.y, 0, dataFormat, dataType, image.pixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);

	return texHandle;
}

u32 LoadTexture2D(App* app, const char* filepath)
{
	for (u32 texIdx = 0; texIdx < app->textures.size(); ++texIdx)
		if (app->textures[texIdx].filepath == filepath)
			return texIdx;

	Image image = LoadImage(filepath);

	if (image.pixels)
	{
		Texture tex = {};
		tex.handle = CreateTexture2DFromImage(image);
		tex.filepath = filepath;

		u32 texIdx = app->textures.size();
		app->textures.push_back(tex);

		FreeImage(image);
		return texIdx;
	}
	else
	{
		return UINT32_MAX;
	}
}

void InicializeResources(App* app)
{
	// Initialize the resources
	glGenBuffers(1, &app->embeddedVertices);
	glBindBuffer(GL_ARRAY_BUFFER, app->embeddedVertices);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Prepare index buffer
	glGenBuffers(1, &app->embeddedElements);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedElements);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// Prepare Vertex Array Object
	glGenVertexArrays(1, &app->vao);
	glBindVertexArray(app->vao);
	glBindBuffer(GL_ARRAY_BUFFER, app->embeddedVertices);

	// Layouts
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexV3V2), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexV3V2), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedElements);
	glBindVertexArray(0);
}

void LoadTextures(App* app)
{
	app->whiteTexIdx = LoadTexture2D(app, "color_white.png");
	app->blackTexIdx = LoadTexture2D(app, "color_black.png");
	app->normalTexIdx = LoadTexture2D(app, "color_normal.png");
	app->magentaTexIdx = LoadTexture2D(app, "color_magenta.png");
}

void InicializeGLInfo(App* app)
{
	app->glInfo.glVersion = reinterpret_cast<const char*>(glGetString(GL_VERSION));
	app->glInfo.glRender = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
	app->glInfo.glVendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
	app->glInfo.glShadingVersion = reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION));

	GLint numExtensions = 0;
	glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
	for (GLint i = 0; i < numExtensions; ++i)
	{
		app->glInfo.glExtensions.push_back(reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, GLuint(i))));
	}
}

void GenerateRenderTextures(App* app)
{
	// Albedo Texture
	glGenTextures(1, &app->colorAttachmentTexture);
	glBindTexture(GL_TEXTURE_2D, app->colorAttachmentTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);
	app->renderTargets["Color"] = app->colorAttachmentTexture;
	
	// Position Texture
	glGenTextures(1, &app->positionAttachmentTexture);
	glBindTexture(GL_TEXTURE_2D, app->positionAttachmentTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);
	app->renderTargets["Position"] = app->positionAttachmentTexture;

	// Normal Texture
	glGenTextures(1, &app->normalAttachmentTexture);
	glBindTexture(GL_TEXTURE_2D, app->normalAttachmentTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);
	app->renderTargets["Normal"] = app->normalAttachmentTexture;

	// Depth Texture
	glGenTextures(1, &app->depthAttachmentTexture);
	glBindTexture(GL_TEXTURE_2D, app->depthAttachmentTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);
	app->renderTargets["Depth"] = app->depthAttachmentTexture;

	// Depth Component
	GLuint depthAttachmentHandle;
	glGenTextures(1, &depthAttachmentHandle);
	glBindTexture(GL_TEXTURE_2D, depthAttachmentHandle);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, app->displaySize.x, app->displaySize.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenFramebuffers(1, &app->gBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, app->gBuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, app->colorAttachmentTexture, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, app->positionAttachmentTexture, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, app->normalAttachmentTexture, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, app->depthAttachmentTexture, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthAttachmentHandle, 0);

	CheckFrameBufferStatus();
	
	// Select on which render target to draw
	GLuint drawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
	glDrawBuffers(ARRAY_COUNT(drawBuffers), drawBuffers);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//////////////////////////////////////////////////////////////////////////

	// Final Texture
	glGenTextures(1, &app->finalAttachmentTexture);
	glBindTexture(GL_TEXTURE_2D, app->finalAttachmentTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);
	app->renderTargets["Final"] = app->finalAttachmentTexture;

	// Depth Component
	GLuint depthLightingAttachmentHandle;
	glGenTextures(1, &depthLightingAttachmentHandle);
	glBindTexture(GL_TEXTURE_2D, depthLightingAttachmentHandle);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, app->displaySize.x, app->displaySize.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenFramebuffers(1, &app->lightBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, app->lightBuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, app->finalAttachmentTexture, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthLightingAttachmentHandle, 0);

	CheckFrameBufferStatus();

	// Select on which render target to draw
	GLuint drawLightBuffers[] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(ARRAY_COUNT(drawLightBuffers), drawLightBuffers);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GenerateRenderTexturesWater(App* app)
{
	// Reflection Texture
	glGenTextures(1, &app->rtReflection);
	glBindTexture(GL_TEXTURE_2D, app->rtReflection);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Refraction Texture
	glGenTextures(1, &app->rtRefraction);
	glBindTexture(GL_TEXTURE_2D, app->rtRefraction);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Depth Component
	glGenTextures(1, &app->rtReflectionDepth);
	glBindTexture(GL_TEXTURE_2D, app->rtReflectionDepth);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, app->displaySize.x, app->displaySize.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Depth Component
	glGenTextures(1, &app->rtRefractionDepth);
	glBindTexture(GL_TEXTURE_2D, app->rtRefractionDepth);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, app->displaySize.x, app->displaySize.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenFramebuffers(1, &app->fboReflection);
	glBindFramebuffer(GL_FRAMEBUFFER, app->fboReflection);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, app->rtReflection, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, app->rtReflectionDepth, 0);

	CheckFrameBufferStatus();

	// Select on which render target to draw
	GLuint drawReflections[] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(ARRAY_COUNT(drawReflections), drawReflections);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glGenFramebuffers(1, &app->fboRefraction);
	glBindFramebuffer(GL_FRAMEBUFFER, app->fboRefraction);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, app->rtRefraction, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, app->rtRefractionDepth, 0);

	CheckFrameBufferStatus();

	// Select on which render target to draw
	GLuint drawRefractions[] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(ARRAY_COUNT(drawRefractions), drawRefractions);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void CheckFrameBufferStatus()
{
	GLenum framebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (framebufferStatus != GL_FRAMEBUFFER_COMPLETE)
	{
		switch (framebufferStatus)
		{
		case GL_FRAMEBUFFER_UNDEFINED:						ELOG("GL_FRAMEBUFFER_UNDEFINED"); break;
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:			ELOG("GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT"); break;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:	ELOG("GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"); break;
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:			ELOG("GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER"); break;
		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:			ELOG("GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER"); break;
		case GL_FRAMEBUFFER_UNSUPPORTED:					ELOG("GL_FRAMEBUFFER_UNSUPPORTED"); break;
		case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:			ELOG("GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE"); break;
		case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:		ELOG("GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS"); break;
		default:
			ELOG("Unknow framebuffer status error")
				break;
		}
	}
}

GLuint LoadCubemap(App* app)
{
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height, nrComponents;
	std::vector<std::string> faces = {"Skybox/right.jpg", "Skybox/left.jpg", "Skybox/bottom.jpg", "Skybox/top.jpg", "Skybox/front.jpg", "Skybox/back.jpg"};
	for (unsigned int i = 0; i < faces.size(); i++)
	{
		unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrComponents, 0);
		if (data)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			stbi_image_free(data);
		}
		else
		{
			ELOG("Cubemap tex failed to load at path: %s", faces[i]);
			stbi_image_free(data);
		}
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}

void GenerateSkyboxVAO(App* app)
{
	float skyboxVertices[] = {
		// positions          
		-1.0f,  1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		-1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f
	};

	// Skybox VAO
	glGenVertexArrays(1, &app->skyboxVAO);
	glGenBuffers(1, &app->skyboxVBO);
	glBindVertexArray(app->skyboxVAO);
	glBindBuffer(GL_ARRAY_BUFFER, app->skyboxVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

	//Unbinding
	glBindVertexArray(0);
}

void Init(App* app)
{
	InicializeResources(app);
	LoadTextures(app);
	InicializeGLInfo(app);

	//////////////////////////////////

	// FRAMEBUFFERS
	GenerateRenderTextures(app);

	// Init Camera
	InitCamera(app);

	// Crating uniform buffers
	CreateUniformBuffers(app);

	// Crate Skybox
	app->skyboxID = LoadCubemap(app);
	GenerateSkyboxVAO(app);

	// Loading Models
	for (int i = 0; i < app->primitiveNames.size(); i++)
	{
		std::string path = "Primitives/" + app->primitiveNames[i] + ".obj";
		app->primitiveIndex.push_back(LoadModel(app, path.c_str()));
	}
	app->sphereIndex = app->primitiveIndex[1];
	app->quadIndex = app->primitiveIndex[5];

	Entity e;
	e.transform = Transform(vec3(0.0f), vec3(0.0f, -90.0f, 0.0f), vec3(0.5f));
	e.worldMatrix = TransformConstructor(e.transform);
	e.modelIndex = LoadModel(app, "House/casita2.obj");
	e.name = "House";

	app->entities.push_back(e);

	// Create light
	Light lDir = InstanceLight(DIRECTIONAL_LIGHT, "Directional Light 0");
	lDir.color = glm::normalize(vec3(98.0f, 128.0f, 160.0f));
	lDir.position = vec3(17.8f, 27.5f, 0.0f);
	app->lights.push_back(lDir);

	lDir.name = "Directional Light 1";
	lDir.position = vec3(0.0f, 32.0f, 0.0f);
	lDir.direction = vec3(0.0f, -1.0f, 0.0f);
	app->lights.push_back(lDir);

	Light l = InstanceLight(POINT_LIGHT, "Point Light 0");
	l.color = glm::normalize(vec3(254.0f, 101.0f, 53.0f));
	l.position = vec3(6.6f, 18.9f, 11.2f);
	app->lights.push_back(l);

	l.name = "Point Light 1";
	l.position = vec3(-8.6f, 19.7f, 11.2f);
	app->lights.push_back(l);

	l.name = "Point Light 2";
	l.position = vec3(5.0f, 9.9f, 9.8f);
	app->lights.push_back(l);

	l.name = "Point Light 3";
	l.position = vec3(-7.1f, 9.9f, 9.8f);
	app->lights.push_back(l);

	l.name = "Point Light 4";
	l.position = vec3(6.3f, 19.3f, -12.1f);
	app->lights.push_back(l);

	l.name = "Point Light 5";
	l.position = vec3(-8.2f, 18.2f, -12.1f);
	app->lights.push_back(l);

	// Load programs
	app->texturedDeferredGeometryProgramIdx = LoadProgram(app, "shaders.glsl", "SHOW_GEOMETRY_PASS");
	LoadShader(app, app->texturedDeferredGeometryProgramIdx);
	app->programDeferredUniformTexture = glGetUniformLocation(app->programs[app->texturedDeferredGeometryProgramIdx].handle, "uTexture");

	app->texturedLightingProgramIdx = LoadProgram(app, "shaders.glsl", "SHOW_LIGHTING_PASS");
	LoadShader(app, app->texturedLightingProgramIdx);
	app->uGAlbedo = glGetUniformLocation(app->programs[app->texturedLightingProgramIdx].handle, "uGAlbedo");
	app->uGPosition = glGetUniformLocation(app->programs[app->texturedLightingProgramIdx].handle, "uGPosition");
	app->uGNormal = glGetUniformLocation(app->programs[app->texturedLightingProgramIdx].handle, "uGNormal");

	app->debugLightsProgramIdx = LoadProgram(app, "shaders.glsl", "SHOW_LIGHT_DEBUG");
	LoadShader(app, app->debugLightsProgramIdx);
	app->uWorldViewProjection = glGetUniformLocation(app->programs[app->debugLightsProgramIdx].handle, "worldViewProjection");
	app->uDebugLightColor = glGetUniformLocation(app->programs[app->debugLightsProgramIdx].handle, "uLightColor");

	app->texturedForwardGeometryProgramIdx = LoadProgram(app, "shaders.glsl", "SHOW_FORWARD");
	LoadShader(app, app->texturedForwardGeometryProgramIdx);
	app->programForwardUniformTexture = glGetUniformLocation(app->programs[app->texturedForwardGeometryProgramIdx].handle, "uTexture");

	app->cubemapProgramIdx = LoadProgram(app, "shaders.glsl", "SHOW_CUBE_MAP");
	LoadShader(app, app->cubemapProgramIdx);
	app->cubemapuWorldViewProjection = glGetUniformLocation(app->programs[app->cubemapProgramIdx].handle, "worldViewProjection");
	app->cubemapTexture = glGetUniformLocation(app->programs[app->cubemapProgramIdx].handle, "skybox");

	app->waterProgramIdx = LoadProgram(app, "shaders.glsl", "SHOW_WATER");
	LoadShader(app, app->waterProgramIdx);
	app->wateruWorldViewProjection = glGetUniformLocation(app->programs[app->waterProgramIdx].handle, "worldViewProjection");

	app->mode = FORWARD;
	app->currentMode = "Forward";
}

void LoadShader(App* app, u32 index)
{
	Program& texturedGeometryProgram = app->programs[index];

	GLint attributeCount;
	glGetProgramiv(texturedGeometryProgram.handle, GL_ACTIVE_ATTRIBUTES, &attributeCount);

	for (GLint i = 0; i < attributeCount; ++i)
	{
		GLchar  attributeName[32];
		GLsizei attributeNameLength;
		GLint   attributeSize;
		GLenum  attributeType;

		glGetActiveAttrib(texturedGeometryProgram.handle, i,
			ARRAY_COUNT(attributeName),
			&attributeNameLength,
			&attributeSize,
			&attributeType,
			attributeName);

		GLint attributeLocation = glGetAttribLocation(texturedGeometryProgram.handle, attributeName);
		VertexShaderAttribute attribute = VertexShaderAttribute(attributeLocation, GetComponentCount(attributeType));
		texturedGeometryProgram.vertexInputLayout.attributes.push_back(attribute);
	}
}

void InitCamera(App* app)
{
	app->camera.aspectRatio = (float)app->displaySize.x / (float)app->displaySize.y;
	app->camera.zNear = 0.1f;
	app->camera.zFar = 500.0f;
	app->camera.FOV = 60.0f;

	app->camera.position = vec3(50.0f, 40.0f, 60.0f);
	app->camera.target = vec3(0.0f, 13.0f, 0.0f);
	app->camera.upWorld = vec3(0.0f, 1.0f, 0.0f);
	app->camera.front = glm::normalize(vec3(-50.0f, -28.0f, -60.0f));
	app->camera.right = glm::normalize(glm::cross(app->camera.front, app->camera.upWorld));
	app->camera.up = glm::normalize(glm::cross(app->camera.right, app->camera.front));
	app->camera.projection = glm::perspective(glm::radians(app->camera.FOV), app->camera.aspectRatio, app->camera.zNear, app->camera.zFar);
	app->camera.view = glm::lookAt(app->camera.position, app->camera.position + app->camera.front, app->camera.up);

	app->camera.speed = 15.0f;
	app->camera.orbitSpeed = 10.0f;
	app->camera.sensibility = 0.15f;
	app->camera.pitch = glm::degrees(asin(app->camera.front.y));
	app->camera.yaw = -glm::degrees(acos(app->camera.front.x / cos(glm::radians(app->camera.pitch))));
}

void CreateUniformBuffers(App* app)
{
	GLint maxUniformBufferSize;
	glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxUniformBufferSize);
	glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &app->uniformBufferAlignment);

	// For each buffer you need to create
	app->uniformBuffer = CreateConstantBuffer(maxUniformBufferSize);
}

void CreateDocking()
{
	// DockSpace
	static bool dockOpen = true;
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->GetWorkPos());
	ImGui::SetNextWindowSize(viewport->GetWorkSize());

	ImGuiWindowFlags host_window_flags = 0;
	host_window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking;
	host_window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.5f, 0.5f));
	ImGui::Begin("DockSpace", &dockOpen, host_window_flags);
	ImGui::PopStyleVar(3);

	ImGuiID dockspaceId = ImGui::GetID("MyDockSpace");
	ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
	ImGui::End();
}

void Gui(App* app)
{
	// Docking
	CreateDocking();

	// Info
	ImGui::Begin("Info");
	ImGui::Text("FPS: %f", 1.0f / app->deltaTime);
	ImGui::End();

	// Inspector Transform
	ImGui::Begin("Inspector Transform");

	GuiPrimitives(app);
	GuiLightsInstance(app);

	ImGui::NewLine();
	ImGui::Text("Render Mode:");
	if (ImGui::BeginCombo("##RenderMode", app->currentMode.c_str()))
	{
		for (std::string it : app->renderModes)
		{
			bool isSelected = !app->currentMode.compare(it);
			if (ImGui::Selectable(it.c_str(), isSelected))
			{
				app->currentMode = it;
				app->mode = (it == "Deferred") ? DEFERRED : FORWARD;				
			}
			if (isSelected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::Separator();
		ImGui::EndCombo();
	}

	if (app->mode == DEFERRED)
	{
		ImGui::NewLine();

		ImGui::NewLine();
		ImGui::Text("Render Targets Textures:");
		if (ImGui::BeginCombo("##RenderTargetsTextures", app->currentRenderTarget.c_str()))
		{
			for (auto it : app->renderTargets)
			{
				bool isSelected = !app->currentRenderTarget.compare(it.first);
				if (ImGui::Selectable(it.first.c_str(), isSelected))
					app->currentRenderTarget = it.first;
				if (isSelected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::Separator();
			ImGui::EndCombo();
		}

		ImGui::NewLine();
	}

	ImGui::Text("Pivot Point: ");
	glm::vec3& position = app->camera.target;
	ImGui::DragFloat3("##Pivot Point", &position[0], 0.5f, true);
	ImGui::NewLine();

	GuiEntities(app);
	GuiLights(app);

	ImGui::End();

	// Scene
	if (ImGui::Begin("Scene"))
	{
		ImVec2 size = ImGui::GetContentRegionAvail();
		RecalculateProjection(app, glm::vec2(size.x, size.y));

		if(app->mode == DEFERRED)
			ImGui::Image((ImTextureID)app->renderTargets[app->currentRenderTarget], size, ImVec2(0, 1), ImVec2(1, 0));
		else
			ImGui::Image((ImTextureID)app->renderTargets["Color"], size, ImVec2(0, 1), ImVec2(1, 0));
	}
	ImGui::End();

	//ShowOpenGlInfo(app);
}

void RecalculateProjection(App* app, glm::vec2 size)
{
	app->camera.aspectRatio = size.x / size.y;
	app->camera.projection = glm::perspective(glm::radians(app->camera.FOV), app->camera.aspectRatio, app->camera.zNear, app->camera.zFar);
}

void GuiPrimitives(App* app)
{
	// Draw the drop down to generate primitive
	if (ImGui::BeginMenu("Primitives"))
	{
		for (size_t i = 0; i < app->primitiveIndex.size(); i++)
		{
			if (ImGui::MenuItem(app->primitiveNames[i].c_str(), nullptr))
			{
				std::string name = app->primitiveNames[i].c_str();
				name += " " +  GetNewEntityName(app, name);
				app->entities.push_back(GeneratePrimitive(app->primitiveIndex[i], name));
			}
		}

		ImGui::EndMenu();
	}
}

void GuiLightsInstance(App* app)
{
	// Draw the drop down to generate light
	if (ImGui::BeginMenu("Lights"))
	{
		if (ImGui::MenuItem("Directional Light", nullptr))
		{
			std::string name = "Directional Light";
			name += " " + GetNewLightName(app, name);
			app->lights.push_back(InstanceLight(DIRECTIONAL_LIGHT, name));
		}
		if (ImGui::MenuItem("Point Light", nullptr))
		{
			std::string name = "Point Light";
			name += " " + GetNewLightName(app, name);
			app->lights.push_back(InstanceLight(POINT_LIGHT, name));
		}

		ImGui::EndMenu();
	}
}

void GuiEntities(App* app)
{
	for (int i = 0; i < app->entities.size(); ++i)
	{
		if (app->entities[i].name == "")
			break;

		ImGui::PushID(app->entities[i].name.c_str());

		if (ImGui::CollapsingHeader(app->entities[i].name.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Text("Position: ");
			glm::vec3& position = app->entities[i].transform.position;
			if (ImGui::DragFloat3("##Position", &position[0], 0.5f, true))
			{
				app->entities[i].worldMatrix = TransformConstructor(app->entities[i].transform);
			}

			ImGui::Text("Rotation: ");
			glm::vec3& rotation = app->entities[i].transform.rotation;
			if (ImGui::DragFloat3("##Rotation", &rotation[0], 1.0f, 0.0f, 360.0f))
			{
				app->entities[i].worldMatrix = TransformConstructor(app->entities[i].transform);
			}

			ImGui::Text("Scale: ");
			glm::vec3& scale = app->entities[i].transform.scale;
			if (ImGui::DragFloat3("##Scale", &scale[0], 0.01f, 0.00001f, 10000.0f))
			{
				app->entities[i].worldMatrix = TransformConstructor(app->entities[i].transform);
			}
		}
		ImGui::PopID();
	}
}

void GuiLights(App* app)
{
	for (int i = 0; i < app->lights.size(); i++)
	{
		ImGui::PushID(app->lights[i].name.c_str());

		if (ImGui::CollapsingHeader(app->lights[i].name.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
		{
			switch (app->lights[i].type)
			{
			case DIRECTIONAL_LIGHT:
			{
				ImGui::Text("Position: ");
				glm::vec3& pos = app->lights[i].position;
				ImGui::DragFloat3("##Position", &pos[0], 0.1f, true);
				ImGui::Text("Direction: ");
				glm::vec3& dir = app->lights[i].direction;
				ImGui::DragFloat3("##Direction", &dir[0], 0.1f, true);
			}
			break;
			case POINT_LIGHT:
			{
				ImGui::Text("Position: ");
				glm::vec3& pos = app->lights[i].position;
				ImGui::DragFloat3("##Position", &pos[0],                   0.1f, true);
				ImGui::DragFloat("##Radius",    &app->lights[i].radius,    0.1f, 0.00001f, 1000000.0f);
				ImGui::DragFloat("##Intensity", &app->lights[i].intensity, 0.1f, 0.00001f, 1.0f);
			}
			break;
			default:
				break;
			}

			ImGui::Text("Color: ");
			glm::vec3& color = app->lights[i].color;
			ImGui::ColorEdit3("##Color", &color[0]);
		}

		ImGui::PopID();
	}
}

void ShowOpenGlInfo(App* app)
{
	if (app->input.keys[K_SPACE] == ButtonState::BUTTON_PRESS)
	{
		ImGui::OpenPopup("OpenGl Info");
	}

	if (ImGui::BeginPopup("OpenGl Info"))
	{
		ImGui::Text("Version: %s", app->glInfo.glVersion.c_str());
		ImGui::Text("Render: %s", app->glInfo.glRender.c_str());
		ImGui::Text("Vendor: %s", app->glInfo.glVendor.c_str());
		ImGui::Text("GLSL Version: %s", app->glInfo.glShadingVersion.c_str());

		ImGui::Separator();
		ImGui::Text("Extensions:");
		for (size_t i = 0; i < app->glInfo.glExtensions.size(); i++)
		{
			ImGui::Text("GLSL Version: %s", app->glInfo.glExtensions[i].c_str());
		}
		ImGui::EndPopup();

	}
	ImGui::CloseCurrentPopup();
}

std::string GetNewEntityName(App* app, std::string& name)
{
	int nameRepeat = 0;
	for (int i = 0; i < app->entities.size(); i++)
	{
		if (app->entities[i].name.find(name) != std::string::npos)
			nameRepeat++;
	}

	return std::to_string(nameRepeat);
}

std::string GetNewLightName(App* app, std::string& name)
{
	int nameRepeat = 0;
	for (int i = 0; i < app->lights.size(); i++)
	{
		if (app->lights[i].name.find(name) != std::string::npos)
			nameRepeat++;
	}

	return std::to_string(nameRepeat);
}

void Update(App* app)
{
	app->timeGame += app->deltaTime;
	// You can handle app->input keyboard/mouse here
	if (app->input.keys[K_CTRL] == ButtonState::BUTTON_PRESSED)
		OrbitCamera(app);
	else
		LookAtCamera(app);

	ZoomCamera(app);
	MoveCamera(app);
}

void MoveCamera(App* app)
{
	if (app->input.keys[K_W] == ButtonState::BUTTON_PRESSED)
		app->camera.position += app->camera.speed * app->deltaTime * app->camera.front;
	if (app->input.keys[K_S] == ButtonState::BUTTON_PRESSED)
		app->camera.position -= app->camera.speed * app->deltaTime * app->camera.front;
	if (app->input.keys[K_A] == ButtonState::BUTTON_PRESSED)
		app->camera.position -= app->camera.right * app->camera.speed * app->deltaTime;
	if (app->input.keys[K_D] == ButtonState::BUTTON_PRESSED)
		app->camera.position += app->camera.right * app->camera.speed * app->deltaTime;
	if (app->input.keys[K_Q] == ButtonState::BUTTON_PRESSED)
		app->camera.position -= app->camera.speed * app->deltaTime * app->camera.up;
	if (app->input.keys[K_E] == ButtonState::BUTTON_PRESSED)
		app->camera.position += app->camera.speed * app->deltaTime * app->camera.up;

	app->camera.view = glm::lookAt(app->camera.position, app->camera.position + app->camera.front, app->camera.up);
}

void LookAtCamera(App* app)
{
	if (app->input.mouseButtons[RIGHT] == ButtonState::BUTTON_PRESSED)
	{
		app->input.mouseDelta.x *= app->camera.sensibility;
		app->input.mouseDelta.y *= -app->camera.sensibility;

		app->camera.yaw += app->input.mouseDelta.x;
		app->camera.pitch += app->input.mouseDelta.y;

		if (app->camera.pitch > 89.0f)
			app->camera.pitch = 89.0f;
		if (app->camera.pitch < -89.0f)
			app->camera.pitch = -89.0f;

		ComputeCameraAxisDirection(app->camera);
	}
}

void OrbitCamera(App* app)
{
	if (app->input.mouseButtons[RIGHT] == ButtonState::BUTTON_PRESSED)
	{
		// OrbitCamera
		app->input.mouseDelta.x *= -app->camera.sensibility;
		app->input.mouseDelta.y *= -app->camera.sensibility;

		vec3 reference = app->camera.target;
		glm::quat pivot = glm::angleAxis(app->input.mouseDelta.x * app->deltaTime * app->camera.orbitSpeed, app->camera.upWorld);

		// Subir da positivo, bajat da negativo
		if (abs(app->camera.up.y) < 0.1f) // Avoid gimball lock on up & down apex
		{			
			// Look down
			if ((app->camera.position.y > reference.y && app->input.mouseDelta.y > 0.f) ||	// Look down
				(app->camera.position.y < reference.y && app->input.mouseDelta.y < 0.f))	// Look up	
				pivot = pivot * glm::angleAxis(app->input.mouseDelta.y * app->deltaTime * app->camera.orbitSpeed, app->camera.right);
		}
		else
		{
			pivot = pivot * glm::angleAxis(app->input.mouseDelta.y * app->deltaTime * app->camera.orbitSpeed, app->camera.right);
		}

		app->camera.position = pivot * (app->camera.position - reference) + reference;
		app->camera.front = glm::normalize(reference - app->camera.position);
		app->camera.right = glm::normalize(glm::cross(app->camera.front, app->camera.upWorld));
		app->camera.up = glm::normalize(glm::cross(app->camera.right, app->camera.front));

		app->camera.pitch = glm::degrees(asin(app->camera.front.y));
		if(app->camera.front.z < 0.0f)
			app->camera.yaw = -glm::degrees(acos(app->camera.front.x / cos(glm::radians(app->camera.pitch))));
		else
			app->camera.yaw = glm::degrees(acos(app->camera.front.x / cos(glm::radians(app->camera.pitch))));

		if (app->camera.pitch > 89.0f)
			app->camera.pitch = 89.0f;
		if (app->camera.pitch < -89.0f)
			app->camera.pitch = -89.0f;	
	}
}

void ZoomCamera(App* app)
{
	if (app->input.scrollDelta.y != 0.0f)
	{
		app->camera.FOV -= app->input.scrollDelta.y;
		if (app->camera.FOV < 1.0f)
			app->camera.FOV = 1.0f;
		if (app->camera.FOV > 60.0f)
			app->camera.FOV = 60;

		app->input.scrollDelta.y = 0.0f;
		app->camera.projection = glm::perspective(glm::radians(app->camera.FOV), app->camera.aspectRatio, app->camera.zNear, app->camera.zFar);
	}
}

void ComputeCameraAxisDirection(Camera& cam)
{
	glm::vec3 direction;
	direction.x = cos(glm::radians(cam.yaw)) * cos(glm::radians(cam.pitch));
	direction.y = sin(glm::radians(cam.pitch));
	direction.z = sin(glm::radians(cam.yaw)) * cos(glm::radians(cam.pitch));

	cam.front = glm::normalize(direction);
	cam.right = glm::normalize(glm::cross(cam.front, cam.upWorld));
	cam.up = glm::normalize(glm::cross(cam.right, cam.front));
}

void UniformBufferAlignment(App* app, Camera cam)
{
	glBindBuffer(GL_UNIFORM_BUFFER, app->uniformBuffer.handle);
	app->uniformBuffer.data = (u8*)glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
	app->uniformBuffer.head = 0;

	// Global params
	app->globalParamsOffset = app->uniformBuffer.head;

	PushVec3(app->uniformBuffer, cam.position);
	PushUInt(app->uniformBuffer, app->lights.size());

	for (u32 i = 0; i < app->lights.size(); ++i)
	{
		AlignHead(app->uniformBuffer, sizeof(vec4));

		Light& light = app->lights[i];
		PushUInt(app->uniformBuffer,  light.type);
		PushVec3(app->uniformBuffer,  light.color);
		PushVec3(app->uniformBuffer,  light.direction);
		PushVec3(app->uniformBuffer,  light.position);
		PushFloat(app->uniformBuffer, light.radius);
		PushFloat(app->uniformBuffer, light.intensity);
	}

	app->globalParamsSize = app->uniformBuffer.head - app->globalParamsOffset;

	// Local Params
	for (u32 i = 0; i < app->entities.size(); ++i)
	{
		AlignHead(app->uniformBuffer, app->uniformBufferAlignment);
		Entity& entity = app->entities[i];
		glm::mat4 world = entity.worldMatrix;
		glm::mat4 worldViewProjection = cam.projection * cam.view * world;

		entity.localParamsOffset = app->uniformBuffer.head;
		PushMat4(app->uniformBuffer, world);
		PushMat4(app->uniformBuffer, worldViewProjection);
		entity.localParamsSize = app->uniformBuffer.head - entity.localParamsOffset;
	}

	glUnmapBuffer(GL_UNIFORM_BUFFER);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void Render(App* app)
{
	switch (app->mode)
	{
	case TEXTURED_QUAD:
	{
		// Indicate which shader we are going to use
		Program& programTexturedGeometry = app->programs[app->texturedForwardGeometryProgramIdx];
		glUseProgram(programTexturedGeometry.handle);
		glBindVertexArray(app->vao);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// Use 1i because we pass it 1 int, if we pass it a vector3 of floats we would use glUniform3f
		glUniform1i(app->programForwardUniformTexture, 0); // ID = 0
		glActiveTexture(GL_TEXTURE0);				// The last number of GL_TEXTURE must match the above number which is 0
		GLuint textureHandle = app->textures[app->whiteTexIdx].handle;
		glBindTexture(GL_TEXTURE_2D, textureHandle);

		// Dibujamos usando triangulos
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

		glBindVertexArray(0);
		glUseProgram(0);

		break;
	}
	case FORWARD:
	{
		// Fill water render textures 
		FillRTWater(app);
		// Render World
		DrawScene(app, app->texturedForwardGeometryProgramIdx, app->programForwardUniformTexture, app->gBuffer);
		// Debug lights
		RenderDebug(app);
		// Cubemap
		RenderSkybox(app);
		// Water
		RenderWaterShader(app);

		// Render on screen again
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		break;
	}
	case DEFERRED:
	{
		// Fill water render textures 
		FillRTWater(app);

		// Render World
		DrawScene(app, app->texturedDeferredGeometryProgramIdx, app->programDeferredUniformTexture, app->gBuffer);

		if (app->currentRenderTarget != "Final")
		{
			RenderSkybox(app);	
			RenderWaterShader(app);
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		///////////////////////////////////////////////// Lighting pass ////////////////////////////////////////
		RenderDeferredLights(app);
		// Debug lights
		RenderDebug(app);		
		if (app->currentRenderTarget == "Final")
		{
			RenderSkybox(app);
			RenderWaterShader(app);			
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		break;
	}
	default:
		break;
	}
}

void DrawScene(App* app, u32 programIdx, GLuint uTexture, GLuint fbo)
{
	// Clean screen
	glClearColor(0.1, 0.1, 0.1, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Indicate to OpneGL the screen size in the current frame
	glViewport(0, 0, app->displaySize.x, app->displaySize.y);

	// Render on this framebuffer render target
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	// Indicate which shader we are going to use
	Program& programTexturedGeometry = app->programs[programIdx];
	glUseProgram(programTexturedGeometry.handle);

	for (Entity entity : app->entities)
	{
		Model& model = app->models[entity.modelIndex];
		Mesh& mesh = app->meshes[model.meshIdx];

		if(app->mode == FORWARD)
			glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(0), app->uniformBuffer.handle, app->globalParamsOffset, app->globalParamsSize);
		glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(1), app->uniformBuffer.handle, entity.localParamsOffset, entity.localParamsSize);

		for (u32 i = 0; i < mesh.submeshes.size(); ++i)
		{
			GLuint vao = FindVAO(mesh, i, programTexturedGeometry);
			glBindVertexArray(vao);
			u32 submeshMaterialIdx = model.materialIdx[i];
			Material& submeshMaterial = app->materials[submeshMaterialIdx];

			glActiveTexture(GL_TEXTURE0);
			GLuint textureHandle = app->textures[submeshMaterial.albedoTextureIdx].handle;
			glBindTexture(GL_TEXTURE_2D, textureHandle);

			glUniform1i(uTexture, 0);

			Submesh& submesh = mesh.submeshes[i];
			glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.indexOffset);

			glBindVertexArray(0);
		}
	}
	glUseProgram(0);
}

void RenderQuad(App* app)
{
	glBindVertexArray(app->vao);

	// Bind textures
	glUniform1i(app->uGAlbedo, 0);
	glUniform1i(app->uGPosition, 1);
	glUniform1i(app->uGNormal, 2);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, app->colorAttachmentTexture);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, app->positionAttachmentTexture);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, app->normalAttachmentTexture);

	// Send Uniforms
	glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(0), app->uniformBuffer.handle, app->globalParamsOffset, app->globalParamsSize);

	// Draw
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
	glBindVertexArray(0);
}

void RenderDeferredLights(App* app)
{
	// Render on this framebuffer render target
	glBindFramebuffer(GL_FRAMEBUFFER, app->lightBuffer);

	glClearColor(0.1, 0.1, 0.1, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	// Indicate which shader we are going to use
	Program& programTexturedLighting = app->programs[app->texturedLightingProgramIdx];
	glUseProgram(programTexturedLighting.handle);

	RenderQuad(app);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, app->gBuffer);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, app->lightBuffer);

	glBlitFramebuffer(0, 0, app->displaySize.x, app->displaySize.x, 0, 0, app->displaySize.x, app->displaySize.x, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
	glUseProgram(0);
}

void RenderDebug(App* app)
{
	Program& programDebugLighting = app->programs[app->debugLightsProgramIdx];
	glUseProgram(programDebugLighting.handle);

	u32 modelIndex = 0;
	float scaleFactor = 1.0;

	// Lights
	for (Light it : app->lights)
	{
		if (it.type == DIRECTIONAL_LIGHT)
		{
			modelIndex = app->quadIndex;
			scaleFactor = 0.1f;
		}
		else
		{
			modelIndex = app->sphereIndex;
			scaleFactor = 0.5f;
		}

		Mesh& mesh = app->meshes[app->models[modelIndex].meshIdx];
		GLuint vao = FindVAO(mesh, 0, programDebugLighting);

		glm::mat4 model = TransformConstructor(Transform(it.position, glm::degrees(it.direction), vec3(scaleFactor)));
		model = app->camera.projection * app->camera.view * model;

		glBindVertexArray(vao);
		glUniformMatrix4fv(app->uWorldViewProjection, 1, GL_FALSE, &model[0][0]);
		glUniform3f(app->uDebugLightColor, it.color.r, it.color.g, it.color.b);

		glDrawElements(GL_TRIANGLES, mesh.submeshes[0].indices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
	}
	// Debug Pivot Target
	Mesh& mesh = app->meshes[app->models[app->sphereIndex].meshIdx];
	GLuint vao = FindVAO(mesh, 0, programDebugLighting);

	glm::mat4 model = TransformConstructor(Transform(app->camera.target, vec3(0.0f), vec3(1.0)));
	model = app->camera.projection * app->camera.view * model;

	glBindVertexArray(vao);
	glUniformMatrix4fv(app->uWorldViewProjection, 1, GL_FALSE, &model[0][0]);
	glUniform3f(app->uDebugLightColor, 0.8f, 0.8f, 0.8f);

	glDrawElements(GL_TRIANGLES, mesh.submeshes[0].indices.size(), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

	glUseProgram(0);
}

// CUBEMAP
void RenderSkybox(App* app)
{
	glDepthFunc(GL_LEQUAL);

	Program& programCubemap = app->programs[app->cubemapProgramIdx];
	glUseProgram(programCubemap.handle);
	glBindVertexArray(app->skyboxVAO);

	glm::mat4 view = glm::mat4(glm::mat3(app->camera.view));
	glm::mat4 cubemapuWorldViewProjection = app->camera.projection * view;
	glUniformMatrix4fv(app->cubemapuWorldViewProjection, 1, GL_FALSE, &cubemapuWorldViewProjection[0][0]);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, app->skyboxID);

	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
	glDepthFunc(GL_LESS);
}

void FillRTWater(App* app)
{
	//////////////////////////////////////////////////// REFLECTION /////////////////////////////////////
	// Render on this framebuffer render target
	glBindFramebuffer(GL_FRAMEBUFFER, app->fboReflection);

	Camera reflectionCam = app->camera;
	reflectionCam.position.y *= -1;
	reflectionCam.pitch *= -1;
	ComputeCameraAxisDirection(reflectionCam);
	reflectionCam.view = glm::lookAt(reflectionCam.position, reflectionCam.position + reflectionCam.front, reflectionCam.up);

	UniformBufferAlignment(app, reflectionCam);

	PassWaterScene(app, &reflectionCam, true, app->fboReflection);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//////////////////////////////////////////////////// REFRACTION /////////////////////////////////////
	// Render on this framebuffer render target
	glBindFramebuffer(GL_FRAMEBUFFER, app->fboRefraction);

	Camera refractionCam = app->camera;
	UniformBufferAlignment(app, refractionCam);
	PassWaterScene(app, &refractionCam, false, app->fboRefraction);
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PassWaterScene(App* app, Camera* cam, bool reflection, GLuint fbo)
{
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CLIP_DISTANCE0);

	if (app->mode == FORWARD)
		DrawScene(app, app->texturedForwardGeometryProgramIdx, app->programForwardUniformTexture, fbo);
	else
		DrawScene(app, app->texturedDeferredGeometryProgramIdx, app->programDeferredUniformTexture, fbo);

	glDisable(GL_CLIP_DISTANCE0);
}

void RenderWaterShader(App* app)
{
	//Water effect
	Program& programWater = app->programs[app->waterProgramIdx];
	glUseProgram(programWater.handle);

	// Debug Pivot Target
	Mesh& mesh = app->meshes[app->models[app->quadIndex].meshIdx];
	GLuint vao = FindVAO(mesh, 0, programWater);

	glm::mat4 model = TransformConstructor(Transform(vec3(0.0), vec3(0.0), vec3(4.0)));
	model = app->camera.projection * app->camera.view * model;

	glBindVertexArray(vao);
	glUniformMatrix4fv(app->wateruWorldViewProjection, 1, GL_FALSE, &model[0][0]);

	glDrawElements(GL_TRIANGLES, mesh.submeshes[0].indices.size(), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

	glUseProgram(0);
}

GLuint FindVAO(Mesh& mesh, u32 submeshIndex, const Program& program)
{
	Submesh& submesh = mesh.submeshes[submeshIndex];

	// Try finding a vao for this submesh/program
	for (u32 i = 0; i < (u32)submesh.vaos.size(); ++i)
	{
		if (submesh.vaos[i].programHandle == program.handle)
			return submesh.vaos[i].handle;
	}
	// If don't find the vao, create a new vao for this submesh/program
	GLuint vaoHandle = 0;
	glGenVertexArrays(1, &vaoHandle);
	glBindVertexArray(vaoHandle);

	glBindBuffer(GL_ARRAY_BUFFER, mesh.vertexBufferHandle);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indexBufferHandle);

	// We have to link all vertex inputs attributes to attributrs in the vertex buffer
	for (u32 i = 0; i < program.vertexInputLayout.attributes.size(); ++i)
	{
		bool attributeWasLinked = false;
		for (u32 j = 0; j < submesh.vertexBufferLayout.attributes.size(); ++j)
		{
			if (program.vertexInputLayout.attributes[i].location == submesh.vertexBufferLayout.attributes[j].location)
			{
				const u32 index = submesh.vertexBufferLayout.attributes[j].location;
				const u32 ncomp = submesh.vertexBufferLayout.attributes[j].componentCount;
				const u32 offset = submesh.vertexBufferLayout.attributes[j].offset + submesh.vertexOffset;
				const u32 stride = submesh.vertexBufferLayout.stride;

				glVertexAttribPointer(index, ncomp, GL_FLOAT, GL_FALSE, stride, (void*)(u64)offset);
				glEnableVertexAttribArray(index);

				attributeWasLinked = true;
				break;
			}
		}

		assert(attributeWasLinked);
	}

	glBindVertexArray(0);
	// Store it in the list of vaos for this submesh
	Vao vao = { vaoHandle, program.handle };
	submesh.vaos.push_back(vao);

	return vaoHandle;
}

// Transform Constructor
glm::mat4 TransformConstructor(const Transform t)
{
	glm::mat4 transform = glm::translate(t.position);

	glm::vec3 rotRadians = glm::radians(t.rotation);
	transform = glm::rotate(transform, rotRadians.x, glm::vec3(1.0f, 0.0f, 0.0f));
	transform = glm::rotate(transform, rotRadians.y, glm::vec3(0.0f, 1.0f, 0.0f));
	transform = glm::rotate(transform, rotRadians.z, glm::vec3(0.0f, 0.0f, 1.0f));

	return glm::scale(transform, t.scale);
}

glm::mat4 TransformPosition(glm::mat4 matrix, const vec3& pos)
{
	return glm::translate(matrix, pos);
}
glm::mat4 TransformScale(const glm::mat4 transform, const vec3& scaleFactor)
{
	return glm::scale(transform, scaleFactor);
}

//////////////////////////////////////////////////

u8 GetComponentCount(const GLenum& type)
{
	switch (type)
	{
	case GL_FLOAT:				return 1;
	case GL_FLOAT_VEC2:			return 2;
	case GL_FLOAT_VEC3:			return 3;
	case GL_FLOAT_VEC4:			return 4;
	case GL_FLOAT_MAT2:			return 4;
	case GL_FLOAT_MAT3:			return 9;
	case GL_FLOAT_MAT4:			return 16;
	case GL_FLOAT_MAT2x3:		return 6;
	case GL_FLOAT_MAT2x4:		return 8;
	case GL_FLOAT_MAT3x2:		return 6;
	case GL_FLOAT_MAT3x4:		return 12;
	case GL_FLOAT_MAT4x2:		return 8;
	case GL_FLOAT_MAT4x3:		return 12;
	case GL_INT:				return 1;
	case GL_INT_VEC2:			return 2;
	case GL_INT_VEC3:			return 3;
	case GL_INT_VEC4:			return 4;
	case GL_UNSIGNED_INT:		return 1;
	case GL_UNSIGNED_INT_VEC2:	return 2;
	case GL_UNSIGNED_INT_VEC3:	return 3;
	case GL_UNSIGNED_INT_VEC4:	return 4;
	case GL_DOUBLE:				return 1;
	case GL_DOUBLE_VEC2:		return 2;
	case GL_DOUBLE_VEC3:		return 3;
	case GL_DOUBLE_VEC4:		return 4;
	case GL_DOUBLE_MAT2:		return 4;
	case GL_DOUBLE_MAT3:		return 9;
	case GL_DOUBLE_MAT4:		return 16;
	case GL_DOUBLE_MAT2x3:		return 6;
	case GL_DOUBLE_MAT2x4:		return 8;
	case GL_DOUBLE_MAT3x2:		return 6;
	case GL_DOUBLE_MAT3x4:		return 12;
	case GL_DOUBLE_MAT4x2:		return 8;
	case GL_DOUBLE_MAT4x3:		return 12;
	default:					return 0;
	}
}

// Primitives
Entity GeneratePrimitive(u32 primitiveIndex, std::string name)
{
	Entity e;
	e.transform = Transform();
	e.worldMatrix = TransformConstructor(e.transform);
	e.modelIndex = primitiveIndex;
	e.name = name;

	return e;
}

// Light

Light InstanceLight(LightType type, std::string name)
{
	switch (type)
	{
	case DIRECTIONAL_LIGHT:
		return Light(vec3(0.0f),			 vec3(-1.0f), vec3(1.0f), DIRECTIONAL_LIGHT, 10.0f, 1.0f, name);
		break;
	case POINT_LIGHT:
		return Light(vec3(0.0f, 5.0f, 0.0f), vec3(1.0f),  vec3(1.0f), POINT_LIGHT,       14.0f, 1.0f, name);
		break;
	default:
		break;
	}
}
