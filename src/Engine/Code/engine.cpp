//
// engine.cpp : Put all your graphics stuff in this file. This is kind of the graphics module.
// In here, you should type all your OpenGL commands, and you can also type code to handle
// input platform events (e.g to move the camera or react to certain shortcuts), writing some
// graphics related GUI options, and so on.
//

#include <imgui.h>
#include <stb_image.h>
#include <stb_image_write.h>

#include "engine.h"
#include "assimp_model_loading.h"
#include "buffer_management.h"

#define BINDING(b) b

GLuint CreateProgramFromSource(String programSource, const char* shaderName)
{
    GLchar  infoLogBuffer[1024] = {};
    GLsizei infoLogBufferSize = sizeof(infoLogBuffer);
    GLsizei infoLogSize;
    GLint   success;

    char versionString[] = "#version 430\n";
    char shaderNameDefine[128];
    sprintf_s(shaderNameDefine, "#define %s\n", shaderName);
    char vertexShaderDefine[] = "#define VERTEX\n";
    char fragmentShaderDefine[] = "#define FRAGMENT\n";

    const GLchar* vertexShaderSource[] = {
        versionString,
        shaderNameDefine,
        vertexShaderDefine,
        programSource.str
    };
    const GLint vertexShaderLengths[] = {
        (GLint) strlen(versionString),
        (GLint) strlen(shaderNameDefine),
        (GLint) strlen(vertexShaderDefine),
        (GLint) programSource.len
    };
    const GLchar* fragmentShaderSource[] = {
        versionString,
        shaderNameDefine,
        fragmentShaderDefine,
        programSource.str
    };
    const GLint fragmentShaderLengths[] = {
        (GLint) strlen(versionString),
        (GLint) strlen(shaderNameDefine),
        (GLint) strlen(fragmentShaderDefine),
        (GLint) programSource.len
    };

    GLuint vshader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vshader, ARRAY_COUNT(vertexShaderSource), vertexShaderSource, vertexShaderLengths);
    glCompileShader(vshader);
    glGetShaderiv(vshader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vshader, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glCompileShader() failed with vertex shader %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
    }

    GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fshader, ARRAY_COUNT(fragmentShaderSource), fragmentShaderSource, fragmentShaderLengths);
    glCompileShader(fshader);
    glGetShaderiv(fshader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fshader, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glCompileShader() failed with fragment shader %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
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
    GLenum dataFormat     = GL_RGB;
    GLenum dataType       = GL_UNSIGNED_BYTE;

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

u8 GetAttributeComponentCount(const GLenum& type)
{
    switch (type)
    {
    case GL_FLOAT: return 1; break; case GL_FLOAT_VEC2: return 2; break; case GL_FLOAT_VEC3: return 3; break; case GL_FLOAT_VEC4: return 4; break;
    case GL_FLOAT_MAT2: return 4; break; case GL_FLOAT_MAT3: return 9; break; case GL_FLOAT_MAT4: return 16;
    case GL_FLOAT_MAT2x3: return 6; break; case GL_FLOAT_MAT2x4: return 8; break;
    case GL_FLOAT_MAT3x2: return 6; break; case GL_FLOAT_MAT3x4: return 12; break;
    case GL_FLOAT_MAT4x2: return 8; break; case GL_FLOAT_MAT4x3: return 12; break;
    case GL_INT: return 1; break; case GL_INT_VEC2: return 2; break; case GL_INT_VEC3: return 3; break; case GL_INT_VEC4: return 4; break;
    case GL_UNSIGNED_INT: return 1; break; case GL_UNSIGNED_INT_VEC2: return 2; break; case GL_UNSIGNED_INT_VEC3: return 3; break; case GL_UNSIGNED_INT_VEC4: return 4; break;
    case GL_DOUBLE: return 1; break; case GL_DOUBLE_VEC2: return 2; break; case GL_DOUBLE_VEC3: return 3; break; case GL_DOUBLE_VEC4: return 4; break;
    case GL_DOUBLE_MAT2: return 4; break; case GL_DOUBLE_MAT3: return 9; break; case GL_DOUBLE_MAT4: return 16;
    case GL_DOUBLE_MAT2x3: return 6; break; case GL_DOUBLE_MAT2x4: return 8; break;
    case GL_DOUBLE_MAT3x2: return 6; break; case GL_DOUBLE_MAT3x4: return 12; break;
    case GL_DOUBLE_MAT4x2: return 8; break; case GL_DOUBLE_MAT4x3: return 12; break;

    default: return 0; break;
    }
}

u32 Align(const u32& value, const u32& alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

glm::mat4 TransformPosition(const vec3& position)
{
    glm::mat4 transform = glm::translate(position);

    return transform;
}

glm::mat4 TransformRotation(const float& angle, const vec3& axis)
{
    glm::mat4 transform = glm::mat4(1.0f);
    transform = glm::rotate(transform, glm::radians(angle), axis);

    return transform;
}

glm::mat4 TransformScale(const vec3& scale)
{
    glm::mat4 transform = glm::mat4(1.0f);
    transform = glm::scale(transform, scale);
    
    return transform;
}

glm::mat4 TransformPositionRotation(const vec3& position, const float& angle, const vec3& axis)
{
    glm::mat4 transform = glm::translate(position);
    transform = glm::rotate(transform, glm::radians(angle), axis);

    return transform;
}

glm::mat4 TransformPositionScale(const vec3& position, const vec3& scale)
{
    glm::mat4 transform = glm::translate(position);
    transform = glm::scale(transform, scale);

    return transform;
}

glm::mat4 TransformPositionRotationScale(const vec3& position, const float& angle, const vec3& axis, const vec3& scale)
{
    glm::mat4 transform = glm::translate(position);
    transform = glm::rotate(transform, glm::radians(angle), axis);
    transform = glm::scale(transform, scale);

    return transform;
}

void Init(App* app)
{
    app->opengl_info.version = glGetString(GL_VERSION);
    app->opengl_info.renderer = glGetString(GL_RENDERER);
    app->opengl_info.vendor = glGetString(GL_VENDOR);
    app->opengl_info.glsl_version = glGetString(GL_SHADING_LANGUAGE_VERSION);
    // add opengl extensions

    glEnable(GL_DEPTH_TEST);

    app->debug_group_mode = true;

    // Camera
    app->camera = Camera(vec3(0.0f));

    // TODO: Initialize your resources here!
    // - vertex buffers
    // - element/index buffers
    // - vaos
    // - programs (and retrieve uniform indices)
    // - textures
    const VertexV3V2 vertices[] = {
        { vec3(-0.5f, -0.5f, 0.0f), vec2(0.0f, 0.0f) },
        { vec3( 0.5f, -0.5f, 0.0f), vec2(1.0f, 0.0f) },
        { vec3( 0.5f,  0.5f, 0.0f), vec2(1.0f, 1.0f) },
        { vec3(-0.5f,  0.5f, 0.0f), vec2(0.0f, 1.0f) }
    };

    const u16 indices[] = {
        0, 1, 2,
        0, 2, 3
    };

    glGenBuffers(1, &app->embeddedVertices);
    glBindBuffer(GL_ARRAY_BUFFER, app->embeddedVertices);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenBuffers(1, &app->embeddedElements);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedElements);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glGenVertexArrays(1, &app->vao);
    glBindVertexArray(app->vao);
    glBindBuffer(GL_ARRAY_BUFFER, app->embeddedVertices);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexV3V2), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexV3V2), (void*)12);
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedElements);
    glBindVertexArray(0);

    app->texturedGeometryProgramIdx = LoadProgram(app, "shaders.glsl", "TEXTURED_GEOMETRY");
    Program& texturedGeometryProgram = app->programs[app->texturedGeometryProgramIdx];
    app->programUniformTexture = glGetUniformLocation(texturedGeometryProgram.handle, "uTexture");

    app->diceTexIdx = LoadTexture2D(app, "dice.png");
    app->whiteTexIdx = LoadTexture2D(app, "color_white.png");
    app->blackTexIdx = LoadTexture2D(app, "color_black.png");
    app->normalTexIdx = LoadTexture2D(app, "color_normal.png");
    app->magentaTexIdx = LoadTexture2D(app, "color_magenta.png");

    // --------------------------------

    app->patrick_index = LoadModel(app, "Patrick/Patrick.obj");

    app->LoadSphere();

    app->entities.push_back({ TransformPositionRotationScale(vec3(0.0f, 0.0f, -20.0f), 60.0f, vec3(0.0f, 1.0f, 0.0f), vec3(2.0f)),
                              app->patrick_index });
    app->entities.push_back({ TransformPositionRotationScale(vec3(-5.0f, 0.0f, -20.0f), 60.0f, vec3(0.0f, 1.0f, 0.0f), vec3(2.0f)),
                              app->patrick_index });
    app->entities.push_back({ TransformPositionRotationScale(vec3(5.0f, 0.0f, -20.0f), 60.0f, vec3(0.0f, 1.0f, 0.0f), vec3(2.0f)),
                              app->patrick_index });

    app->lights.push_back({ LightType_Point, vec3(1.0f, 0.0f, 0.0f), vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 5.0f, -20.0f), 20.0f, 1.0f });
    app->lights.push_back({ LightType_Point, vec3(0.0f, 1.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), vec3(5.0f, 7.0f, 0.0f), 14.0f, 0.7f });
    app->lights.push_back({ LightType_Directional, vec3(1.0f, 1.0f, 1.0f), vec3(0.0f, 0.0f, 1.0f), vec3(0.0f, 10.0f, -3.0f), 0.0f, 1.0f });

    /*int elements_j_patricks = 6, elements_i_patricks = 6;
    for (int j = -elements_j_patricks / 2; j <= elements_j_patricks / 2; ++j)
    {
        for (int i = -elements_i_patricks / 2; i <= elements_i_patricks / 2; ++i)
        {
            app->entities.push_back({ TransformPositionRotationScale(vec3(i * 30, 0.0f, j * 30),
                60.0f, vec3(0.0f, 1.0f, 0.0f), vec3(2.0f)), app->patrick_index });
        }
    }*/

    /*int elements_j_lights = 6, elements_i_lights = 6;
    for (int j = -elements_j_lights / 2; j <= elements_j_lights / 2; ++j)
    {
        for (int i = -elements_i_lights / 2; i <= elements_i_lights / 2; ++i)
        {
            app->lights.push_back({ LightType_Point, vec3(GenerateRandomFloat(0.0f, 1.0f), GenerateRandomFloat(0.0f, 0.75f), GenerateRandomFloat(0.0f, 0.5f)),
                vec3(1.0f, 0.0f, 0.0f), vec3(i * 30 + 10, 3.0f, j * 30), 50.0, 1.0, true });
        }
    }*/

    /*
    app->lights.push_back({ LightType_Point, vec3(1.0f, 0.0f, 0.0f), vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 20.0f, 0.0f), 50.0, 1.0, true });
    app->lights.push_back({ LightType_Point, vec3(0.0f, 1.0f, 0.0f), vec3(1.0f, 0.0f, 0.0f), vec3(-50.0f, 5.0f, 50.0f), 70.0, 0.6, true });
    app->lights.push_back({ LightType_Point, vec3(0.0f, 0.0f, 1.0f), vec3(1.0f, 0.0f, 0.0f), vec3(50.0f, 7.0f, -50.0f), 50.0, 0.75, true });
    app->lights.push_back({ LightType_Point, vec3(1.0f, 0.0f, 1.0f), vec3(1.0f, 0.0f, 0.0f), vec3(50.0f, 5.0f, 50.0f), 50.0, 0.44, true });
    */

    /*
    app->lights.push_back({ LightType_Directional, vec3(1.0f, 1.0f, 1.0f), vec3(0.0f, 0.0f, 1.0f), vec3(0.0f, 45.0f, -50.0f), 0.0f, 0.2f, true });
    app->lights.push_back({ LightType_Directional, vec3(0.9f, 0.7f, 0.5f), vec3(0.0f, 1.0f, 0.0f), vec3(50.0f, 40.0f, 20.0f), 0.0f, 0.2f, true });
    */

    /* FORWARD RENDERING SHADER */

    app->texturedMeshProgramIdx = LoadProgram(app, "shaders.glsl", "SHOW_TEXTURED_MESH");
    Program& texturedMeshProgram = app->programs[app->texturedMeshProgramIdx];

    GLint attribute_count;
    glGetProgramiv(texturedMeshProgram.handle, GL_ACTIVE_ATTRIBUTES, &attribute_count);

    for (int i = 0; i < attribute_count; ++i)
    {
        GLchar attribute_name[32];
        GLsizei attribute_length;
        GLint attribute_size;
        GLenum attribute_type;

        glGetActiveAttrib(texturedMeshProgram.handle, i, ARRAY_COUNT(attribute_name), &attribute_length, &attribute_size, &attribute_type, attribute_name);
        GLint attribute_location = glGetAttribLocation(texturedMeshProgram.handle, attribute_name);
        
        ELOG("Attribute %s. Location: %d Type: %d", attribute_name, attribute_location, attribute_type);
        
        texturedMeshProgram.vertex_input_layout.attributes.push_back({ (u8)attribute_location, GetAttributeComponentCount(attribute_type) });
    }

    app->texturedMeshProgram_uTexture = glGetUniformLocation(texturedMeshProgram.handle, "uTexture");
    app->texturedMeshProgram_uSkybox = glGetUniformLocation(texturedMeshProgram.handle, "uSkybox");

    app->texturedMeshWithClippingProgramIdx = LoadProgram(app, "shaders.glsl", "SHOW_TEXTURED_MESH_WITH_CLIPPING");
    Program& texturedMeshWithClippingProgram = app->programs[app->texturedMeshWithClippingProgramIdx];

    GLint attribute_clipping_count;
    glGetProgramiv(texturedMeshWithClippingProgram.handle, GL_ACTIVE_ATTRIBUTES, &attribute_clipping_count);

    for (int i = 0; i < attribute_clipping_count; ++i)
    {
        GLchar attribute_name[32];
        GLsizei attribute_length;
        GLint attribute_size;
        GLenum attribute_type;

        glGetActiveAttrib(texturedMeshWithClippingProgram.handle, i, ARRAY_COUNT(attribute_name), &attribute_length, &attribute_size, &attribute_type, attribute_name);
        GLint attribute_location = glGetAttribLocation(texturedMeshWithClippingProgram.handle, attribute_name);

        ELOG("Attribute %s. Location: %d Type: %d", attribute_name, attribute_location, attribute_type);

        texturedMeshWithClippingProgram.vertex_input_layout.attributes.push_back({ (u8)attribute_location, GetAttributeComponentCount(attribute_type) });
    }

    app->texturedMeshWithClippingProgram_uTexture = glGetUniformLocation(texturedMeshWithClippingProgram.handle, "uTexture");
    app->texturedMeshWithClippingProgram_uSkybox = glGetUniformLocation(texturedMeshWithClippingProgram.handle, "uSkybox");
    app->texturedMeshWithClippingProgram_uProjection = glGetUniformLocation(texturedMeshWithClippingProgram.handle, "uProjection");
    app->texturedMeshWithClippingProgram_uView = glGetUniformLocation(texturedMeshWithClippingProgram.handle, "uView");
    app->texturedMeshWithClippingProgram_uModel = glGetUniformLocation(texturedMeshWithClippingProgram.handle, "uModel");

    app->waterMeshProgramIdx = LoadProgram(app, "shaders.glsl", "WATER_MESH");
    Program& waterMeshProgram = app->programs[app->waterMeshProgramIdx];

    GLint attribute_water_count;
    glGetProgramiv(waterMeshProgram.handle, GL_ACTIVE_ATTRIBUTES, &attribute_water_count);

    for (int i = 0; i < attribute_water_count; ++i)
    {
        GLchar attribute_name[32];
        GLsizei attribute_length;
        GLint attribute_size;
        GLenum attribute_type;

        glGetActiveAttrib(waterMeshProgram.handle, i, ARRAY_COUNT(attribute_name), &attribute_length, &attribute_size, &attribute_type, attribute_name);
        GLint attribute_location = glGetAttribLocation(waterMeshProgram.handle, attribute_name);

        ELOG("Attribute %s. Location: %d Type: %d", attribute_name, attribute_location, attribute_type);

        waterMeshProgram.vertex_input_layout.attributes.push_back({ (u8)attribute_location, GetAttributeComponentCount(attribute_type) });
    }

    app->waterMeshProgram_uProjection = glGetUniformLocation(waterMeshProgram.handle, "uProjection");
    app->waterMeshProgram_uView = glGetUniformLocation(waterMeshProgram.handle, "uView");
    app->waterMeshProgram_uModel = glGetUniformLocation(waterMeshProgram.handle, "uModel");
    
    /* --------- */

    /* DEFERRED RENDERING SHADERS */

    app->deferredGeometryPassProgramIdx = LoadProgram(app, "shaders.glsl", "DEFERRED_GEOMETRY_PASS");
    Program& deferredGeometryPassProgram = app->programs[app->deferredGeometryPassProgramIdx];

    GLint deferred_geometry_attribute_count;
    glGetProgramiv(deferredGeometryPassProgram.handle, GL_ACTIVE_ATTRIBUTES, &deferred_geometry_attribute_count);

    for (int i = 0; i < deferred_geometry_attribute_count; ++i)
    {
        GLchar attribute_name[32];
        GLsizei attribute_length;
        GLint attribute_size;
        GLenum attribute_type;

        glGetActiveAttrib(deferredGeometryPassProgram.handle, i, ARRAY_COUNT(attribute_name), &attribute_length, &attribute_size, &attribute_type, attribute_name);
        GLint attribute_location = glGetAttribLocation(deferredGeometryPassProgram.handle, attribute_name);

        ELOG("Attribute %s. Location: %d Type: %d", attribute_name, attribute_location, attribute_type);

        deferredGeometryPassProgram.vertex_input_layout.attributes.push_back({ (u8)attribute_location, GetAttributeComponentCount(attribute_type) });
    }

    app->deferredGeometryProgram_uTexture = glGetUniformLocation(deferredGeometryPassProgram.handle, "uTexture");

    app->deferredLightingPassProgramIdx = LoadProgram(app, "shaders.glsl", "DEFERRED_LIGHTING_PASS");
    Program& deferredLightingPassProgram = app->programs[app->deferredLightingPassProgramIdx];

    GLint deferred_lighting_attribute_count;
    glGetProgramiv(deferredLightingPassProgram.handle, GL_ACTIVE_ATTRIBUTES, &deferred_lighting_attribute_count);

    for (int i = 0; i < deferred_lighting_attribute_count; ++i)
    {
        GLchar attribute_name[32];
        GLsizei attribute_length;
        GLint attribute_size;
        GLenum attribute_type;

        glGetActiveAttrib(deferredLightingPassProgram.handle, i, ARRAY_COUNT(attribute_name), &attribute_length, &attribute_size, &attribute_type, attribute_name);
        GLint attribute_location = glGetAttribLocation(deferredLightingPassProgram.handle, attribute_name);

        ELOG("Attribute %s. Location: %d Type: %d", attribute_name, attribute_location, attribute_type);

        deferredLightingPassProgram.vertex_input_layout.attributes.push_back({ (u8)attribute_location, GetAttributeComponentCount(attribute_type) });
    }

    app->deferredLightingProgram_uGPosition = glGetUniformLocation(deferredLightingPassProgram.handle, "uGPosition");
    app->deferredLightingProgram_uGNormals = glGetUniformLocation(deferredLightingPassProgram.handle, "uGNormals");
    app->deferredLightingProgram_uGDiffuse = glGetUniformLocation(deferredLightingPassProgram.handle, "uGDiffuse");

    app->deferredLightProgramIdx = LoadProgram(app, "shaders.glsl", "LIGHT_VOLUME");
    Program& deferredLightProgram = app->programs[app->deferredLightProgramIdx];

    GLint deferred_light_attribute_count;
    glGetProgramiv(deferredLightProgram.handle, GL_ACTIVE_ATTRIBUTES, &deferred_light_attribute_count);

    for (int i = 0; i < deferred_light_attribute_count; ++i)
    {
        GLchar attribute_name[32];
        GLsizei attribute_length;
        GLint attribute_size;
        GLenum attribute_type;

        glGetActiveAttrib(deferredLightProgram.handle, i, ARRAY_COUNT(attribute_name), &attribute_length, &attribute_size, &attribute_type, attribute_name);
        GLint attribute_location = glGetAttribLocation(deferredLightProgram.handle, attribute_name);

        ELOG("Attribute %s. Location: %d Type: %d", attribute_name, attribute_location, attribute_type);

        deferredLightProgram.vertex_input_layout.attributes.push_back({ (u8)attribute_location, GetAttributeComponentCount(attribute_type) });
    }

    app->deferredLightProgram_uProjection = glGetUniformLocation(deferredLightProgram.handle, "uProjection");
    app->deferredLightProgram_uView = glGetUniformLocation(deferredLightProgram.handle, "uView");
    app->deferredLightProgram_uModel = glGetUniformLocation(deferredLightProgram.handle, "uModel");
    app->deferredLightProgram_uLightColor = glGetUniformLocation(deferredLightProgram.handle, "uLightColor");

    app->skyboxProgramIdx = LoadProgram(app, "shaders.glsl", "SKYBOX");
    Program& skyboxProgram = app->programs[app->skyboxProgramIdx];

    GLint skybox_attribute_count;
    glGetProgramiv(skyboxProgram.handle, GL_ACTIVE_ATTRIBUTES, &skybox_attribute_count);

    for (int i = 0; i < skybox_attribute_count; ++i)
    {
        GLchar attribute_name[32];
        GLsizei attribute_length;
        GLint attribute_size;
        GLenum attribute_type;

        glGetActiveAttrib(skyboxProgram.handle, i, ARRAY_COUNT(attribute_name), &attribute_length, &attribute_size, &attribute_type, attribute_name);
        GLint attribute_location = glGetAttribLocation(skyboxProgram.handle, attribute_name);

        ELOG("Attribute %s. Location: %d Type: %d", attribute_name, attribute_location, attribute_type);

        skyboxProgram.vertex_input_layout.attributes.push_back({ (u8)attribute_location, GetAttributeComponentCount(attribute_type) });
    }

    app->skyboxProgram_uProjection = glGetUniformLocation(skyboxProgram.handle, "uProjection");
    app->skyboxProgram_uView = glGetUniformLocation(skyboxProgram.handle, "uView");
    app->skyboxProgram_uSkybox = glGetUniformLocation(skyboxProgram.handle, "uSkybox");

    /* --------- */

    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &app->max_uniform_buffer_size);
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &app->uniform_block_alignment);

    app->cbuffer = CreateConstantBuffer(app->max_uniform_buffer_size);

    // Framebuffer
    /* Forward */
    glGenTextures(1, &app->renderAttachmentHandle);
    glBindTexture(GL_TEXTURE_2D, app->renderAttachmentHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenTextures(1, &app->forwardDepthAttachmentHandle);
    glBindTexture(GL_TEXTURE_2D, app->forwardDepthAttachmentHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, app->displaySize.x, app->displaySize.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenFramebuffers(1, &app->forwardFrameBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, app->forwardFrameBuffer);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, app->renderAttachmentHandle, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, app->forwardDepthAttachmentHandle, 0);

    GLenum drawForwardBuffer[] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(ARRAY_COUNT(drawForwardBuffer), drawForwardBuffer);

    GLenum forwardFrameBufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (forwardFrameBufferStatus != GL_FRAMEBUFFER_COMPLETE)
    {
        switch (forwardFrameBufferStatus)
        {
        case GL_FRAMEBUFFER_UNDEFINED:                          ELOG("Framebuffer status error: GL_FRAMEBUFFER_UNDEFINED"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:              ELOG("Framebuffer status error: GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:      ELOG("Framebuffer status error: GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:             ELOG("Framebuffer status error: GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:             ELOG("Framebuffer status error: GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER"); break;
        case GL_FRAMEBUFFER_UNSUPPORTED:                        ELOG("Framebuffer status error: GL_FRAMEBUFFER_UNSUPPORTED"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:             ELOG("Framebuffer status error: GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:           ELOG("Framebuffer status error: GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS"); break;

        default: ELOG("Unknown framebuffer status error"); break;
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    /* Water reflection (only for forward rendering) */

    glGenTextures(1, &app->waterReflectionColorAttachment);
    glBindTexture(GL_TEXTURE_2D, app->waterReflectionColorAttachment);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenTextures(1, &app->waterReflectionDepthAttachment);
    glBindTexture(GL_TEXTURE_2D, app->waterReflectionDepthAttachment);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, app->displaySize.x, app->displaySize.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenFramebuffers(1, &app->waterReflectionFrameBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, app->waterReflectionFrameBuffer);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, app->waterReflectionColorAttachment, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, app->waterReflectionDepthAttachment, 0);

    GLenum waterReflectionBuffer[] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(ARRAY_COUNT(waterReflectionBuffer), waterReflectionBuffer);

    GLenum waterReflectionFrameBufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (waterReflectionFrameBufferStatus != GL_FRAMEBUFFER_COMPLETE)
    {
        switch (waterReflectionFrameBufferStatus)
        {
        case GL_FRAMEBUFFER_UNDEFINED:                          ELOG("Framebuffer status error: GL_FRAMEBUFFER_UNDEFINED"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:              ELOG("Framebuffer status error: GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:      ELOG("Framebuffer status error: GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:             ELOG("Framebuffer status error: GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:             ELOG("Framebuffer status error: GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER"); break;
        case GL_FRAMEBUFFER_UNSUPPORTED:                        ELOG("Framebuffer status error: GL_FRAMEBUFFER_UNSUPPORTED"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:             ELOG("Framebuffer status error: GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:           ELOG("Framebuffer status error: GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS"); break;

        default: ELOG("Unknown framebuffer status error"); break;
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    /* Water refraction (only for forward rendering) */

    glGenTextures(1, &app->waterRefractionColorAttachment);
    glBindTexture(GL_TEXTURE_2D, app->waterRefractionColorAttachment);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenTextures(1, &app->waterRefractionDepthAttachment);
    glBindTexture(GL_TEXTURE_2D, app->waterRefractionDepthAttachment);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, app->displaySize.x, app->displaySize.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenFramebuffers(1, &app->waterRefractionFrameBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, app->waterRefractionFrameBuffer);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, app->waterRefractionColorAttachment, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, app->waterRefractionDepthAttachment, 0);

    GLenum waterRefractionBuffer[] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(ARRAY_COUNT(waterRefractionBuffer), waterRefractionBuffer);

    GLenum waterRefractionFrameBufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (waterRefractionFrameBufferStatus != GL_FRAMEBUFFER_COMPLETE)
    {
        switch (waterRefractionFrameBufferStatus)
        {
        case GL_FRAMEBUFFER_UNDEFINED:                          ELOG("Framebuffer status error: GL_FRAMEBUFFER_UNDEFINED"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:              ELOG("Framebuffer status error: GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:      ELOG("Framebuffer status error: GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:             ELOG("Framebuffer status error: GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:             ELOG("Framebuffer status error: GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER"); break;
        case GL_FRAMEBUFFER_UNSUPPORTED:                        ELOG("Framebuffer status error: GL_FRAMEBUFFER_UNSUPPORTED"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:             ELOG("Framebuffer status error: GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:           ELOG("Framebuffer status error: GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS"); break;

        default: ELOG("Unknown framebuffer status error"); break;
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    /* Deferred */
    app->currentFboAttachment = FboAttachmentType::FinalRender;

    /* Geometry pass framebuffer */

    glGenTextures(1, &app->positionAttachmentHandle);
    glBindTexture(GL_TEXTURE_2D, app->positionAttachmentHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenTextures(1, &app->normalsAttachmentHandle);
    glBindTexture(GL_TEXTURE_2D, app->normalsAttachmentHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenTextures(1, &app->diffuseAttachmentHandle);
    glBindTexture(GL_TEXTURE_2D, app->diffuseAttachmentHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenTextures(1, &app->depthAttachmentHandle);
    glBindTexture(GL_TEXTURE_2D, app->depthAttachmentHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, app->displaySize.x, app->displaySize.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenFramebuffers(1, &app->gBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, app->gBuffer);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, app->positionAttachmentHandle, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, app->normalsAttachmentHandle, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, app->diffuseAttachmentHandle, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, app->finalRenderAttachmentHandle, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, app->depthAttachmentHandle, 0);

    GLenum drawBuffersGBuffer[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(ARRAY_COUNT(drawBuffersGBuffer), drawBuffersGBuffer);

    GLenum frameBufferStatusGBuffer = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (frameBufferStatusGBuffer != GL_FRAMEBUFFER_COMPLETE)
    {
        switch (frameBufferStatusGBuffer)
        {
        case GL_FRAMEBUFFER_UNDEFINED:                          ELOG("Framebuffer status error: GL_FRAMEBUFFER_UNDEFINED"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:              ELOG("Framebuffer status error: GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:      ELOG("Framebuffer status error: GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:             ELOG("Framebuffer status error: GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:             ELOG("Framebuffer status error: GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER"); break;
        case GL_FRAMEBUFFER_UNSUPPORTED:                        ELOG("Framebuffer status error: GL_FRAMEBUFFER_UNSUPPORTED"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:             ELOG("Framebuffer status error: GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:           ELOG("Framebuffer status error: GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS"); break;

        default: ELOG("Unknown framebuffer status error"); break;
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    /* Lighting pass framebuffer */

    glGenTextures(1, &app->finalRenderAttachmentHandle);
    glBindTexture(GL_TEXTURE_2D, app->finalRenderAttachmentHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenFramebuffers(1, &app->fBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, app->fBuffer);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, app->finalRenderAttachmentHandle, 0);

    GLenum drawBuffersFBuffer[] = { GL_COLOR_ATTACHMENT3 };
    glDrawBuffers(ARRAY_COUNT(drawBuffersFBuffer), drawBuffersFBuffer);

    GLenum frameBufferStatusFBuffer = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (frameBufferStatusFBuffer != GL_FRAMEBUFFER_COMPLETE)
    {
        switch (frameBufferStatusFBuffer)
        {
        case GL_FRAMEBUFFER_UNDEFINED:                          ELOG("Framebuffer status error: GL_FRAMEBUFFER_UNDEFINED"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:              ELOG("Framebuffer status error: GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:      ELOG("Framebuffer status error: GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:             ELOG("Framebuffer status error: GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:             ELOG("Framebuffer status error: GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER"); break;
        case GL_FRAMEBUFFER_UNSUPPORTED:                        ELOG("Framebuffer status error: GL_FRAMEBUFFER_UNSUPPORTED"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:             ELOG("Framebuffer status error: GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:           ELOG("Framebuffer status error: GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS"); break;

        default: ELOG("Unknown framebuffer status error"); break;
        }
    }

    app->mode = Mode_Count;

    /* Cubemap */

    std::vector<std::string> faces = {
        "Cubemap/right.jpg",
        "Cubemap/left.jpg",
        "Cubemap/top.jpg",
        "Cubemap/bottom.jpg",
        "Cubemap/front.jpg",
        "Cubemap/back.jpg",
    };

    app->cubemap = app->LoadCubemap(faces);

    float skybox_vertices[] = {
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

    glGenVertexArrays(1, &app->skybox_vao);
    glGenBuffers(1, &app->skybox_vbo);
    glBindVertexArray(app->skybox_vao);
    glBindBuffer(GL_ARRAY_BUFFER, app->skybox_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skybox_vertices), &skybox_vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}

void Gui(App* app)
{
    static bool p_open = true;
    static bool opt_fullscreen = true;
    static bool opt_padding = false;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

    // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
    // because it would be confusing to have two docking targets within each others.
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    if (opt_fullscreen)
    {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->GetWorkPos());
        ImGui::SetNextWindowSize(viewport->GetWorkSize());
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    }
    else
    {
        dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
    }

    // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
    // and handle the pass-thru hole, so we ask Begin() to not render a background.
    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;

    // Important: note that we proceed even if Begin() returns false (aka window is collapsed).
    // This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
    // all active windows docked into it will lose their parent and become undocked.
    // We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
    // any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
    if (!opt_padding)
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace Demo", &p_open, window_flags);
    if (!opt_padding)
        ImGui::PopStyleVar();

    if (opt_fullscreen)
        ImGui::PopStyleVar(2);

    // DockSpace
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    }

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("Options"))
        {
            // Disabling fullscreen would allow the window to be moved to the front of other windows,
            // which we can't undo at the moment without finer window depth/z control.
            ImGui::MenuItem("Fullscreen", NULL, &opt_fullscreen);
            ImGui::MenuItem("Padding", NULL, &opt_padding);
            ImGui::Separator();

            if (ImGui::MenuItem("Flag: NoSplit", "", (dockspace_flags & ImGuiDockNodeFlags_NoSplit) != 0)) { dockspace_flags ^= ImGuiDockNodeFlags_NoSplit; }
            if (ImGui::MenuItem("Flag: NoResize", "", (dockspace_flags & ImGuiDockNodeFlags_NoResize) != 0)) { dockspace_flags ^= ImGuiDockNodeFlags_NoResize; }
            if (ImGui::MenuItem("Flag: NoDockingInCentralNode", "", (dockspace_flags & ImGuiDockNodeFlags_NoDockingInCentralNode) != 0)) { dockspace_flags ^= ImGuiDockNodeFlags_NoDockingInCentralNode; }
            if (ImGui::MenuItem("Flag: AutoHideTabBar", "", (dockspace_flags & ImGuiDockNodeFlags_AutoHideTabBar) != 0)) { dockspace_flags ^= ImGuiDockNodeFlags_AutoHideTabBar; }
            if (ImGui::MenuItem("Flag: PassthruCentralNode", "", (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode) != 0, opt_fullscreen)) { dockspace_flags ^= ImGuiDockNodeFlags_PassthruCentralNode; }
            ImGui::Separator();

            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }

    ImGui::Begin("Info");

    ImGui::Text("FPS: %f", 1.0f / app->deltaTime);

    ImGui::Separator();

    ImGui::Text("OpenGL version: %s", app->opengl_info.version);
    ImGui::Text("OpenGL renderer: %s", app->opengl_info.renderer);
    ImGui::Text("OpenGL vendor: %s", app->opengl_info.vendor);
    ImGui::Text("OpenGL GLSL version: %s", app->opengl_info.glsl_version);

    ImGui::Separator();

    ImGui::Checkbox("Enable Debug Group Mode", &app->debug_group_mode);

    ImGui::Separator();

    if (ImGui::Button("RenderDoc Capture"))
    {
        rdoc_api->TriggerCapture();
    }

    ImGui::Separator();

    static bool is_deferred;
    if (ImGui::Checkbox("Deferred", &is_deferred))
    {
        is_deferred ? app->mode = Mode_Deferred : app->mode = Mode_Count;
    }

    ImGui::Separator();

    ImGui::Image((ImTextureID)app->waterReflectionColorAttachment, ImVec2(100.0f, 100.0f), { 0, 1 }, { 1, 0 });

    ImGui::Separator();

    if (app->mode == Mode_Deferred)
    {
        const char* items[] = { "Position", "Normals", "Diffuse", "Depth", "Final Render" };
        static const char* current_item = items[4];
        if (ImGui::BeginCombo("##combo", current_item))
        {
            for (int n = 0; n < IM_ARRAYSIZE(items); n++)
            {
                bool is_selected = (current_item == items[n]);
                if (ImGui::Selectable(items[n], is_selected))
                    current_item = items[n];
                if (is_selected)
                {
                    ImGui::SetItemDefaultFocus();
                }

                if (strcmp(current_item, items[0]) == 0)
                    app->currentFboAttachment = FboAttachmentType::Position;
                if (strcmp(current_item, items[1]) == 0)
                    app->currentFboAttachment = FboAttachmentType::Normals;
                if (strcmp(current_item, items[2]) == 0)
                    app->currentFboAttachment = FboAttachmentType::Diffuse;
                if (strcmp(current_item, items[3]) == 0)
                    app->currentFboAttachment = FboAttachmentType::Depth;
                if (strcmp(current_item, items[4]) == 0)
                    app->currentFboAttachment = FboAttachmentType::FinalRender;
            }
            ImGui::EndCombo();
        }
        ImGui::Separator();

        
    }
    if (ImGui::TreeNode("Lights##2"))
    {
        for (int i = 0; i < app->lights.size(); ++i) {
            std::string type;
            if (app->lights[i].type == LightType::LightType_Directional) type = ("Directional Light " + std::to_string(i));
            else  type = ("Point Light " + std::to_string(i));

            if (ImGui::TreeNode(type.c_str()))
            {
                //Color
                float col1[3] = { app->lights[i].color.r, app->lights[i].color.g, app->lights[i].color.b };
                ImGui::ColorEdit3("Color", col1);
                app->lights[i].color.r = col1[0];
                app->lights[i].color.g = col1[1];
                app->lights[i].color.b = col1[2];

                ImGui::Spacing();

                //Intensity
                float f1 = app->lights[i].intensity;
                ImGui::DragFloat("Intensity", &f1, 0.01f, 0.0f, 0.0f, "%.06f");
                if (f1 < 0.0f)f1 = 0.0f;
                app->lights[i].intensity = f1;

                ImGui::Spacing();

                //position
                ImGui::DragFloat3("Position", (float*)&app->lights[i].position, 0.01f);

                ImGui::Spacing();

                //Direction
                ImGui::DragFloat3("Direction", (float*)&app->lights[i].direction, 0.01f);

                ImGui::Spacing();

                //Radius
                if (app->lights[i].type == LightType::LightType_Point) {
                    float f2 = app->lights[i].radius;
                    ImGui::DragFloat("Radius", &f2, 0.01f, 0.0f, 0.0f, "%.06f");
                    if (f2 < 0.0f)f2 = 0.0f;
                    app->lights[i].radius = f2;
                }

                ImGui::TreePop();
            }
        }

        ImGui::TreePop();
        ImGui::Separator();
    }

    ImGui::End();
    //------------------------------------------------------------------------------------------
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
    ImGui::Begin("Scene");
    ImVec2 size = ImGui::GetContentRegionAvail();
    
    GLuint currentAttachment = 0;
    switch (app->mode)
    {
    case Mode_Deferred:
    {
        switch (app->currentFboAttachment)
        {
            case FboAttachmentType::Position:
            {
                currentAttachment = app->positionAttachmentHandle;
            }
            break;

            case FboAttachmentType::Normals:
            {
                currentAttachment = app->normalsAttachmentHandle;
            }
            break;

            case FboAttachmentType::Diffuse:
            {
                currentAttachment = app->diffuseAttachmentHandle;
            }
            break;

            case FboAttachmentType::Depth:
            {
                currentAttachment = app->depthAttachmentHandle;
            }
            break;

            case FboAttachmentType::FinalRender:
            {
                currentAttachment = app->finalRenderAttachmentHandle;
            }
            break;

            default:
            {} break;
        }
    }
    break;

    case Mode_Count:
    {
        currentAttachment = app->renderAttachmentHandle;
    }
    break;

    default:
    {} break;

    }

    ImGui::Image((ImTextureID)currentAttachment, size, { 0, 1 }, { 1, 0 });
    
    app->focused = ImGui::IsWindowFocused();

    ImGui::End();
    ImGui::PopStyleVar();

    // End dockspace
    ImGui::End();
}

void Update(App* app)
{
    // TODO: Handle app->input keyboard/mouse here
    if (app->input.keys[K_W] == BUTTON_PRESSED)
    {
        app->camera.Move(Camera::MOVE::FORWARD);
    }
    if (app->input.keys[K_S] == BUTTON_PRESSED)
    {
        app->camera.Move(Camera::MOVE::BACK);
    }
    if (app->input.keys[K_A] == BUTTON_PRESSED)
    {
        app->camera.Move(Camera::MOVE::LEFT);
    }
    if (app->input.keys[K_D] == BUTTON_PRESSED)
    {
        app->camera.Move(Camera::MOVE::RIGHT);
    }
    if (app->input.keys[K_R] == BUTTON_PRESSED)
    {
        app->camera.Move(Camera::MOVE::UP);
    }
    if (app->input.keys[K_F] == BUTTON_PRESSED)
    {
        app->camera.Move(Camera::MOVE::DOWN);
    }

    if (app->input.mouseButtons[LEFT] == BUTTON_PRESSED)
    {
        float xoffset = app->input.mouseDelta.x;
        float yoffset = -app->input.mouseDelta.y;

        if (app->focused)
            app->camera.Rotate(xoffset, yoffset);
    }

    if (app->input.mouseButtons[LEFT] == BUTTON_RELEASE)
    {

    }

    app->camera.SetAspectRatio((float)app->displaySize.x, (float)app->displaySize.y);
    
    app->view = app->camera.GetViewMatrix();
    app->projection = app->camera.GetProjectionMatrix();

    glBindBuffer(GL_UNIFORM_BUFFER, app->cbuffer.handle);
    app->cbuffer.data = (u8*)glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
    app->cbuffer.head = 0;

    // Global parameters
    app->globalParamsOffset = app->cbuffer.head;

    PushVec3(app->cbuffer, app->camera.position);
    PushUInt(app->cbuffer, app->lights.size());

    for (u32 i = 0; i < app->lights.size(); ++i)
    {
        AlignHead(app->cbuffer, sizeof(vec4));

        Light& light = app->lights[i];

        PushUInt(app->cbuffer, light.type);
        PushVec3(app->cbuffer, light.color);
        PushVec3(app->cbuffer, light.direction);
        PushFloat(app->cbuffer, light.intensity);
        PushVec3(app->cbuffer, light.position);
        PushFloat(app->cbuffer, light.radius);
    }

    app->globalParamsSize = app->cbuffer.head - app->globalParamsOffset;

    // Local parameters
    for (u32 i = 0; i < app->entities.size(); ++i)
    {
        AlignHead(app->cbuffer, app->uniform_block_alignment);

        Entity& entity = app->entities[i];

        glm::mat4 world = entity.worldMatrix;
        glm::mat4 worldViewProjectionMatrix = app->projection * app->view * world;

        entity.localParamsOffset = app->cbuffer.head;

        PushMat4(app->cbuffer, world);
        PushMat4(app->cbuffer, worldViewProjectionMatrix);

        entity.localParamsSize = app->cbuffer.head - entity.localParamsOffset;
    }
    
    glUnmapBuffer(GL_UNIFORM_BUFFER);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // Check timestamp & reload
    for (u64 i = 0; i < app->programs.size(); ++i)
    {
        Program& program = app->programs[i];
        u64 currentTimestamp = GetFileLastWriteTimestamp(program.filepath.c_str());

        if (currentTimestamp > program.lastWriteTimestamp)
        {
            glDeleteProgram(program.handle);
            String programSource = ReadTextFile(program.filepath.c_str());
            const char* programName = program.programName.c_str();
            program.handle = CreateProgramFromSource(programSource, programName);
            program.lastWriteTimestamp = currentTimestamp;
        }
    }
}

void Render(App* app)
{
    if (app->debug_group_mode)
        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "Shaded Model");

    switch (app->mode)
    {
        case Mode_TexturedQuad:
        {
            // TODO: Draw your textured quad here!
            // - clear the framebuffer
            // - set the viewport
            // - set the blending state
            // - bind the texture into unit 0
            // - bind the program 
            //   (...and make its texture sample from unit 0)
            // - bind the vao
            // - glDrawElements() !!!
            glBindFramebuffer(GL_FRAMEBUFFER, app->gBuffer);

            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glViewport(0, 0, app->displaySize.x, app->displaySize.y);

            glEnable(GL_DEPTH_TEST);

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            Program& programTexturedGeometry = app->programs[app->texturedGeometryProgramIdx];
            glUseProgram(programTexturedGeometry.handle);
            glBindVertexArray(app->vao);

            glUniform1i(app->programUniformTexture, 0);
            glActiveTexture(GL_TEXTURE0);
            GLuint textureHandle = app->textures[app->diceTexIdx].handle;
            glBindTexture(GL_TEXTURE_2D, textureHandle);

            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

            glBindVertexArray(0);
            glUseProgram(0);
        }
        break;

        case Mode_Count:
        {
            /* Water reflection */
            glBindFramebuffer(GL_FRAMEBUFFER, app->waterReflectionFrameBuffer);

            GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0 };
            glDrawBuffers(ARRAY_COUNT(drawBuffers), drawBuffers);

            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glViewport(0, 0, app->displaySize.x, app->displaySize.y);

            glEnable(GL_DEPTH_TEST);

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            glEnable(GL_CLIP_DISTANCE0);

            Program& texturedMeshWithClippingProgram = app->programs[app->texturedMeshWithClippingProgramIdx];
            glUseProgram(texturedMeshWithClippingProgram.handle);

            Camera reflectionCamera = app->camera;
            reflectionCamera.position.y = -reflectionCamera.position.y;
            reflectionCamera.pitch = -reflectionCamera.pitch;

            glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(0), app->cbuffer.handle, app->globalParamsOffset, app->globalParamsSize);

            for (const Entity& entity : app->entities)
            {
                Model& model = app->models[entity.modelIndex];
                Mesh& mesh = app->meshes[model.mesh_index];

                glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(1), app->cbuffer.handle, entity.localParamsOffset, entity.localParamsSize);

                glUniformMatrix4fv(app->texturedMeshWithClippingProgram_uProjection, 1, GL_FALSE, &reflectionCamera.GetProjectionMatrix()[0][0]);
                glUniformMatrix4fv(app->texturedMeshWithClippingProgram_uView, 1, GL_FALSE, &reflectionCamera.GetViewMatrix()[0][0]);
                glUniformMatrix4fv(app->texturedMeshWithClippingProgram_uModel, 1, GL_FALSE, &entity.worldMatrix[0][0]);

                for (u32 i = 0; i < mesh.submeshes.size(); ++i)
                {
                    GLuint vao = FindVao(mesh, i, texturedMeshWithClippingProgram);
                    glBindVertexArray(vao);

                    u32 submesh_material_index = model.material_index[i];
                    Material& submesh_material = app->materials[submesh_material_index];

                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, app->textures[submesh_material.albedo_texture_index].handle);
                    glUniform1i(app->texturedMeshWithClippingProgram_uTexture, 0);

                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_CUBE_MAP, app->cubemap);
                    glUniform1i(app->texturedMeshWithClippingProgram_uSkybox, 1);

                    Submesh& submesh = mesh.submeshes[i];
                    glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.index_offset);

                    glBindVertexArray(0);
                }
            }

            glUseProgram(0);

            glDisable(GL_CLIP_DISTANCE0);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            /* Skybox */
            glBindFramebuffer(GL_FRAMEBUFFER, app->waterReflectionFrameBuffer);

            Program& skyboxProgram = app->programs[app->skyboxProgramIdx];
            glUseProgram(skyboxProgram.handle);

            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LEQUAL);
            glDepthMask(GL_FALSE);

            glDisable(GL_BLEND);

            glUniformMatrix4fv(app->skyboxProgram_uProjection, 1, GL_FALSE, &app->projection[0][0]);
            glm::mat4 view_no_translation = glm::mat4(glm::mat3(app->view)); // No translation
            glUniformMatrix4fv(app->skyboxProgram_uView, 1, GL_FALSE, &view_no_translation[0][0]);

            glUniform1i(app->skyboxProgram_uSkybox, 4);

            glBindVertexArray(app->skybox_vao);

            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_CUBE_MAP, app->cubemap);

            glDrawArrays(GL_TRIANGLES, 0, 36);

            glBindVertexArray(0);

            glDepthMask(GL_TRUE);
            glDepthFunc(GL_LESS);

            glUseProgram(0);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            /* Dafault */
            glBindFramebuffer(GL_FRAMEBUFFER, app->forwardFrameBuffer);

            //GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0 };
            glDrawBuffers(ARRAY_COUNT(drawBuffers), drawBuffers);

            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glViewport(0, 0, app->displaySize.x, app->displaySize.y);

            glEnable(GL_DEPTH_TEST);

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            Program& texturedMeshProgram = app->programs[app->texturedMeshProgramIdx];
            glUseProgram(texturedMeshProgram.handle);
            
            glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(0), app->cbuffer.handle, app->globalParamsOffset, app->globalParamsSize);

            for (const Entity& entity : app->entities)
            {
                Model& model = app->models[entity.modelIndex];
                Mesh& mesh = app->meshes[model.mesh_index];

                glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(1), app->cbuffer.handle, entity.localParamsOffset, entity.localParamsSize);

                for (u32 i = 0; i < mesh.submeshes.size(); ++i)
                {
                    GLuint vao = FindVao(mesh, i, texturedMeshProgram);
                    glBindVertexArray(vao);

                    u32 submesh_material_index = model.material_index[i];
                    Material& submesh_material = app->materials[submesh_material_index];
                    
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, app->textures[submesh_material.albedo_texture_index].handle);
                    glUniform1i(app->texturedMeshProgram_uTexture, 0);

                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_CUBE_MAP, app->cubemap);
                    glUniform1i(app->texturedMeshProgram_uSkybox, 1);

                    Submesh& submesh = mesh.submeshes[i];
                    glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.index_offset);
                    
                    glBindVertexArray(0);
                }
            }

            glUseProgram(0);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            /* Skybox */
            glBindFramebuffer(GL_FRAMEBUFFER, app->forwardFrameBuffer);

            //Program& skyboxProgram = app->programs[app->skyboxProgramIdx];
            glUseProgram(skyboxProgram.handle);

            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LEQUAL);
            glDepthMask(GL_FALSE);

            glDisable(GL_BLEND);

            glUniformMatrix4fv(app->skyboxProgram_uProjection, 1, GL_FALSE, &app->projection[0][0]);
            /*glm::mat4 */view_no_translation = glm::mat4(glm::mat3(app->view)); // No translation
            glUniformMatrix4fv(app->skyboxProgram_uView, 1, GL_FALSE, &view_no_translation[0][0]);

            glUniform1i(app->skyboxProgram_uSkybox, 4);

            glBindVertexArray(app->skybox_vao);

            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_CUBE_MAP, app->cubemap);

            glDrawArrays(GL_TRIANGLES, 0, 36);

            glBindVertexArray(0);

            glDepthMask(GL_TRUE);
            glDepthFunc(GL_LESS);

            glUseProgram(0);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            // Water

            Program& waterMeshProgram = app->programs[app->waterMeshProgramIdx];
            glUseProgram(waterMeshProgram.handle);

            glBindFramebuffer(GL_FRAMEBUFFER, app->forwardFrameBuffer);

            glUniformMatrix4fv(app->waterMeshProgram_uProjection, 1, GL_FALSE, &app->projection[0][0]);
            glUniformMatrix4fv(app->waterMeshProgram_uView, 1, GL_FALSE, &app->view[0][0]);

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, vec3(0.0f));
            model = glm::rotate(model, glm::radians(90.0f), vec3(1.0f, 0.0f, 0.0f));
            model = glm::scale(model, glm::vec3(100.0f));

            glUniformMatrix4fv(app->waterMeshProgram_uModel, 1, GL_FALSE, &model[0][0]);

            app->RenderQuad();

            glUseProgram(0);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
        break;

        case Mode_Deferred:
        {
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

            /* First pass (geometry) */

            glBindFramebuffer(GL_FRAMEBUFFER, app->gBuffer);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            GLenum drawBuffersGBuffer[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
            glDrawBuffers(ARRAY_COUNT(drawBuffersGBuffer), drawBuffersGBuffer);

            glViewport(0, 0, app->displaySize.x, app->displaySize.y);

            glEnable(GL_DEPTH_TEST);

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            glDepthMask(GL_TRUE);

            Program& deferredGeometryPassProgram = app->programs[app->deferredGeometryPassProgramIdx];
            glUseProgram(deferredGeometryPassProgram.handle);

            for (const Entity& entity : app->entities)
            {
                Model& model = app->models[entity.modelIndex];
                Mesh& mesh = app->meshes[model.mesh_index];

                glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(1), app->cbuffer.handle, entity.localParamsOffset, entity.localParamsSize);

                for (u32 i = 0; i < mesh.submeshes.size(); ++i)
                {
                    GLuint vao = FindVao(mesh, i, deferredGeometryPassProgram);
                    glBindVertexArray(vao);

                    u32 submesh_material_index = model.material_index[i];
                    Material& submesh_material = app->materials[submesh_material_index];

                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, app->textures[submesh_material.albedo_texture_index].handle);
                    glUniform1i(app->deferredGeometryProgram_uTexture, 0);

                    Submesh& submesh = mesh.submeshes[i];
                    glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.index_offset);

                    glBindVertexArray(0);
                }
            }

            glUseProgram(0);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            /* Second pass (lighting) */

            glBindFramebuffer(GL_FRAMEBUFFER, app->fBuffer);

            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            GLenum drawBuffersFBuffer[] = { GL_COLOR_ATTACHMENT3 };
            glDrawBuffers(ARRAY_COUNT(drawBuffersFBuffer), drawBuffersFBuffer);

            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE);

            //glDepthMask(GL_FALSE);

            Program& deferredLightingPassProgram = app->programs[app->deferredLightingPassProgramIdx];
            glUseProgram(deferredLightingPassProgram.handle);

            glUniform1i(app->deferredLightingProgram_uGPosition, 1);
            glUniform1i(app->deferredLightingProgram_uGNormals, 2);
            glUniform1i(app->deferredLightingProgram_uGDiffuse, 3);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, app->positionAttachmentHandle);

            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, app->normalsAttachmentHandle);

            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, app->diffuseAttachmentHandle);

            glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(0), app->cbuffer.handle, app->globalParamsOffset, app->globalParamsSize);
            
            app->RenderQuad();

            glBindFramebuffer(GL_READ_FRAMEBUFFER, app->gBuffer);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, app->fBuffer);

            glBlitFramebuffer(0, 0, app->displaySize.x, app->displaySize.x, 0, 0, app->displaySize.x, app->displaySize.x, GL_DEPTH_BUFFER_BIT, GL_NEAREST);

            glUseProgram(0);

            // Render lights
            Program& deferredLightProgram = app->programs[app->deferredLightProgramIdx];
            glUseProgram(deferredLightProgram.handle);

            glUniformMatrix4fv(app->deferredLightProgram_uProjection, 1, GL_FALSE, &app->projection[0][0]);
            glUniformMatrix4fv(app->deferredLightProgram_uView, 1, GL_FALSE, &app->view[0][0]);

            for (const Light& light : app->lights)
            {
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, light.position);
                model = glm::scale(model, glm::vec3(2.0f));

                glUniformMatrix4fv(app->deferredLightProgram_uModel, 1, GL_FALSE, &model[0][0]);
                glUniform3f(app->deferredLightProgram_uLightColor, light.color.r, light.color.g, light.color.b);

                switch (light.type)
                {
                case LightType_Point: app->RenderSphere(app->sphere_vao, app->index_count); break;
                case LightType_Directional: app->RenderQuad(); break;

                default: break;
                }
            }

            glUseProgram(0);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
        break;

        default:
        {} break;
    }

    if (app->debug_group_mode)
        glPopDebugGroup();
}

GLuint FindVao(Mesh& mesh, u32 submesh_index, const Program& program)
{
    Submesh& submesh = mesh.submeshes[submesh_index];

    // Try to find a vao for this submesh/program
    for (u32 i = 0; i < (u32)submesh.vaos.size(); ++i)
    {
        if (submesh.vaos[i].program_handle == program.handle)
        {
            return submesh.vaos[i].handle;
        }
    }

    GLuint vao_handle = 0;

    // Create a new VAO for this submesh/program
    {
        glGenVertexArrays(1, &vao_handle);
        glBindVertexArray(vao_handle);

        glBindBuffer(GL_ARRAY_BUFFER, mesh.vertex_buffer_handle);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.index_buffer_handle);

        // Link all vertex input attributes to attributes in the vertex buffer
        for (u32 i = 0; i < program.vertex_input_layout.attributes.size(); ++i)
        {
            bool attribute_was_linked = false;

            for (u32 j = 0; j < submesh.vertex_buffer_layout.attributes.size(); ++j)
            {
                if (program.vertex_input_layout.attributes[i].location == submesh.vertex_buffer_layout.attributes[j].location)
                {
                    const u32 index = submesh.vertex_buffer_layout.attributes[j].location;
                    const u32 ncomp = submesh.vertex_buffer_layout.attributes[j].component_count;
                    const u32 offset = submesh.vertex_buffer_layout.attributes[j].offset + submesh.vertex_offset;
                    const u32 stride = submesh.vertex_buffer_layout.stride;

                    glVertexAttribPointer(index, ncomp, GL_FLOAT, GL_FALSE, stride, (void*)(u64)offset);
                    glEnableVertexAttribArray(index);

                    attribute_was_linked = true;

                    break;
                }
            }

            assert(attribute_was_linked);
        }

        glBindVertexArray(0);
    }

    Vao vao = { vao_handle, program.handle };
    submesh.vaos.push_back(vao);

    return vao_handle;
}

void App::RenderQuad()
{
    if (quad_vao == 0)
    {
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        
        glGenVertexArrays(1, &quad_vao);
        glGenBuffers(1, &quad_vbo);

        glBindVertexArray(quad_vao);
        glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }

    glBindVertexArray(quad_vao);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindVertexArray(0);
}

void App::LoadSphere()
{
    glGenVertexArrays(1, &sphere_vao);

    unsigned int vbo, ebo;
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> uv;
    std::vector<glm::vec3> normals;
    std::vector<unsigned int> indices;

    const unsigned int X_SEGMENTS = 64;
    const unsigned int Y_SEGMENTS = 64;
    for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
    {
        for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
        {
            float xSegment = (float)x / (float)X_SEGMENTS;
            float ySegment = (float)y / (float)Y_SEGMENTS;
            float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
            float yPos = std::cos(ySegment * PI);
            float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

            positions.push_back(glm::vec3(xPos, yPos, zPos));
            uv.push_back(glm::vec2(xSegment, ySegment));
            normals.push_back(glm::vec3(xPos, yPos, zPos));
        }
    }

    bool oddRow = false;
    for (unsigned int y = 0; y < Y_SEGMENTS; ++y)
    {
        if (!oddRow)
        {
            for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
            {
                indices.push_back(y * (X_SEGMENTS + 1) + x);
                indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
            }
        }
        else
        {
            for (int x = X_SEGMENTS; x >= 0; --x)
            {
                indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                indices.push_back(y * (X_SEGMENTS + 1) + x);
            }
        }

        oddRow = !oddRow;
    }

    index_count = indices.size();

    std::vector<float> data;
    for (std::size_t i = 0; i < positions.size(); ++i)
    {
        data.push_back(positions[i].x);
        data.push_back(positions[i].y);
        data.push_back(positions[i].z);

        if (uv.size() > 0)
        {
            data.push_back(uv[i].x);
            data.push_back(uv[i].y);
        }

        if (normals.size() > 0)
        {
            data.push_back(normals[i].x);
            data.push_back(normals[i].y);
            data.push_back(normals[i].z);
        }
    }

    glBindVertexArray(sphere_vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
    float stride = (3 + 2 + 3) * sizeof(float);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(5 * sizeof(float)));
}

void App::RenderSphere(const GLuint& vao, const u32& index_count)
{
    glBindVertexArray(sphere_vao);

    glDrawElements(GL_TRIANGLE_STRIP, index_count, GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);
}

GLuint App::LoadCubemap(const std::vector<std::string>& faces)
{
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        stbi_set_flip_vertically_on_load(false);
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
            );

            stbi_image_free(data);
        }
        else
        {
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
