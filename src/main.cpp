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

    // if (state.cam.isLeftButtonPressed) {
    //     // how much we moved
    //     float deltaX = (float)(x - state.cam.lastMousePos.x);
    //     float deltaY = (float)(y - state.cam.lastMousePos.y);
    //     state.cam.lastMousePos = glm::dvec2(x, y); // update
    //
    //     float sensitivity = 0.002f;
    //     float yawAngle = -deltaX * sensitivity;
    //     float pitchAngle = -deltaY * sensitivity;
    //
    //     // Pitch Quaternion
    //     glm::vec3 right = state.cam.orientation * glm::vec3(1, 0, 0);
    //     glm::quat pitchQuaternion = glm::angleAxis(pitchAngle, right);
    //
    //     // Yaw Quaternion
    //     glm::vec3 up = glm::vec3(0, 1, 0);
    //     glm::quat yawQuaternion = glm::angleAxis(yawAngle, up);
    //
    //     state.cam.orientation = yawQuaternion * pitchQuaternion * state.cam.orientation;
    //     state.cam.orientation = glm::normalize(state.cam.orientation);
    // }

    float deltaX = (float)(x - state.cam.lastMousePos.x);
    float deltaY = (float)(y - state.cam.lastMousePos.y);
    state.cam.lastMousePos = glm::dvec2(x, y);

    float sensitivity = 0.002f;

    if (state.isRightButtonPressed){
        state.orbitCam.yaw += deltaX * sensitivity;
        state.orbitCam.pitch += deltaY * sensitivity;
        state.orbitCam.pitch = glm::clamp(state.orbitCam.pitch, -1.5f, 1.5f);
    }

    if (glfwGetKey(wnd, GLFW_KEY_LEFT_ALT) == GLFW_PRESS){
        glm::quat qYaw = glm::angleAxis(-deltaX * sensitivity, glm::vec3(0, 1, 0));
        glm::vec3 planeRight = state.plane.orientation * glm::vec3(1, 0, 0);
        glm::quat qPitch = glm::angleAxis(-deltaY * sensitivity, planeRight);

        state.plane.orientation = qYaw * qPitch * state.plane.orientation;
        state.plane.orientation = glm::normalize(state.plane.orientation);
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

    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        state.isRightButtonPressed = action == GLFW_PRESS;
    }

    if (action == GLFW_PRESS) {
        glfwGetCursorPos(wnd, &state.cam.lastMousePos.x, &state.cam.lastMousePos.y);
    }
}

void MouseScrollCallback(GLFWwindow* wnd, double dx, double dy)
{
    GLState& state = *static_cast<GLState*>(glfwGetWindowUserPointer(wnd));

    float zoomSpeed = 10.0f;

    state.orbitCam.distance -= (float) dy * zoomSpeed;

    // clamp
    state.orbitCam.distance = glm::clamp(state.orbitCam.distance, 20.0f, 2000.0f);
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
        if (key == GLFW_KEY_W)
            state.plane.speed += 0.5f;

        if (key == GLFW_KEY_S)
            state.plane.speed = std::max(0.0f, state.plane.speed - 0.5f);

        // yaw for plane
        if (key == GLFW_KEY_Q) {
            glm::quat qYaw = glm::angleAxis(0.02f, glm::vec3(0, 1, 0));
            state.plane.orientation = qYaw * state.plane.orientation;
        }

        if (key == GLFW_KEY_E) {
            glm::quat qYaw = glm::angleAxis(-0.02f, glm::vec3(0, 1, 0));
            state.plane.orientation = qYaw * state.plane.orientation;
        }
    }

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

