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
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);
	app->renderModes["Color"] = app->colorAttachmentTexture;

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
	app->renderModes["Depth"] = app->depthAttachmentTexture;

	// Final Texture
	glGenTextures(1, &app->finalAttachmentTexture);
	glBindTexture(GL_TEXTURE_2D, app->finalAttachmentTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);
	app->renderModes["Final"] = app->finalAttachmentTexture;

	// Depth Component
	glGenTextures(1, &app->depthAttachmentHandle);
	glBindTexture(GL_TEXTURE_2D, app->depthAttachmentHandle);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, app->displaySize.x, app->displaySize.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenFramebuffers(1, &app->fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, app->fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, app->colorAttachmentTexture, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, app->depthAttachmentTexture, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, app->finalAttachmentTexture, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, app->depthAttachmentHandle, 0);

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
	// Select on which render target to draw
	GLuint drawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	glDrawBuffers(ARRAY_COUNT(drawBuffers), drawBuffers);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Init(App* app)
{
	//InicializeResources(app);
	LoadTextures(app);
	InicializeGLInfo(app);

	//////////////////////////////////

	// FRAMEBUFFERS
	GenerateRenderTextures(app);

	// Init Camera
	InitCamera(app);

	// Crating uniform buffers
	CreateUniformBuffers(app);

	// Loading Models
	for (int i = 0; i < app->primitiveNames.size(); i++)
	{
		std::string path = "Primitives/" + app->primitiveNames[i] + ".obj";
		app->primitiveIndex.push_back(LoadModel(app, path.c_str()));
	}

	u32 modelIndex = LoadModel(app, "Patrick/patrick.obj"); // "Backpack/backpack.obj"

	// Fill entities array
	for (size_t i = 0; i < 3; ++i)
	{
		Entity e;
		vec3 pos = glm::rotate(glm::radians(360.0f / 3.0f * i), vec3(0.0f, 1.0f, 0.0f)) * vec4(vec3(4.0f, 4.0f, 0.0f), 1.0f);
		e.transform = Transform(pos, vec3(0.0f, -90.0f, 0.0f));
		e.worldMatrix = TransformConstructor(e.transform);
		e.modelIndex = modelIndex;
		char n = static_cast<char>(i);
		e.name = "Patrick " + std::to_string(i);

		app->entities.push_back(e);
	}

	// Set up playground
	Entity playground = GeneratePrimitive(app->primitiveIndex[PrimitiveType::PLANE], "Playground");
	app->entities.push_back(playground);


	// Create light
	app->lights.push_back(InstanceLight(DIRECTIONAL_LIGHT,  "Directional Light"));
	app->lights.push_back(InstanceLight(POINT_LIGHT,		"Point Light"));

	// Load programs
	app->texturedGeometryProgramIdx = LoadProgram(app, "shaders.glsl", "SHOW_TEXTURED_MESH");
	Program& texturedGeometryProgram = app->programs[app->texturedGeometryProgramIdx];

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

	app->programUniformTexture = glGetUniformLocation(texturedGeometryProgram.handle, "uTexture");

	app->mode = MESH;
}

