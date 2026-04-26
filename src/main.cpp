#include <cstdio>
#include <array>
#include <iostream>

#include "utility.h"
#include "thread_pool.h"

#include <GLFW/glfw3.h>
#include <glm/ext.hpp> // for matrix calculation

void WindowPositionCallback(GLFWwindow* wnd, int x, int y)
{
    GLState& state = *static_cast<GLState*>(glfwGetWindowUserPointer(wnd));

    state.curWndParams.pos[0] = x;
    state.curWndParams.pos[1] = y;
}

void WindowSizeCallback(GLFWwindow* wnd, int x, int y)
{
    GLState& state = *static_cast<GLState*>(glfwGetWindowUserPointer(wnd));
    state.curWndParams.size[0] = x;
    state.curWndParams.size[1] = y;
}

void MouseMoveCallback(GLFWwindow* wnd, double x, double y)
{
    GLState& state = *static_cast<GLState*>(glfwGetWindowUserPointer(wnd));

    if (state.cam.isLeftButtonPressed) {
        // how much we moved
        float deltaX = (float)(x - state.cam.lastMousePos.x);
        float deltaY = (float)(y - state.cam.lastMousePos.y);
        state.cam.lastMousePos = glm::dvec2(x, y); // update

        float sensitivity = 0.002f;
        float yawAngle = -deltaX * sensitivity;
        float pitchAngle = -deltaY * sensitivity;

        // Pitch Quaternion
        glm::vec3 right = state.cam.orientation * glm::vec3(1, 0, 0);
        glm::quat pitchQuaternion = glm::angleAxis(pitchAngle, right);

        // Yaw Quaternion
        glm::vec3 up = glm::vec3(0, 1, 0);
        glm::quat yawQuaternion = glm::angleAxis(yawAngle, up);

        state.cam.orientation = yawQuaternion * pitchQuaternion * state.cam.orientation;
        state.cam.orientation = glm::normalize(state.cam.orientation);
    }
}

void MouseButtonCallback(GLFWwindow* wnd, int button, int action, int)
{
    GLState& state = *static_cast<GLState*>(glfwGetWindowUserPointer(wnd));

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            state.cam.isLeftButtonPressed = true;
            double x, y;
            glfwGetCursorPos(wnd, &x, &y);
            state.cam.lastMousePos = glm::dvec2(x, y);
        }
        else if (action == GLFW_RELEASE) {
            state.cam.isLeftButtonPressed = false;
        }
    }
}

void MouseScrollCallback(GLFWwindow* wnd, double dx, double dy)
{
}

void FramebufferChangeCallback(GLFWwindow* wnd, int w, int h)
{
    GLState& state = *static_cast<GLState*>(glfwGetWindowUserPointer(wnd));
    state.curWndParams.fbSize[0] = w;
    state.curWndParams.fbSize[1] = h;
}

void KeyboardCallback(GLFWwindow* wnd, int key, int scancode, int action, int modifier)
{
    GLState& state = *static_cast<GLState*>(glfwGetWindowUserPointer(wnd));
    uint32_t mode = state.mode;

    if (action == GLFW_PRESS || action == GLFW_REPEAT) { // for holding down (https://www.glfw.org/docs/3.3/group__input.html)
        if (key == GLFW_KEY_KP_ADD)
            state.heightScale += 0.05f;

        if (key == GLFW_KEY_KP_SUBTRACT)
            state.heightScale -= 0.05f;
    }

    if(action != GLFW_RELEASE) return;

    if(key == GLFW_KEY_P) mode = (mode == 2) ? 0 : (mode + 1);
    if(key == GLFW_KEY_O) mode = (mode == 0) ? 2 : (mode - 1);

    // Wireframe Mode
    // https://stackoverflow.com/questions/137629/how-do-you-render-primitives-as-wireframes-in-opengl
    if(key == GLFW_KEY_L) {
        static bool wireframe = false;
        wireframe = !wireframe;
        glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
    }

    if(key == GLFW_KEY_K) {
        state.tesselationRate++;
        state.needsTerrainUpdate = true;
    }
    if(key == GLFW_KEY_J && state.tesselationRate > 1) {
        state.tesselationRate--;
        state.needsTerrainUpdate = true;
    }

    if(state.needsTerrainUpdate) { // for debugging, because sometimes I cant notice the change
        std::cout << "New Tessellation Rate: " << state.tesselationRate << std::endl;
    }

    // https://www.glfw.org/docs/3.3/window_guide.html#window_monitor
    if(key == GLFW_KEY_ENTER) {
        state.isFullscreen = !state.isFullscreen;

        if (state.isFullscreen) {
            GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
            const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);

            glfwSetWindowMonitor(wnd, primaryMonitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        }
        else        {
            glfwSetWindowMonitor(wnd, nullptr, 200, 200, 1280, 720, GLFW_DONT_CARE);
        }
    }

    state.mode = mode;
}