void renderMesh(MeshGL* mesh, const glm::mat4& model, const glm::mat4& view, const glm::mat4& proj, GLuint pipeline, GLuint vShaderId, GLuint fShaderId, bool isGlass)
{
    glm::mat3 normalMatrix = glm::inverseTranspose(glm::mat3(model));

    glUseProgramStages(pipeline, GL_VERTEX_SHADER_BIT, vShaderId);
    glUseProgramStages(pipeline, GL_FRAGMENT_SHADER_BIT, fShaderId);

    glProgramUniformMatrix4fv(vShaderId, 0, 1, GL_FALSE, glm::value_ptr(model));
    glProgramUniformMatrix4fv(vShaderId, 1, 1, GL_FALSE, glm::value_ptr(view));
    glProgramUniformMatrix4fv(vShaderId, 2, 1, GL_FALSE, glm::value_ptr(proj));
    glProgramUniformMatrix3fv(vShaderId, 3, 1, GL_FALSE, glm::value_ptr(normalMatrix));

    glProgramUniform1i(fShaderId, glGetUniformLocation(fShaderId, "uIsGlass"), isGlass);

    glBindVertexArray(mesh->vaoId);
    glDrawElements(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, nullptr);
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

    // Tonemap shaders
    ShaderGL tmvShader = ShaderGL(ShaderGL::VERTEX, "shaders/tonemap.vert");
    ShaderGL tmfShader = ShaderGL(ShaderGL::FRAGMENT, "shaders/tonemap.frag");

    // Skybox shaders
    ShaderGL skyboxVShader = ShaderGL(ShaderGL::VERTEX, "shaders/skybox.vert");
    ShaderGL skyboxFShader = ShaderGL(ShaderGL::FRAGMENT, "shaders/skybox.frag");

    // Water shaders
    ShaderGL waterVShader = ShaderGL(ShaderGL::VERTEX, "shaders/water.vert");
    ShaderGL waterFShader = ShaderGL(ShaderGL::FRAGMENT, "shaders/water.frag");

    // Plane shaders
    ShaderGL planeVShader = ShaderGL(ShaderGL::VERTEX, "shaders/plane.vert");
    ShaderGL planeFShader = ShaderGL(ShaderGL::FRAGMENT, "shaders/plane.frag");

    TextureGL skyboxTex = TextureGL("kloppenheim_06_puresky_4k.hdr", TextureGL::LINEAR, TextureGL::REPEAT);
    TextureGL planeBaseTex = TextureGL("textures/plane_base_albedo.jpg", TextureGL::LINEAR, TextureGL::REPEAT);
    TextureGL planeHelixTex = TextureGL("textures/plane_helix_albedo.jpg", TextureGL::LINEAR, TextureGL::REPEAT);

    MeshGL mesh = MeshGL("meshes/tri.obj");
    TextureGL tex = TextureGL("textures/mixed_brick_wall_diff_1k.png",
                              TextureGL::LINEAR, TextureGL::REPEAT);

    state.plane.body = new MeshGL("meshes/plane_body.obj");
    state.plane.cables = new MeshGL("meshes/plane_cable.obj");
    state.plane.cockpit = new MeshGL("meshes/plane_glass.obj");
    state.plane.propeller = new MeshGL("meshes/plane_helix.obj");

    // Tonemap shader program (seperate from the main rendering pipeline)
    GLuint tonemapPipeline;
    glGenProgramPipelines(1, &tonemapPipeline);

    // Binding the tonemap vertex and tonemap fragment shaders to the pipeline
    glUseProgramStages(tonemapPipeline, GL_VERTEX_SHADER_BIT, tmvShader.shaderId);
    glUseProgramStages(tonemapPipeline, GL_FRAGMENT_SHADER_BIT, tmfShader.shaderId);

    // Water shaders and pipeline
    GLuint waterPipeline;
    glGenProgramPipelines(1, &waterPipeline);

    // Binding the water vertex and water fragment shaders to the pipeline
    glUseProgramStages(waterPipeline, GL_VERTEX_SHADER_BIT, waterVShader.shaderId);
    glUseProgramStages(waterPipeline, GL_FRAGMENT_SHADER_BIT, waterFShader.shaderId);

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

    float waterLevel = bbMin.y + 50.0f; // set water level a bit above the minimum height of the terrain
    state.plane.pos = glm::vec3(center.x, bbMax.y + 200.0f, center.z); // start above the terrain

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

    glGenerateMipmap(GL_TEXTURE_2D);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBuffer, 0);

    // Depth
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);

    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, fbWidth, fbHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Tonemapping quad
    float quadVertices[] = { // x, y, u, v
        -1.0f, 1.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, -1.0f, 1.0f, 0.0f,
    };

    GLuint tmquadVAO, tmquadVBO;
    glGenVertexArrays(1, &tmquadVAO);
    glGenBuffers(1, &tmquadVBO);
    glBindVertexArray(tmquadVAO);

    // Load quad vertices
    glBindBuffer(GL_ARRAY_BUFFER, tmquadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0); // vPos
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*) 0); // location 0
    glEnableVertexAttribArray(2); // vUV
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*) (2 * sizeof(float))); // location 2

    // Water
    int waterRes = 250;
    std::vector<glm::vec3> waterVertices;
    std::vector<uint32_t> waterIndices;

    float terrainSizeX = (bbMax.x - bbMin.x) * 1.5f; // bit bigger to guarantee
    float terrainSizeZ = (bbMax.z - bbMin.z) * 1.5f;

    // similar logic to terrain vertices with animation handled in the vertex shader
    for (int i = 0; i <= waterRes; ++i) {
        for (int j = 0; j <= waterRes; ++j) {
            // mapping water vertices to the terrain bounding box, with a bit of offset to make sure it covers the whole terrain even if the camera goes outside
            float x = bbMin.x + (float) i / waterRes * terrainSizeX - terrainSizeX * 0.1f;
            float z = bbMin.z + (float) j / waterRes * terrainSizeZ - terrainSizeZ * 0.1f;

            waterVertices.push_back(glm::vec3(x, 0.0f, z)); // y will be set in the vertex shader based on the water level
        }
    }

    for (int i = 0; i < waterRes; ++i) {
        for (int j = 0; j < waterRes; ++j) {
            // same logic as terrain indices but with waterRes instead of state.tesselationRate and without the vBase offset since water vertices are in a single array
            int row1 = i * (waterRes + 1);
            int row2 = (i + 1) * (waterRes + 1);
            waterIndices.push_back(row1 + j);
            waterIndices.push_back(row2 + j);
            waterIndices.push_back(row1 + j + 1);

            waterIndices.push_back(row1 + j + 1);
            waterIndices.push_back(row2 + j);
            waterIndices.push_back(row2 + j + 1);
        }
    }

    GLuint waterVAO, waterVBO, waterEBO;
    glGenVertexArrays(1, &waterVAO);
    glGenBuffers(1, &waterVBO);
    glGenBuffers(1, &waterEBO);

    glBindVertexArray(waterVAO);
    glBindBuffer(GL_ARRAY_BUFFER, waterVBO);
    glBufferData(GL_ARRAY_BUFFER, waterVertices.size() * sizeof(glm::vec3), waterVertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, waterEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, waterIndices.size() * sizeof(uint32_t), waterIndices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    // =============== //
    //   RENDER LOOP   //
    // =============== //
    while(!glfwWindowShouldClose(state.window))
    {
        // Poll inputs from the OS via GLFW
        glfwPollEvents();

        float aspectRatio = float(state.curWndParams.fbSize[0]) / float(state.curWndParams.fbSize[1]);
        glm::mat4x4 proj = glm::perspective(glm::radians(50.0f), aspectRatio,
                                            5.0f, 50000.0f); // some changes for better view

        glm::vec3 planeForward = state.plane.orientation * glm::vec3(0, 0, 1); // +z
        state.plane.pos += planeForward * state.plane.speed;
        state.plane.propellerAngle += state.plane.speed * 0.1f; // propeller speed depending on plane speed

        glm::vec3 relativeCamPos;
        // spherical coordinates based on the orbit camera angles and distance
        relativeCamPos.x = state.orbitCam.distance * cos(state.orbitCam.pitch) * sin(state.orbitCam.yaw);
        relativeCamPos.y = state.orbitCam.distance * sin(state.orbitCam.pitch);
        relativeCamPos.z = state.orbitCam.distance * cos(state.orbitCam.pitch) * cos(state.orbitCam.yaw);

        state.cam.pos = state.plane.pos - relativeCamPos;
        glm::mat4 view = glm::lookAt(state.cam.pos, state.plane.pos, glm::vec3(0, 1, 0));

        glm::mat4 planeWorldMatrix = glm::translate(glm::mat4(1.0f), state.plane.pos) * glm::mat4_cast(state.plane.orientation);

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
            glGenerateMipmap(GL_TEXTURE_2D);

            // Resize depth buffer
            glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, newWidth, newHeight);
        }

        // // camera movement
        // glm::vec3 forward = state.cam.orientation * glm::vec3(0, 0, -1);
        // glm::vec3 right = state.cam.orientation * glm::vec3(1, 0, 0);
        // glm::vec3 up = state.cam.orientation * glm::vec3(0, 1, 0);
        //
        // float camSpeed = 10.0f;
        // if (glfwGetKey(state.window, GLFW_KEY_W) == GLFW_PRESS) state.cam.pos += forward * camSpeed;
        // if (glfwGetKey(state.window, GLFW_KEY_S) == GLFW_PRESS) state.cam.pos -= forward * camSpeed;
        // if (glfwGetKey(state.window, GLFW_KEY_A) == GLFW_PRESS) state.cam.pos -= right * camSpeed;
        // if (glfwGetKey(state.window, GLFW_KEY_D) == GLFW_PRESS) state.cam.pos += right * camSpeed;

        // Roll for camera (from hw1)
        // float rollSpeed = 0.02f;
        // if (glfwGetKey(state.window, GLFW_KEY_Q) == GLFW_PRESS) {
        //     glm::quat roll = glm::angleAxis(rollSpeed, forward);
        //     state.cam.orientation = roll * state.cam.orientation;
        // }
        // if (glfwGetKey(state.window, GLFW_KEY_E) == GLFW_PRESS) {
        //     glm::quat roll = glm::angleAxis(-rollSpeed, forward);
        //     state.cam.orientation = roll * state.cam.orientation;
        // }

        state.cam.orientation = glm::normalize(state.cam.orientation); // prevent precision loss

        glm::mat4x4 model = glm::identity<glm::mat4x4>();

        // HDR rendering to framebuffer
        glBindProgramPipeline(state.renderPipeline); // switch to main rendering pipeline
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

        // Skybox
        glDepthFunc(GL_LEQUAL); //

        // switch to skybox vertex shader
        glUseProgramStages(state.renderPipeline, GL_VERTEX_SHADER_BIT, skyboxVShader.shaderId);
        glActiveShaderProgram(state.renderPipeline, skyboxVShader.shaderId);

        // Send inverse of projection and view matrices to the shader to reconstruct the world position from the depth buffer
        glm::mat4 invProj = glm::inverse(proj);
        glm::mat4 invView = glm::inverse(glm::mat4(glm::mat3(view)));

        glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(invProj));
        glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(invView));

        // switch to skybox fragment shader
        glUseProgramStages(state.renderPipeline, GL_FRAGMENT_SHADER_BIT, skyboxFShader.shaderId);
        glActiveShaderProgram(state.renderPipeline, skyboxFShader.shaderId);

        // Bind skybox texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, skyboxTex.textureId);

        glBindVertexArray(tmquadVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glBindProgramPipeline(waterPipeline); // switch to water pipeline

        // sending matrices and time for animation to the water vertex shader
        GLint wModelLoc = glGetUniformLocation(waterVShader.shaderId, "uModel");
        GLint wViewLoc  = glGetUniformLocation(waterVShader.shaderId, "uView");
        GLint wProjLoc  = glGetUniformLocation(waterVShader.shaderId, "uProj");
        GLint wTimeLoc  = glGetUniformLocation(waterVShader.shaderId, "uTime");

        glProgramUniformMatrix4fv(waterVShader.shaderId, wModelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glProgramUniformMatrix4fv(waterVShader.shaderId, wViewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glProgramUniformMatrix4fv(waterVShader.shaderId, wProjLoc, 1, GL_FALSE, glm::value_ptr(proj));
        glProgramUniform1f(waterVShader.shaderId, wTimeLoc, (float) glfwGetTime()); // time for animation

        GLint wWaterLevelLoc = glGetUniformLocation(waterVShader.shaderId, "uWaterLevel");
        glProgramUniform1f(waterVShader.shaderId, wWaterLevelLoc, waterLevel);

        // uniforms
        GLint wViewPosLoc = glGetUniformLocation(waterFShader.shaderId, "uViewPos");
        GLint wSkyTexLoc  = glGetUniformLocation(waterFShader.shaderId, "tSkybox");

        glProgramUniform3fv(waterFShader.shaderId, wViewPosLoc, 1, glm::value_ptr(state.cam.pos));
        glProgramUniform1i(waterFShader.shaderId, wSkyTexLoc, 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, skyboxTex.textureId);

        glBindVertexArray(waterVAO);
        glDrawElements(GL_TRIANGLES, (GLsizei) waterIndices.size(), GL_UNSIGNED_INT, nullptr);

        glDepthFunc(GL_LESS);

        glBindProgramPipeline(state.renderPipeline);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, planeBaseTex.textureId);

        // it's already in world space
        renderMesh(state.plane.body, planeWorldMatrix, view, proj, state.renderPipeline, planeVShader.shaderId, planeFShader.shaderId,false);

        glm::mat4 cablesModel = planeWorldMatrix
            * glm::translate(glm::mat4(1.0f), state.plane.cablesOffset);
        renderMesh(state.plane.cables, cablesModel, view, proj, state.renderPipeline, planeVShader.shaderId, planeFShader.shaderId, false);

        glBindTexture(GL_TEXTURE_2D, planeHelixTex.textureId);
        glm::mat4 propModel = planeWorldMatrix
            * glm::translate(glm::mat4(1.0f), state.plane.propOffset)
            * glm::rotate(glm::mat4(1.0f), state.plane.propellerAngle, glm::vec3(0, 0, 1));
        renderMesh(state.plane.propeller, propModel, view, proj, state.renderPipeline, planeVShader.shaderId, planeFShader.shaderId, false);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, skyboxTex.textureId); // hdr map for reflections on the cockpit glass

        GLint uViewPosLoc = glGetUniformLocation(planeFShader.shaderId, "uViewPos"); // needed for reflections in the cockpit shader
        glProgramUniform3fv(planeFShader.shaderId, uViewPosLoc, 1, glm::value_ptr(state.cam.pos));

        glm::mat4 cockpitModel = planeWorldMatrix
            * glm::translate(glm::mat4(1.0f), state.plane.cockpitOffset);
        renderMesh(state.plane.cockpit, cockpitModel, view, proj, state.renderPipeline, planeVShader.shaderId, planeFShader.shaderId, true);

        // Mipmap generation for the color buffer
        glBindTexture(GL_TEXTURE_2D, colorBuffer);
        glGenerateMipmap(GL_TEXTURE_2D);

        // Tonemapping
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, hdrFBO);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

        glBindProgramPipeline(tonemapPipeline); // switch to tonemap pipeline

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, colorBuffer);

        glActiveShaderProgram(tonemapPipeline, tmfShader.shaderId);
        glUniform1i(glGetUniformLocation(tmfShader.shaderId, "hdrsampler"), 0);

        glBindVertexArray(tmquadVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glfwSwapBuffers(state.window);
    }
}
