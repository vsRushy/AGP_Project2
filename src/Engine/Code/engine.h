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
    vec3 front;
    vec3 up;
    vec3 right;
    vec3 world_up;

    float yaw;
    float pitch;

    float fov = 65.0f;
    float aspect_ratio;
    float near_plane = 0.1f;
    float far_plane = 1000.0f;

    float speed;
    float mouse_sensitivity;

    enum class MOVE { FORWARD, BACK, LEFT, RIGHT, UP, DOWN };

    Camera() {}

    Camera(const vec3& pos, const float& fv = 60.0f, const float& znear = 0.1f, const float& zfar = 1000.0f)
    {
        position = pos;
        front = vec3(0.0f, 0.0f, -1.0f);
        up = vec3(0.0f, 1.0f, 0.0f);
        world_up = up;
        right = glm::cross(front, world_up);

        yaw = -90.0f;
        pitch = 0.0f;

        fov = fv;
        near_plane = znear;
        far_plane = zfar;

        speed = 0.25f;
        mouse_sensitivity = 0.25f;
    }

    void SetAspectRatio(const float& display_size_x, const float& display_size_y)
    {
        aspect_ratio = display_size_x / display_size_y;
    }

    glm::mat4 GetViewMatrix()
    {
        return glm::lookAt(position, position + front, up);
    }

    glm::mat4 GetProjectionMatrix()
    {
        return glm::perspective(glm::radians(fov), aspect_ratio, near_plane, far_plane);
    }

    void Move(const MOVE& movement)
    {
        switch (movement)
        {
        case MOVE::FORWARD:    position += front * speed;    break;
        case MOVE::BACK:       position -= front * speed;    break;
        case MOVE::LEFT:       position -= right * speed;    break;
        case MOVE::RIGHT:      position += right * speed;    break;
        case MOVE::UP:         position += up * speed;       break;
        case MOVE::DOWN:       position -= up * speed;       break;

        default: break;
        }
    }

    void Rotate(float xoffset, float yoffset)
    {
        xoffset *= mouse_sensitivity;
        yoffset *= mouse_sensitivity;

        yaw += xoffset;
        pitch += yoffset;

        if (true)
        {
            if (pitch > 89.0f) pitch = 89.0f;
            if (pitch < -89.0f) pitch = -89.0f;
        }

        UpdateCameraValues();
    }

    void Zoom(const float& yoffset)
    {
        fov -= yoffset * 2.0f;

        if (fov < 2.0f) fov = 2.0f;
        if (fov > 178.0f) fov = 178.0f;
    }

    void UpdateCameraValues()
    {
        glm::vec3 f;
        f.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        f.y = sin(glm::radians(pitch));
        f.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(f);

        right = glm::normalize(glm::cross(front, world_up));
        up = glm::normalize(glm::cross(right, front));
    }
};

struct Entity
{
    glm::mat4 worldMatrix;
    u32 modelIndex;
    u32 localParamsOffset;
    u32 localParamsSize;
};

enum LightType
{
    LightType_Directional,
    LightType_Point
};

struct Light
{
    LightType type;
    vec3 color;
    vec3 direction;
    vec3 position;
    float radius;
    float intensity;
};

struct Buffer
{
    GLuint handle;
    GLenum type;
    u32 size;
    u32 head;
    void* data;
};

enum Mode
{
    Mode_TexturedQuad,
    Mode_Count,
    Mode_Deferred,
};

enum class FboAttachmentType
{
    Position,
    Normals,
    Diffuse,
    Depth,

    FinalRender, // Used only in the lighting pass FBO
};

struct App
{
    // Loop
    f32  deltaTime;
    f32  timeSinceStartup;
    bool isRunning;

    // Input
    Input input;

    // Graphics
    OpenGLInfo opengl_info;

    bool focused = false;

    ivec2 displaySize;

    std::vector<Texture>    textures;
    std::vector<Material>   materials;
    std::vector<Mesh>       meshes;
    std::vector<Model>      models;
    std::vector<Program>    programs;
    std::vector<Entity>     entities;
    std::vector<Light>      lights;

    // program indices
    u32 texturedGeometryProgramIdx;