void InitCamera(App* app)
{
	app->camera.aspectRatio = (float)app->displaySize.x / (float)app->displaySize.y;
	app->camera.zNear = 0.1f;
	app->camera.zFar = 100.0f;
	app->camera.FOV = 60.0f;

	app->camera.position = vec3(-20.0f, 10.0f, 5.0f);
	app->camera.front = glm::normalize(vec3(20.0f, -9.0f, -5.0f));
	app->camera.up = vec3(0.0f, 1.0f, 0.0f);
	app->camera.projection = glm::perspective(glm::radians(app->camera.FOV), app->camera.aspectRatio, app->camera.zNear, app->camera.zFar);
	app->camera.view = glm::lookAt(app->camera.position, app->camera.position + app->camera.front, app->camera.up);

	app->camera.speed = 15.0f;
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

	if (ImGui::BeginCombo("##RenderMode", app->currentRenderMode.c_str()))
	{
		for (auto it : app->renderModes)
		{
			bool isSelected = !app->currentRenderMode.compare(it.first);
			if (ImGui::Selectable(it.first.c_str(), isSelected))
				app->currentRenderMode = it.first;
			if (isSelected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::Separator();
		ImGui::EndCombo();
	}

	ImGui::NewLine();

	GuiEntities(app);
	GuiLights(app);

	ImGui::End();

	// Scene
	if (ImGui::Begin("Scene"))
	{
		ImVec2 size = ImGui::GetContentRegionAvail();
		RecalculateProjection(app, glm::vec2(size.x, size.y));
		ImGui::Image((ImTextureID)app->renderModes[app->currentRenderMode], size, ImVec2(0, 1), ImVec2(1, 0));
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
				ImGui::Text("Direction: ");
				glm::vec3& dir = app->lights[i].direction;
				ImGui::DragFloat3("##Direction", &dir[0], 0.1f, true);
			}
			break;
			case POINT_LIGHT:
			{
				ImGui::Text("Position: ");
				glm::vec3& pos = app->lights[i].position;
				ImGui::DragFloat3("##Position", &pos[0], 0.1f, true);
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
	// You can handle app->input keyboard/mouse here
	MoveCamera(app);
	LookAtCamera(app);

	// Zoom camera (why don't can extrat to function?)
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

	UniformBufferAlignment(app);
}

void MoveCamera(App* app)
{
	if (app->input.keys[K_W] == ButtonState::BUTTON_PRESSED)
		app->camera.position += app->camera.speed * app->deltaTime * app->camera.front;
	if (app->input.keys[K_S] == ButtonState::BUTTON_PRESSED)
		app->camera.position -= app->camera.speed * app->deltaTime * app->camera.front;
	if (app->input.keys[K_A] == ButtonState::BUTTON_PRESSED)
		app->camera.position -= glm::normalize(glm::cross(app->camera.front, app->camera.up)) * app->camera.speed * app->deltaTime;
	if (app->input.keys[K_D] == ButtonState::BUTTON_PRESSED)
		app->camera.position += glm::normalize(glm::cross(app->camera.front, app->camera.up)) * app->camera.speed * app->deltaTime;
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

		glm::vec3 direction;
		direction.x = cos(glm::radians(app->camera.yaw)) * cos(glm::radians(app->camera.pitch));
		direction.y = sin(glm::radians(app->camera.pitch));
		direction.z = sin(glm::radians(app->camera.yaw)) * cos(glm::radians(app->camera.pitch));
		app->camera.front = glm::normalize(direction);
	}
}

void UniformBufferAlignment(App* app)
{
	glBindBuffer(GL_UNIFORM_BUFFER, app->uniformBuffer.handle);
	app->uniformBuffer.data = (u8*)glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
	app->uniformBuffer.head = 0;

	// Global params
	app->globalParamsOffset = app->uniformBuffer.head;

	PushVec3(app->uniformBuffer, app->camera.position);
	PushUInt(app->uniformBuffer, app->lights.size());

	for (u32 i = 0; i < app->lights.size(); ++i)
	{
		AlignHead(app->uniformBuffer, sizeof(vec4));

		Light& light = app->lights[i];
		PushUInt(app->uniformBuffer, light.type);
		PushVec3(app->uniformBuffer, light.color);
		PushVec3(app->uniformBuffer, light.direction);
		PushVec3(app->uniformBuffer, light.position);
	}

	app->globalParamsSize = app->uniformBuffer.head - app->globalParamsOffset;

	// Local Params
	for (u32 i = 0; i < app->entities.size(); ++i)
	{
		AlignHead(app->uniformBuffer, app->uniformBufferAlignment);
		Entity& entity = app->entities[i];
		glm::mat4 world = entity.worldMatrix;
		glm::mat4 worldViewProjection = app->camera.projection * app->camera.view * world;

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
	// Auxiliar clear to fix ImGui bug tabs loop draw
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Render on this framebuffer render target
	glBindFramebuffer(GL_FRAMEBUFFER, app->fbo);

	// Clean screen
	glClearColor(0.1, 0.1, 0.1, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);

	// Indicate to OpneGL the screen size in the current frame
	glViewport(0, 0, app->displaySize.x, app->displaySize.y);

	switch (app->mode)
	{
	case TEXTURED_QUAD:
	{
		// Indicate which shader we are going to use
		Program& programTexturedGeometry = app->programs[app->texturedGeometryProgramIdx];
		glUseProgram(programTexturedGeometry.handle);
		glBindVertexArray(app->vao);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// Use 1i because we pass it 1 int, if we pass it a vector3 of floats we would use glUniform3f
		glUniform1i(app->programUniformTexture, 0); // ID = 0
		glActiveTexture(GL_TEXTURE0);				// The last number of GL_TEXTURE must match the above number which is 0
		GLuint textureHandle = app->textures[app->whiteTexIdx].handle;
		glBindTexture(GL_TEXTURE_2D, textureHandle);

		// Dibujamos usando triangulos
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

		glBindVertexArray(0);
		glUseProgram(0);

		break;
	}
	case MESH:
	{
		// Indicate which shader we are going to use
		Program& programTexturedGeometry = app->programs[app->texturedGeometryProgramIdx];
		glUseProgram(programTexturedGeometry.handle);

		for (Entity entity : app->entities)
		{
			Model& model = app->models[entity.modelIndex];
			Mesh& mesh = app->meshes[model.meshIdx];

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
				glUniform1i(app->programUniformTexture, 0);

				Submesh& submesh = mesh.submeshes[i];
				glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.indexOffset);

				glBindVertexArray(0);
			}
		}

		glUseProgram(0);
		break;
	}
	default:
		break;
	}

	// Render on screen again
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
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
		return Light(vec3(0.0f),			 vec3(-1.0f), vec3(1.0f), DIRECTIONAL_LIGHT, name);
		break;
	case POINT_LIGHT:
		return Light(vec3(0.0f, 5.0f, 0.0f), vec3(1.0f),  vec3(1.0f), POINT_LIGHT,		 name);
		break;
	default:
		break;
	}
}