const glm::mat4 Mb( // column major order
    glm::vec4(-1.0f/6.0f,  3.0f/6.0f, -3.0f/6.0f, 1.0f/6.0f),
    glm::vec4( 3.0f/6.0f, -6.0f/6.0f,  0.0f/6.0f, 4.0f/6.0f),
    glm::vec4(-3.0f/6.0f,  3.0f/6.0f,  3.0f/6.0f, 1.0f/6.0f),
    glm::vec4( 1.0f/6.0f,  0.0f/6.0f,  0.0f/6.0f, 0.0f/6.0f)
);

void calculateBSplineSurfacePoint(float s, float t, const glm::mat4& G, float startX, float startZ, float step, Vertex& outV) {
    // [s^3, s^2, s, 1]
    glm::vec4 S(s*s*s, s*s, s, 1.0f);
    // [t^3, t^2, t, 1]
    glm::vec4 T(t*t*t, t*t, t, 1.0f);

    // Derivatives
    // [3s^2, 2s, 1, 0]
    glm::vec4 dS(3.0f*s*s, 2.0f*s, 1.0f, 0.0f);

    // [3t^2, 2t, 1, 0]
    glm::vec4 dT(3.0f*t*t, 2.0f*t, 1.0f, 0.0f);

    glm::mat4 C = Mb * G * glm::transpose(Mb);
    // P(s, t) = S * C * T

    float height = glm::dot(S, C * T);

    float worldScale = 30.0f; // 30 meter in real life
    float patchScale = step * worldScale;

    outV.position = glm::vec3((startX + s * step) * worldScale, height, (startZ + t * step) * worldScale);

    // dY/ds and dY/dt for normal
    float dy_ds = glm::dot(dS, C * T);
    float dy_dt = glm::dot(S, C * dT);

    glm::vec3 tangentS(patchScale, dy_ds, 0.0f);
    glm::vec3 tangentT(0.0f, dy_dt, patchScale);

    outV.normal = glm::normalize(glm::cross(tangentT, tangentS));
}

