//
// engine.h: This file contains the types and functions relative to the engine.
//

#pragma once

#include "platform.h"
#include <glad/glad.h>

typedef glm::vec2  vec2;
typedef glm::vec3  vec3;
typedef glm::vec4  vec4;
typedef glm::ivec2 ivec2;
typedef glm::ivec3 ivec3;
typedef glm::ivec4 ivec4;

struct VertexBufferAttribute
{
    u8 location;
    u8 componentCount;
    u8 offset;
};

struct VertexBufferLayout
{
    std::vector<VertexBufferAttribute> attributes;
    u8                                 stride;
};

struct VertexShaderAttribute
{
    VertexShaderAttribute(u8 loc, u8 count) { location = loc, componentCount = count; }

    u8 location;
    u8 componentCount;
};

struct VertexShaderLayout
{
    std::vector<VertexShaderAttribute> attributes;
};

struct Vao 
{
    GLuint handle;
    GLuint programHandle;
};

struct Image
{
    void* pixels;
    ivec2 size;
    i32   nchannels;
    i32   stride;
};

struct Texture
{
    GLuint      handle;
    std::string filepath;
};

struct Material
{
    std::string name;
    vec3        albedo;
    vec3        emissive;
    f32         smoothness;
    u32         albedoTextureIdx;
    u32         emissiveTextureIdx;
    u32         specularTextureIdx;
    u32         normalsTextureIdx;
    u32         bumpTextureIdx;
};

struct Submesh
{
    VertexBufferLayout vertexBufferLayout;
    std::vector<float> vertices;
    std::vector<u32>   indices;
    u32                vertexOffset;
    u32                indexOffset;

    std::vector<Vao>   vaos;
};

struct Mesh
{
    std::vector<Submesh> submeshes;
    GLuint               vertexBufferHandle;
    GLuint               indexBufferHandle;
};

struct Model
{
    u32              meshIdx;
    std::vector<u32> materialIdx;
};

struct Program
{
    GLuint             handle;
    std::string        filepath;
    std::string        programName;
    VertexShaderLayout vertexInputLayout;

    u64                lastWriteTimestamp; // What is this for?
};

struct Buffer
{
    GLuint  handle;
    GLenum  type;
    u32     size;
    u32     head;
    void*   data; // mapped data
};

enum Mode
{
    TEXTURED_QUAD,  // To render UI (maybe)
    MESH,          // To render meshes

    COUNT          // Number of Modes [MAX_MODE]
};

struct OpenGLInfo
{
    std::string                 glVersion;
    std::string                 glRender;
    std::string                 glVendor;
    std::string                 glShadingVersion;
    std::vector<std::string>    glExtensions;
};

struct VertexV3V2
{
    glm::vec3 pos;
    glm::vec2 uv;
};

struct Camera 
{
    glm::mat4 projection;
    glm::mat4 view;
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;

    float speed;
    float sensibility;
    float yaw;
    float pitch;

    float aspectRatio;
    float zNear;
    float zFar;
    float FOV;
};

struct Transform
{
    Transform(vec3 pos = vec3(0.0f, 0.0f, 0.0f), vec3 rot = vec3(0.0f, 0.0f, 0.0f), vec3 factor = vec3(1.0f, 1.0f, 1.0f))
    {
        position = pos;
        rotation = rot;
        scale = factor;
    }

    vec3 position;
    vec3 rotation;
    vec3 scale;
};

struct Entity
{
    Transform   transform;
    glm::mat4   worldMatrix;
    u32         modelIndex;
    u32         localParamsOffset;
    u32         localParamsSize;

    std::string name;
};

enum LightType
{
    DIRECTIONAL_LIGHT = 0,
    POINT_LIGHT = 1
};

struct Light
{
    Light(vec3 pos = vec3(0.0f), vec3 dir = vec3(1.0f), vec3 col = vec3(1.0f), LightType t = LightType::POINT_LIGHT, std::string n = "PointLight")
    {
        position = pos;
        direction = dir;
        color = col;
        type = t;
        name = n;
    }

    LightType   type;
    vec3        color;
    vec3        direction;
    vec3        position;

    std::string name;
};

enum PrimitiveType
{
    CUBE = 0,
    SPHERE,
    CYLINDER,
    CONE,
    TORUS,
    PLANE,
};

const VertexV3V2 vertices[] = {
    {glm::vec3(-0.5, -0.5, 0.0), glm::vec2(0.0, 0.0)},
    {glm::vec3(0.5, -0.5, 0.0), glm::vec2(1.0, 0.0)},
    {glm::vec3(0.5, 0.5, 0.0), glm::vec2(1.0, 1.0)},
    {glm::vec3(-0.5, 0.5, 0.0), glm::vec2(0.0, 1.0)},
};

const u16 indices[] = {
    0,1,2,
    0,2,3
};

struct App
{
    // Loop
    f32  deltaTime;
    bool isRunning;

    // Input
    Input input;

    // Graphics
    char gpuName[64];
    char openGlVersion[64];

    ivec2 displaySize;

    std::vector<Texture>  textures;
    std::vector<Material> materials;
    std::vector<Mesh>     meshes;
    std::vector<Model>    models;
    std::vector<Program>  programs;

    // program indices
    u32 texturedGeometryProgramIdx;
    
    // texture indices
    u32 whiteTexIdx;
    u32 blackTexIdx;
    u32 normalTexIdx;
    u32 magentaTexIdx;

    // Mode
    Mode mode;

    // Embedded geometry (in-editor simple meshes such as
    // a screen filling quad, a cube, a sphere...)
    GLuint embeddedVertices;
    GLuint embeddedElements;

    // Location of the texture uniform in the textured quad shader
    GLuint programUniformTexture;

    // VAO object to link our screen filling quad with our textured quad shader
    GLuint vao;

    // Buffer handle
    Buffer uniformBuffer;

    // Uniform Block Alignment
    GLint uniformBufferAlignment;

    // GlobalParams
    u32 globalParamsSize;
    u32 globalParamsOffset;

    // GlInfo
    OpenGLInfo glInfo;

    // Camera
    Camera camera;

    // Entity
    std::vector<Entity> entities;

    // Lights
    std::vector<Light> lights;

    // PrimitiveIndex
    std::vector<u32> primitiveIndex;
    std::vector<std::string> primitiveNames;
};

void Init(App* app);

void CreateUniformBuffers(App* app);

void InitCamera(App* app);

void InicializeGLInfo(App* app);

void LoadTextures(App* app);

void InicializeResources(App* app);

void Gui(App* app);

void GuiPrimitives(App* app);

void GuiLightsInstance(App* app);

void GuiEntities(App* app);

void GuiLights(App* app);

void ShowOpenGlInfo(App* app);

std::string GetNewEntityName(App* app, std::string& name);

std::string GetNewLightName(App* app, std::string& name);

void Update(App* app);

void MoveCamera(App* app);

void LookAtCamera(App* app);

void UniformBufferAlignment(App* app);

void Render(App* app);

u32 LoadTexture2D(App* app, const char* filepath);

GLuint FindVAO(Mesh& mesh, u32 submeshIndex, const Program& program);

glm::mat4 TransformConstructor(const Transform t);

glm::mat4 TransformPosition(glm::mat4 matrix, const vec3& pos);

glm::mat4 TransformScale(const glm::mat4 transform, const vec3& scaleFactor);

u8 GetComponentCount(const GLenum& type);

Entity GeneratePrimitive(u32 primitiveIndex, std::string name);

Light InstanceLight(LightType type, std::string name);