    u32 texturedMeshProgramIdx;
    u32 texturedMeshWithClippingProgramIdx;
    u32 waterMeshProgramIdx;

    u32 deferredGeometryPassProgramIdx;
    u32 deferredLightingPassProgramIdx;
    u32 deferredLightProgramIdx;

    u32 skyboxProgramIdx;
    
    // texture indices
    u32 diceTexIdx;
    u32 whiteTexIdx;
    u32 blackTexIdx;
    u32 normalTexIdx;
    u32 magentaTexIdx;

    u32 dudvMapIdx;

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
    GLint texturedMeshProgram_uTexture; // Forward rendering mesh texture
    GLint texturedMeshProgram_uSkybox; // Forward rendering skybox

    GLint texturedMeshWithClippingProgram_uTexture;
    GLint texturedMeshWithClippingProgram_uSkybox;
    GLint texturedMeshWithClippingProgram_uProjection;
    GLint texturedMeshWithClippingProgram_uView;
    GLint texturedMeshWithClippingProgram_uModel;
    GLint texturedMeshWithClippingProgram_uClippingPlane;

    GLint waterMeshProgram_uProjection;
    GLint waterMeshProgram_uView;
    GLint waterMeshProgram_uModel;

    GLint waterMeshProgram_uReflectionTexture;
    GLint waterMeshProgram_uRefractionTexture;

    GLint waterMeshProgram_uDudvMap;

    GLint waterMeshProgram_uMoveFactor;

    GLint deferredGeometryProgram_uTexture; // Deferred geometry pass

    GLint deferredLightingProgram_uGPosition; // Lighting geometry pass
    GLint deferredLightingProgram_uGNormals; // Lighting geometry pass
    GLint deferredLightingProgram_uGDiffuse; // Lighting geometry pass

    GLint deferredLightProgram_uProjection; // Projection matrix for deferred shading light
    GLint deferredLightProgram_uView; // View matrix for deferred shading light
    GLint deferredLightProgram_uModel; // Model matrix for deferred shading light
    GLint deferredLightProgram_uLightColor; // Light volume for deferred shading

    GLint skyboxProgram_uProjection;
    GLint skyboxProgram_uView;
    GLint skyboxProgram_uSkybox;

    // VAO object to link our screen filling quad with our textured quad shader
    GLuint vao;

    // Model indices
    u32 patrick_index;
    u32 cube_index;

    // Framebuffer
    GLuint forwardFrameBuffer;
    GLuint renderAttachmentHandle;
    GLuint forwardDepthAttachmentHandle;

    GLuint waterReflectionFrameBuffer;
    GLuint waterReflectionColorAttachment;
    GLuint waterReflectionDepthAttachment;

    GLuint waterRefractionFrameBuffer;
    GLuint waterRefractionColorAttachment;
    GLuint waterRefractionDepthAttachment;

    GLuint gBuffer; // Used at geometry pass
    GLuint positionAttachmentHandle;
    GLuint normalsAttachmentHandle;
    GLuint diffuseAttachmentHandle;
    GLuint depthAttachmentHandle;

    GLuint fBuffer; // Used at lighting pass
    GLuint finalRenderAttachmentHandle;

    FboAttachmentType currentFboAttachment;

    // Buffer
    Buffer cbuffer;
    
    u32 globalParamsOffset;
    u32 globalParamsSize;

    glm::mat4 view;
    glm::mat4 projection;

    // Uniform buffer
    GLint max_uniform_buffer_size;
    GLint uniform_block_alignment;

    // Screen quad
    GLuint quad_vao = 0u;
    u32 quad_index_count;

    void LoadQuad();
    void RenderQuad(const GLuint& vao, const u32& index_count);

    // Sphere
    GLuint sphere_vao = 0u;
    u32 sphere_index_count;

    void LoadSphere();
    void RenderSphere(const GLuint& vao, const u32& index_count);

    // Cubemap
    GLuint cubemap;
    GLuint LoadCubemap(const std::vector<std::string>& faces);
    GLuint skybox_vao, skybox_vbo;
};

void Init(App* app);

void Gui(App* app);

void Update(App* app);

void Render(App* app);

GLuint FindVao(Mesh& mesh, u32 submesh_index, const Program& program);

u32 LoadTexture2D(App* app, const char* filepath);
