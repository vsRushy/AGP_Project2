//
// engine.cpp : Put all your graphics stuff in this file. This is kind of the graphics module.
// In here, you should type all your OpenGL commands, and you can also type code to handle
// input platform events (e.g to move the camera or react to certain shortcuts), writing some
// graphics related GUI options, and so on.
//

#include "engine.h"
#include <imgui.h>
#include <stb_image.h>
#include <stb_image_write.h>

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
    else
    {
        GLint attributeCount;
        GLchar attribute_name[32];
        GLsizei attribute_length;
        GLint attribute_size;
        GLenum attribute_type;

        glGetProgramiv(programHandle, GL_ACTIVE_ATTRIBUTES, &attributeCount);

        for (int i = 0; i < attributeCount; ++i)
        {
            glGetActiveAttrib(programHandle, i, ARRAY_COUNT(attribute_name), &attribute_length, &attribute_size, &attribute_type, attribute_name);
            printf("Attribute #%d Type: %u Name: %s\n", i, attribute_type, attribute_name);
        }

        // With the previous code, we should be allowed to call attribute_location = glGetAttribLocation(programHandle, "attribute name"); from anywhere
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

void Init(App* app)
{
    app->opengl_info.version = glGetString(GL_VERSION);
    app->opengl_info.renderer = glGetString(GL_RENDERER);
    app->opengl_info.vendor = glGetString(GL_VENDOR);
    app->opengl_info.glsl_version = glGetString(GL_SHADING_LANGUAGE_VERSION);
    // add opengl extensions

    app->debug_group_mode = true;

    // TODO: Initialize your resources here!
    // - vertex buffers
    // - element/index buffers
    // - vaos
    // - programs (and retrieve uniform indices)
    // - textures

    VertexBufferLayout vertex_buffer_layout = {};
    vertex_buffer_layout.attributes.push_back(VertexBufferAttribute{ 0, 3, 0 });
    vertex_buffer_layout.attributes.push_back(VertexBufferAttribute{ 2, 2, 3 * sizeof(float) });
    vertex_buffer_layout.stride = 5 * sizeof(float);

    


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

    app->texturedMeshProgramIdx = LoadProgram(app, "shaders.glsl", "SHOW_TEXTURED_MESH");
    Program& texturedMeshProgram = app->programs[app->texturedGeometryProgramIdx];
    texturedMeshProgram.vertex_input_layout.attributes.push_back({ 0, 3 });
    texturedMeshProgram.vertex_input_layout.attributes.push_back({ 2, 2 });
    app->texturedMeshProgram_uTexture = glGetUniformLocation(texturedMeshProgram.handle, "uTexture");

    app->mode = Mode_Count;
}

void Gui(App* app)
{
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

    if(ImGui::Button("RenderDoc Capture")) 
    {
        rdoc_api->TriggerCapture();
    }

    ImGui::End();
}

void Update(App* app)
{
    // You can handle app->input keyboard/mouse here

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
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glViewport(0, 0, app->displaySize.x, app->displaySize.y);

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
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glViewport(0, 0, app->displaySize.x, app->displaySize.y);

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            Program& texturedMeshProgram = app->programs[app->texturedMeshProgramIdx];
            glUseProgram(texturedMeshProgram.handle);
            
            Model& model = app->models[0]; // TODO change 0
            Mesh& mesh = app->meshes[model.mesh_index];

            for (u32 i = 0; i < mesh.submeshes.size(); ++i)
            {
                GLuint vao = FindVao(mesh, i, texturedMeshProgram);
                glBindVertexArray(vao);

                u32 submesh_material_index = model.material_index[i];
                Material& submesh_material = app->materials[submesh_material_index];

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, app->textures[submesh_material.albedo_texture_index].handle);
                glUniform1i(app->texturedMeshProgram_uTexture, 0);
                
                Submesh& submesh = mesh.submeshes[i];
                glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.index_offset);
            }

            glUniform1i(app->programUniformTexture, 0);
            glActiveTexture(GL_TEXTURE0);
            GLuint textureHandle = app->textures[app->diceTexIdx].handle;
            glBindTexture(GL_TEXTURE_2D, textureHandle);

            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

            glBindVertexArray(0);
            glUseProgram(0);
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
