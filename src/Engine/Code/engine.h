//
// engine.h: This file contains the types and functions relative to the engine.
//

#pragma once

#include <glad/glad.h>

#include "platform.h"

typedef glm::vec2  vec2;
typedef glm::vec3  vec3;
typedef glm::vec4  vec4;
typedef glm::ivec2 ivec2;
typedef glm::ivec3 ivec3;
typedef glm::ivec4 ivec4;

struct OpenGLInfo
{
    const unsigned char* version;
    const unsigned char* renderer;
    const unsigned char* vendor;
    const unsigned char* glsl_version;
};

struct VertexV3V2
{
    vec3 pos;
    vec2 uv;
};

struct VertexBufferAttribute
{
    u8 location;
    u8 component_count;
    u8 offset;
};

struct VertexBufferLayout
{
    std::vector<VertexBufferAttribute> attributes;
    u8 stride;
};

struct VertexShaderAttribute
{
    u8 location;
    u8 component_count;
};

struct VertexShaderLayout
{
    std::vector<VertexShaderAttribute> attributes;
};

struct Vao
{
    GLuint handle;
    GLuint program_handle;
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

    vec3 albedo;
    vec3 emissive;

    f32 smoothness;

    u32 albedo_texture_index;
    u32 emissive_texture_index;
    u32 specular_texture_index;
    u32 normals_texture_index;
    u32 bump_texture_index;
};

struct Model
{
    u32 mesh_index;
    std::vector<u32> material_index;
};

struct Submesh
{
    VertexBufferLayout vertex_buffer_layout;

    std::vector<float> vertices;
    std::vector<u32> indices;

    u32 vertex_offset;
    u32 index_offset;

    std::vector<Vao> vaos;
};

struct Mesh
{
    std::vector<Submesh> submeshes;

    GLuint vertex_buffer_handle;
    GLuint index_buffer_handle;
};

struct Program
{
    GLuint             handle;
    std::string        filepath;
    std::string        programName;
    u64                lastWriteTimestamp;

    VertexShaderLayout vertex_input_layout;
};

struct Camera
{
    vec3 position;
    vec3 rotation;
    vec3 target;

    float fov = 65.0f;
    float far_plane = 1000.0f;
    float near_plane = 0.1f;
};

struct Entity
{
    glm::mat4 worldMatrix;
    u32 modelIndex;
    u32 localParamsOffset;
    u32 localParamsSize;
};

enum Mode
{
    Mode_TexturedQuad,
    Mode_Count
};

struct App
{
    // Loop
    f32  deltaTime;
    bool isRunning;

    // Input
    Input input;

    // Graphics
    OpenGLInfo opengl_info;

    ivec2 displaySize;

    std::vector<Texture>    textures;
    std::vector<Material>   materials;
    std::vector<Mesh>       meshes;
    std::vector<Model>      models;
    std::vector<Program>    programs;
    std::vector<Entity>     entities;

    // program indices
    u32 texturedGeometryProgramIdx;
    u32 texturedMeshProgramIdx;
    
    // texture indices
    u32 diceTexIdx;
    u32 whiteTexIdx;
    u32 blackTexIdx;
    u32 normalTexIdx;
    u32 magentaTexIdx;

    // Camera
    Camera camera;

    // Mode
    Mode mode;

    // Debug mode
    bool debug_group_mode;

    // Embedded geometry (in-editor simple meshes such as
    // a screen filling quad, a cube, a sphere...)
    GLuint embeddedVertices;
    GLuint embeddedElements;

    // Location of the texture uniform in the textured quad shader
    GLuint programUniformTexture;

    // More Uniforms
    GLint texturedMeshProgram_uTexture;

    // VAO object to link our screen filling quad with our textured quad shader
    GLuint vao;

    // Model indices
    u32 patrick_index;

    // Uniform buffer
    GLuint uniform_buffer_handle;
    GLint max_uniform_buffer_size;
    GLint uniform_block_alignment;
};

void Init(App* app);

void Gui(App* app);

void Update(App* app);

void Render(App* app);

GLuint FindVao(Mesh& mesh, u32 submesh_index, const Program& program);

u32 LoadTexture2D(App* app, const char* filepath);