void generateTerrain(GLState &state, const GeoDataDTED &dted, ThreadPool &tp) {
    int step = 1;
    int numPatchesX = (dted.dimensions[0] - 4) / step + 1;
    int numPatchesZ = (dted.dimensions[1] - 4) / step + 1;
    int res = state.tesselationRate;

    // n x n grid
    int verticesPerPatch = (res + 1) * (res + 1); // res+1 because we need to include the last point at the end of the patch
    int indicesPerPatch = res * res * 2 * 3; // res ^ 2 squares, 2 triangles per square, 3 indices per triangle

    // allocation
    state.vertices.resize(numPatchesX * numPatchesZ * verticesPerPatch);
    state.indices.resize(numPatchesX * numPatchesZ * indicesPerPatch);

    tp.SubmitDetachedBlocks(numPatchesX, [&](uint32_t start, uint32_t end) {
        for (uint32_t i = start; i < end; i++) {
            for (int j = 0; j < numPatchesZ; j++) {
                int step_i = i * step;
                int step_j = j * step;

                // 16 control points
                glm::mat4 G;
                for (int r = 0; r < 4; ++r)
                    for (int c = 0; c < 4; ++c)
                        G[c][r] = dted(step_i+ r, step_j + c); // column major order so G[c][r] instead of G[r][c]

                int patchLinearIdx = i * numPatchesZ + j; // linear index of the patch in the grid
                int vBase = patchLinearIdx * verticesPerPatch;
                int iBase = patchLinearIdx * indicesPerPatch;

                for (int r = 0; r <= res; ++r) {
                    for (int c = 0; c <= res; ++c) {
                        float s = (float) r / res;
                        float t = (float) c / res;

                        // calculate vertex position and normal for the (r, c) vertex of the patch
                        int localVIdx = vBase + (r * (res + 1) + c);
                        calculateBSplineSurfacePoint(s, t, G, (float) step_i, (float) step_j, step, state.vertices[localVIdx]);
                    }
                }

                int count = 0;
                int vInRow = res + 1;

                // 2 triangles per square, iterate each square in the grid
                for (int r = 0; r < res; ++r) {
                    for (int c = 0; c < res; ++c) {
                        uint32_t b = vBase + r * vInRow + c; // base vertex index for the current square

                        // triangle 1
                        state.indices[iBase + count++] = b;
                        state.indices[iBase + count++] = b + vInRow;
                        state.indices[iBase + count++] = b + 1;

                        // triangle 2
                        state.indices[iBase + count++] = b + 1;
                        state.indices[iBase + count++] = b + vInRow;
                        state.indices[iBase + count++] = b + vInRow + 1;
                    }
                }
            }
        }
    });

    // wait all threads
    tp.Wait();
}

int main(int argc, const char* argv[])
{
    // if (argc < 2) {
    //     std::cerr << "No DTED file found in arguments." << std::endl;
    //     return -1;
    // }

    GLState state = GLState("Terrain Renderer", 1280, 720,
                            CallbackPointersGLFW());
    ShaderGL vShader = ShaderGL(ShaderGL::VERTEX, "shaders/generic.vert");
    ShaderGL fShader = ShaderGL(ShaderGL::FRAGMENT, "shaders/debug.frag");
    MeshGL mesh = MeshGL("meshes/tri.obj");
    TextureGL tex = TextureGL("textures/mixed_brick_wall_diff_1k.png",
                              TextureGL::LINEAR, TextureGL::REPEAT);
    // Set unchanged state(s)
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glFrontFace(GL_CCW);

    GeoDataDTED dted("geo/n36_e029_1arc_v3.dt2");

    ThreadPool tp({ .threadCount = std::thread::hardware_concurrency() });
    generateTerrain(state, dted, tp);

    // cam pos setup to start from center
    glm::vec3 bbMin( 1e9f,  1e9f,  1e9f);
    glm::vec3 bbMax(-1e9f, -1e9f, -1e9f);

    for (const auto& v : state.vertices) {
        bbMin = glm::min(bbMin, v.position);
        bbMax = glm::max(bbMax, v.position);
    }

    glm::vec3 center = 0.5f * (bbMin + bbMax);

    state.cam.pos = glm::vec3(center.x, bbMax.y + 1000.0f, center.z); // some offset to see the terrain from above

    GLuint terrainVAO, terrainVBO, terrainEBO;

    // ids
    glGenVertexArrays(1, &terrainVAO);
    glGenBuffers(1, &terrainVBO);
    glGenBuffers(1, &terrainEBO);

    // vao activation
    glBindVertexArray(terrainVAO);

    // load vertices into vbo
    glBindBuffer(GL_ARRAY_BUFFER, terrainVBO);
    glBufferData(GL_ARRAY_BUFFER, state.vertices.size() * sizeof(Vertex), state.vertices.data(), GL_STATIC_DRAW);

    // load indices to ebo
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, terrainEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, state.indices.size() * sizeof(uint32_t), state.indices.data(), GL_STATIC_DRAW);

    // telling shader that "Location 0: Pos, Location 1: Normal"
    // Position (layout(location = 0))
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);

    // Normal (layout(location = 1))
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);

    // HDR
    GLuint hdrFBO, colorBuffer, rboDepth;
    // To update later on
    int fbWidth = state.curWndParams.fbSize[0];
    int fbHeight = state.curWndParams.fbSize[1];
    int currentFBWidth = fbWidth;
    int currentFBHeight = fbHeight;

    glGenFramebuffers(1, &hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);

    glGenTextures(1, &colorBuffer);
    glBindTexture(GL_TEXTURE_2D, colorBuffer);

    // 32 bit color buffer
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, fbWidth, fbHeight, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // Mipmap
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBuffer, 0);

    // Depth
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);

    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, fbWidth, fbHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // =============== //
    //   RENDER LOOP   //
    // =============== //
    while(!glfwWindowShouldClose(state.window))
    {
        // Poll inputs from the OS via GLFW
        glfwPollEvents();

        if (state.needsTerrainUpdate) {
            generateTerrain(state, dted, tp);

            glBindBuffer(GL_ARRAY_BUFFER, terrainVBO);
            glBufferData(GL_ARRAY_BUFFER, state.vertices.size() * sizeof(Vertex), state.vertices.data(), GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, terrainEBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, state.indices.size() * sizeof(uint32_t), state.indices.data(), GL_STATIC_DRAW);

            state.needsTerrainUpdate = false;
            // for debugging, because sometimes I cant notice the change
            std::cout << "Terrain updated with rate: " << state.tesselationRate << std::endl;
        }

        // Framebuffer resize (from enter fullscreen)
        int newWidth = state.curWndParams.fbSize[0];
        int newHeight = state.curWndParams.fbSize[1];
        if (newWidth != currentFBWidth || newHeight != currentFBHeight) {
            currentFBWidth = newWidth;
            currentFBHeight = newHeight;

            fbWidth = newWidth;
            fbHeight = newHeight;

            // Resize color buffer
            glBindTexture(GL_TEXTURE_2D, colorBuffer);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, newWidth, newHeight, 0, GL_RGBA, GL_FLOAT, NULL);

            // Resize depth buffer
            glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, newWidth, newHeight);
        }

        // camera movement
        glm::vec3 forward = state.cam.orientation * glm::vec3(0, 0, -1);
        glm::vec3 right = state.cam.orientation * glm::vec3(1, 0, 0);
        glm::vec3 up = state.cam.orientation * glm::vec3(0, 1, 0);

        float camSpeed = 10.0f;
        if (glfwGetKey(state.window, GLFW_KEY_W) == GLFW_PRESS) state.cam.pos += forward * camSpeed;
        if (glfwGetKey(state.window, GLFW_KEY_S) == GLFW_PRESS) state.cam.pos -= forward * camSpeed;
        if (glfwGetKey(state.window, GLFW_KEY_A) == GLFW_PRESS) state.cam.pos -= right * camSpeed;
        if (glfwGetKey(state.window, GLFW_KEY_D) == GLFW_PRESS) state.cam.pos += right * camSpeed;

        // Roll
        float rollSpeed = 0.02f;
        if (glfwGetKey(state.window, GLFW_KEY_Q) == GLFW_PRESS) {
            glm::quat roll = glm::angleAxis(rollSpeed, forward);
            state.cam.orientation = roll * state.cam.orientation;
        }
        if (glfwGetKey(state.window, GLFW_KEY_E) == GLFW_PRESS) {
            glm::quat roll = glm::angleAxis(-rollSpeed, forward);
            state.cam.orientation = roll * state.cam.orientation;
        }

        state.cam.orientation = glm::normalize(state.cam.orientation); // prevent precision loss

        // Object-common matrices
        float aspectRatio = float(state.curWndParams.fbSize[0]) / float(state.curWndParams.fbSize[1]);
        glm::mat4x4 proj = glm::perspective(glm::radians(50.0f), aspectRatio,
                                            5.0f, 50000.0f); // some changes for better view

        glm::mat4x4 view = glm::lookAt(state.cam.pos, state.cam.pos + forward, up);

        // HDR rendering to framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
        glViewport(0, 0, state.curWndParams.fbSize[0], state.curWndParams.fbSize[1]);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear the framebuffer for new frame

        // Start rendering
        glViewport(0, 0,
                   state.curWndParams.fbSize[0],
                   state.curWndParams.fbSize[1]);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        //glEnable(GL_CULL_FACE);

        // Bind shaders and related uniforms / textures
        // Move this somewhere proper later.
        // These must match the uniform "location" at the shader(s).
        // Vertex
        static constexpr GLuint U_TRANSFORM_MODEL   = 0;
        static constexpr GLuint U_TRANSFORM_VIEW    = 1;
        static constexpr GLuint U_TRANSFORM_PROJ    = 2;
        static constexpr GLuint U_TRANSFORM_NORMAL  = 3;
        // Fragment
        static constexpr GLuint T_ALBEDO = 0;
        static constexpr GLuint U_MODE = 0;

        // glActiveShaderProgram makes "glUniform" family of commands
        // to effect the selected shader
        glUseProgramStages(state.renderPipeline, GL_VERTEX_SHADER_BIT, vShader.shaderId);
        glActiveShaderProgram(state.renderPipeline, vShader.shaderId);
        {
            // Don't move the triangle
            glm::mat4x4 model = glm::identity<glm::mat4x4>();
            // Normal local->world matrix of the object
            glm::mat3x3 normalMatrix = glm::inverseTranspose(model);
            glUniformMatrix4fv(U_TRANSFORM_MODEL, 1, false, glm::value_ptr(model));
            glUniformMatrix4fv(U_TRANSFORM_VIEW, 1, false, glm::value_ptr(view));
            glUniformMatrix4fv(U_TRANSFORM_PROJ, 1, false, glm::value_ptr(proj));
            glUniformMatrix3fv(U_TRANSFORM_NORMAL, 1, false, glm::value_ptr(normalMatrix));

            // sending height info
            static constexpr GLuint U_HEIGHT_SCALE = 4;
            glUniform1f(U_HEIGHT_SCALE, state.heightScale);
        }
        // Fragment shader
        glUseProgramStages(state.renderPipeline, GL_FRAGMENT_SHADER_BIT, fShader.shaderId);
        glActiveShaderProgram(state.renderPipeline, fShader.shaderId);
        {
            // Bind texture(s)
            // You can bind texture via GL_TEXTURE0 + x where x is the bind point at the shader
            // you do not need to explicitly say GL_TEXTURE1 GL_TEXTURE2 etc.
            // Here is a demonstration as static assertions.
            static_assert(GL_TEXTURE0 + 1 == GL_TEXTURE1, "OGL API is wrong!");
            static_assert(GL_TEXTURE0 + 2 == GL_TEXTURE2, "OGL API is wrong!");
            static_assert(GL_TEXTURE0 + 16 == GL_TEXTURE16, "OGL API is wrong!");
            glActiveTexture(GL_TEXTURE0 + T_ALBEDO);
            glBindTexture(GL_TEXTURE_2D, tex.textureId);

            glUniform1ui(U_MODE, state.mode);
        }
        // Bind VAO
        glBindVertexArray(terrainVAO);
        // Draw call!
        // glDrawElements(GL_TRIANGLES, GLsizei(mesh.indexCount), GL_UNSIGNED_INT, nullptr);
        glDrawElements(GL_TRIANGLES, (GLsizei)state.indices.size(), GL_UNSIGNED_INT, nullptr);

        // Mipmap generation for the color buffer
        glBindTexture(GL_TEXTURE_2D, colorBuffer);
        glGenerateMipmap(GL_TEXTURE_2D);

        // Tonemapping (TEMP)
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // TEMP
        glBindFramebuffer(GL_READ_FRAMEBUFFER, hdrFBO);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, fbWidth, fbHeight, 0, 0, fbWidth, fbHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        // #TEMP

        glfwSwapBuffers(state.window);
    }
}
