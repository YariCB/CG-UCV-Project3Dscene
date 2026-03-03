#include "Interface.h"
#include "3DViewer.h"
#include <iostream>
#include <cstdio>
#include <limits>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

static UIState globalUIState;

static unsigned char* resizeRGBA(const unsigned char* src, int srcW, int srcH, int dstW, int dstH) {
    if (!src || srcW <= 0 || srcH <= 0 || dstW <= 0 || dstH <= 0) return nullptr;
    unsigned char* dst = new unsigned char[dstW * dstH * 4];
    for (int y = 0; y < dstH; ++y) {
        float v = (float)y * (float)(srcH) / (float)(dstH);
        int y0 = (int)floor(v);
        int y1 = std::min(y0 + 1, srcH - 1);
        float fy = v - y0;
        for (int x = 0; x < dstW; ++x) {
            float u = (float)x * (float)(srcW) / (float)(dstW);
            int x0 = (int)floor(u);
            int x1 = std::min(x0 + 1, srcW - 1);
            float fx = u - x0;
            for (int c = 0; c < 4; ++c) {
                float c00 = src[(y0 * srcW + x0) * 4 + c];
                float c10 = src[(y0 * srcW + x1) * 4 + c];
                float c01 = src[(y1 * srcW + x0) * 4 + c];
                float c11 = src[(y1 * srcW + x1) * 4 + c];
                float c0 = c00 * (1.0f - fx) + c10 * fx;
                float c1 = c01 * (1.0f - fx) + c11 * fx;
                float cval = c0 * (1.0f - fy) + c1 * fy;
                int out = (int)(cval + 0.5f);
                dst[(y * dstW + x) * 4 + c] = (unsigned char)std::min(255, std::max(0, out));
            }
        }
    }
    return dst;
}

C3DViewer::C3DViewer()
{
}

C3DViewer::~C3DViewer()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    if (m_shaderProgram) glDeleteProgram(m_shaderProgram);
    if (m_window) glfwDestroyWindow(m_window);

    if (m_tableVAO) glDeleteVertexArrays(1, &m_tableVAO);
    if (m_tableVBO) glDeleteBuffers(1, &m_tableVBO);
    if (m_tableTexture) glDeleteTextures(1, &m_tableTexture);

    if (m_teapotVAO) glDeleteVertexArrays(1, &m_teapotVAO);
    if (m_teapotVBO) glDeleteBuffers(1, &m_teapotVBO);
    if (m_teapotTexture) glDeleteTextures(1, &m_teapotTexture);

    if (m_simpleShader) glDeleteProgram(m_simpleShader);
    if (m_lightShader) glDeleteProgram(m_lightShader);
    if (m_sphereVAO) glDeleteVertexArrays(1, &m_sphereVAO);
    if (m_sphereVBO) glDeleteBuffers(1, &m_sphereVBO);
    if (m_sphereEBO) glDeleteBuffers(1, &m_sphereEBO);

    glfwTerminate();
}

bool C3DViewer::setup()
{
    if (!glfwInit())
        return false;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_window = glfwCreateWindow(width, height, "Computación Gráfica - Proyecto 3 - Yarima Contreras", NULL, NULL);
    if (!m_window)
    {
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_window);

    // Inicializar glad
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        glfwDestroyWindow(m_window);
        glfwTerminate();
        return false;
    }

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glEnable(GL_DEPTH_TEST);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Opcional

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow* window, int w, int h) {
        auto ptr = reinterpret_cast<C3DViewer*>(glfwGetWindowUserPointer(window));
        if (ptr)
            ptr->resize(w, h);
        });

    // Setup shader
    if (!setupShader()) return false;

    // Mesa OBJ
    loadOBJTo("OBJs/table/table4.obj", m_tableVAO, m_tableVBO, m_tableVertexCount, m_tableHasTexCoords, m_tableMinY, m_tableMaxY);
    m_tableTexture = loadTexture("OBJs/table/tipical.jpg");
    if (m_tableTexture == 0) std::cout << "Warning: table texture not loaded (m_tableTexture==0)" << std::endl;

    // Tetera OBJ (cargar en su propia malla para no sobrescribir la mesa)
    loadOBJTo("OBJs/teapot/teapot.obj", m_teapotVAO, m_teapotVBO, m_teapotVertexCount, m_teapotHasTexCoords, m_teapotMinY, m_teapotMaxY);

    if (!setupSimpleShader()) return false;

    // Enable seamless cubemap sampling to avoid seams
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    // Mesa y Skybox
    // Si no se cargó un OBJ (m_tableVertexCount == 0) usamos el cubo de prueba;
    // de lo contrario conservamos el VAO creado por loadOBJ().
    if (m_tableVertexCount == 0) {
        setupTable();
    }

    std::vector<std::string> faces = {
        "OBJs/room/room_texture_3.png", // Derecha
        "OBJs/room/room_texture_2.png", // Izquierda
        "OBJs/room/ceiling_texture.png", // Techo
        "OBJs/room/floor_texture.png",   // Suelo
        "OBJs/room/room_texture_4.png", // Atrás
        "OBJs/room/room_texture_1.png"  // Frente
    };

    m_skyboxTexture = loadCubemap(faces);

    setupSkybox();
    setupSphere();

    glEnable(GL_DEPTH_TEST);

    glViewport(0, 0, width, height);

    glfwSetWindowUserPointer(m_window, this);
    glfwSetKeyCallback(m_window, keyCallbackStatic);
    glfwSetMouseButtonCallback(m_window, mouseButtonCallbackStatic);
    glfwSetCursorPosCallback(m_window, cursorPosCallbackStatic);

    // Inicializar temporizador de frames y posición del cursor
    m_lastFrame = glfwGetTime();
    lastX = width / 2.0;
    lastY = height / 2.0;

    return true;
}

void C3DViewer::update() {
    double currentTime = glfwGetTime();
    double deltaTime = currentTime - m_lastFrame;
    if (deltaTime < 0.0) deltaTime = 0.0;
    m_lastFrame = currentTime;

    // Actualizar posiciones de las luces (animación) usando el factor de la UI
    for (int i = 0; i < 3; i++) {
        float angle = (float)(currentTime * m_lightAngularSpeed[i] * globalUIState.lightSpeed);
        m_lightPos[i].x = m_lightRadii[i] * cos(angle);
        m_lightPos[i].z = m_lightRadii[i] * sin(angle);
        m_lightPos[i].y = m_lightHeights[i];
    }

    // Movimiento del usuario: Up/Down manejan avance/retroceso y Right/Left manejan desplazamiento lateral
    float velocity = movementSpeed * deltaTime;
    glm::vec3 moveDir = cameraFront;
    if (globalUIState.cameraMode == 0) { // Modo FPS: El ojo del humano miniatura
        moveDir.y = 0.0f;
        if (glm::length(moveDir) > 0.001f)
            moveDir = glm::normalize(moveDir);
    }
    glm::vec3 cameraRight = glm::normalize(glm::cross(cameraFront, glm::vec3(0.0f, 1.0f, 0.0f)));

    // Movimientos
    if (glfwGetKey(m_window, GLFW_KEY_UP) == GLFW_PRESS)
        cameraPos += moveDir * velocity;
    if (glfwGetKey(m_window, GLFW_KEY_DOWN) == GLFW_PRESS)
        cameraPos -= moveDir * velocity;
    if (glfwGetKey(m_window, GLFW_KEY_LEFT) == GLFW_PRESS)
        cameraPos -= cameraRight * velocity;
    if (glfwGetKey(m_window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        cameraPos += cameraRight * velocity;
}

void C3DViewer::mainLoop()
{
    while (!glfwWindowShouldClose(m_window))
    {
        glfwPollEvents();

        // Borrado
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        render();

        glfwSwapBuffers(m_window);
    }
}

void C3DViewer::onKey(int key, int scancode, int action, int mods)
{
    // Manejo de teclas para movimiento (Up/Down) y ESC
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_ESCAPE) {
            if (cursorCaptured) {
                glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                cursorCaptured = false;
            }
            else {
                glfwSetWindowShouldClose(m_window, GLFW_TRUE);
            }
        }
        else if (key == GLFW_KEY_UP) {
            moveForward = true;
        }
        else if (key == GLFW_KEY_DOWN) {
            moveBackward = true;
        }
    }
    else if (action == GLFW_RELEASE) {
        if (key == GLFW_KEY_UP) moveForward = false;
        else if (key == GLFW_KEY_DOWN) moveBackward = false;
    }
}

void C3DViewer::onMouseButton(int button, int action, int mods)
{
    // Botón derecho para capturar el cursor y entrar en modo look
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            cursorCaptured = true;
            firstMouse = true;
            double x, y; glfwGetCursorPos(m_window, &x, &y); lastX = x; lastY = y;
        }
        else if (action == GLFW_RELEASE) {
            glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            cursorCaptured = false;
        }
    }
}

void C3DViewer::onCursorPos(double xpos, double ypos)
{
    if (!cursorCaptured) return;

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = float(xpos - lastX);
    float yoffset = float(lastY - ypos);
    lastX = xpos;
    lastY = ypos;

    xoffset *= mouseSensitivity;
    yoffset *= mouseSensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

void C3DViewer::render() {
    update();

    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // --- Configuración de cámara ---
    glm::vec3 camPos = cameraPos;
    glm::mat4 view = glm::lookAt(camPos, camPos + cameraFront, cameraUp);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 500.0f);

    // --- Dibujo de skybox ---
    glUseProgram(m_simpleShader);
    glm::mat4 skyView = glm::mat4(glm::mat3(view));
    glUniformMatrix4fv(glGetUniformLocation(m_simpleShader, "view"), 1, GL_FALSE, glm::value_ptr(skyView));
    glUniformMatrix4fv(glGetUniformLocation(m_simpleShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    glBindVertexArray(m_skyboxVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_skyboxTexture);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);

    glEnable(GL_CULL_FACE);

    // --- Preparación de Objetos Iluminados ---
    glUseProgram(m_shaderProgram);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_skyboxTexture); // Unidad 1 para el Skybox (Ambient/Reflection)
    glUniform1i(glGetUniformLocation(m_shaderProgram, "skybox"), 1);
    glActiveTexture(GL_TEXTURE0);

    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3fv(glGetUniformLocation(m_shaderProgram, "viewPos"), 1, &camPos[0]);
    glUniform1i(glGetUniformLocation(m_shaderProgram, "attenuationEnabled"), globalUIState.attenuation);

    // Datos de Luces
    for (int i = 0; i < 3; i++) {
        char name[64];
        snprintf(name, sizeof(name), "lightPos[%d]", i);
        glUniform3fv(glGetUniformLocation(m_shaderProgram, name), 1, &m_lightPos[i][0]);
        snprintf(name, sizeof(name), "lightAmbient[%d]", i);
        glUniform3fv(glGetUniformLocation(m_shaderProgram, name), 1, globalUIState.lightAmbient[i]);
        snprintf(name, sizeof(name), "lightDiffuse[%d]", i);
        glUniform3fv(glGetUniformLocation(m_shaderProgram, name), 1, globalUIState.lightColors[i]);
        snprintf(name, sizeof(name), "lightSpecular[%d]", i);
        glUniform3fv(glGetUniformLocation(m_shaderProgram, name), 1, globalUIState.lightSpecular[i]);
        snprintf(name, sizeof(name), "lightEnabled[%d]", i);
        glUniform1i(glGetUniformLocation(m_shaderProgram, name), globalUIState.lightEnabled[i]);
        // Modelo de Iluminación
        snprintf(name, sizeof(name), "lightModel[%d]", i);
        glUniform1i(glGetUniformLocation(m_shaderProgram, name), globalUIState.shadingModels[i]);
    }

    // --- DIBUJO DE LA MESA ---
    glUniform1i(glGetUniformLocation(m_shaderProgram, "isReflective"), 0);
    // Material de madera
    glUniform3f(glGetUniformLocation(m_shaderProgram, "materialAmbient"), 0.35f, 0.35f, 0.35f);
    glUniform3f(glGetUniformLocation(m_shaderProgram, "materialDiffuse"), 0.8f, 0.8f, 0.8f);
    glUniform3f(glGetUniformLocation(m_shaderProgram, "materialSpecular"), 0.2f, 0.2f, 0.2f);
    glUniform1f(glGetUniformLocation(m_shaderProgram, "materialShininess"), 16.0f);

    float scale = 0.5f;
    float displacementY = -m_tableMinY * scale;
    glm::mat4 tableModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, displacementY, 0.0f));
    tableModel = glm::scale(tableModel, glm::vec3(scale));
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(tableModel));

    glBindTexture(GL_TEXTURE_2D, m_tableTexture);
    glUniform1i(glGetUniformLocation(m_shaderProgram, "useTexture"), (m_tableTexture != 0 && m_tableHasTexCoords) ? 1 : 0);
    glBindVertexArray(m_tableVAO);
    glDrawArrays(GL_TRIANGLES, 0, m_tableVertexCount);

    // --- DIBUJO DE LA TETERA ---
    if (m_teapotVertexCount > 0 && m_teapotVAO != 0) {
        // Reflejo de Fresnel
        glUniform1i(glGetUniformLocation(m_shaderProgram, "isReflective"), 1);
        // Material Metálico
        glUniform3f(glGetUniformLocation(m_shaderProgram, "materialAmbient"), 0.02f, 0.02f, 0.02f);
        glUniform3f(glGetUniformLocation(m_shaderProgram, "materialDiffuse"), 0.1f, 0.1f, 0.1f);
        glUniform3f(glGetUniformLocation(m_shaderProgram, "materialSpecular"), 1.0f, 1.0f, 1.0f);
        glUniform1f(glGetUniformLocation(m_shaderProgram, "materialShininess"), 256.0f);
        // Cálculos de escala y posición
        float tableHeight = scale * (m_tableMaxY - m_tableMinY);
        float teapotHeightModel = (m_teapotMaxY - m_teapotMinY);
        float teapotScale = (teapotHeightModel > 0.0001f) ? (tableHeight * 0.30f) / teapotHeightModel : 1.0f;
        float teapotDisplacementY = -m_teapotMinY * teapotScale + tableHeight + 0.02f;

        glm::mat4 teapotModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, teapotDisplacementY, 0.0f));
        teapotModel = glm::scale(teapotModel, glm::vec3(teapotScale));
        glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(teapotModel));

        glBindTexture(GL_TEXTURE_2D, m_teapotTexture);
        glUniform1i(glGetUniformLocation(m_shaderProgram, "useTexture"), (m_teapotTexture != 0 && m_teapotHasTexCoords) ? 1 : 0);

        glBindVertexArray(m_teapotVAO);
        glDrawArrays(GL_TRIANGLES, 0, m_teapotVertexCount);
    }

    // --- Dibujo de esferas de luz ---
    glUseProgram(m_lightShader);
    glUniformMatrix4fv(glGetUniformLocation(m_lightShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(m_lightShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glBindVertexArray(m_sphereVAO);
    for (int i = 0; i < 3; i++) {
        if (!globalUIState.lightEnabled[i]) continue;
        glm::mat4 lModel = glm::translate(glm::mat4(1.0f), m_lightPos[i]);
        lModel = glm::scale(lModel, glm::vec3(2.0f));
        glUniformMatrix4fv(glGetUniformLocation(m_lightShader, "model"), 1, GL_FALSE, glm::value_ptr(lModel));
        glUniform3fv(glGetUniformLocation(m_lightShader, "color"), 1, globalUIState.lightColors[i]);
        glDrawElements(GL_TRIANGLES, m_sphereIndexCount, GL_UNSIGNED_INT, 0);
    }

    drawInterface();
}

void C3DViewer::drawInterface()
{
    double currentTime = glfwGetTime();
    double deltaTime = currentTime - lastTime;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    DrawMainPanel(globalUIState);

    ImGui::SetNextWindowSize(ImVec2(400, 1000), ImGuiCond_Once);
    //ImGui::Begin("Control Panel");
    static int anyDummyValue = 50;
    //if (ImGui::SliderInt("slider-demo", &anyDummyValue, 1, 100)) 
    //{
        // valor actualizado autom�ticamente en anyDummyValue
    //}
    //ImGui::End();
    ImGui::Render();
    // Rendirizar ImGui con OpenGL
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    if (deltaTime >= 1.0)
    {
        lastTime = currentTime;
    }
}

void C3DViewer::resize(int new_width, int new_height)
{
    width = new_width;
    height = new_height;

    // actualizamos el viewport cada vez que haya un resize
    glViewport(0, 0, width, height);
}

bool C3DViewer::setupShader()
{
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSrc, nullptr);
    glCompileShader(vertexShader);
    if (!checkCompileErrors(vertexShader, "VERTEX")) return false;

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSrc, nullptr);
    glCompileShader(fragmentShader);
    if (!checkCompileErrors(fragmentShader, "FRAGMENT")) return false;

    m_shaderProgram = glCreateProgram();
    glAttachShader(m_shaderProgram, vertexShader);
    glAttachShader(m_shaderProgram, fragmentShader);
    glLinkProgram(m_shaderProgram);
    if (!checkCompileErrors(m_shaderProgram, "PROGRAM")) return false;

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    // Ensure sampler bindings for the main shader: texture_diffuse -> unit 0, skybox -> unit 1
    glUseProgram(m_shaderProgram);
    GLint locTex = glGetUniformLocation(m_shaderProgram, "texture_diffuse");
    if (locTex >= 0) glUniform1i(locTex, 0);
    GLint locSky = glGetUniformLocation(m_shaderProgram, "skybox");
    if (locSky >= 0) glUniform1i(locSky, 1);
    return true;
}

bool C3DViewer::checkCompileErrors(GLuint shader, const char* type)
{
    GLint success;
    GLchar infoLog[1024];
    if (strcmp(type, "PROGRAM") != 0)
    {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            fprintf(stderr, "ERROR::SHADER_COMPILATION_ERROR of type: %s\n%s\n", type, infoLog);
            return false;
        }
    }
    else
    {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            fprintf(stderr, "ERROR::PROGRAM_LINKING_ERROR of type: %s\n%s\n", type, infoLog);
            return false;
        }
    }
    return true;
}

void C3DViewer::setupTriangle()
{
    float vertices[] =
    {
        // x      y      z     r     g     b 
        -1.0f,  1.0f,  0.0f, 1.0f, 0.0f, 0.0f,
         1.0f,  1.0f,  0.0f, 0.0f, 1.0f, 0.0f,
         1.0f, -1.0f,  0.0f, 0.0f, 0.0f, 1.0f
    };

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

void C3DViewer::keyCallbackStatic(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
    C3DViewer* self = (C3DViewer*)glfwGetWindowUserPointer(window);
    if (self)
        self->onKey(key, scancode, action, mods);
}

void C3DViewer::mouseButtonCallbackStatic(GLFWwindow* window, int button, int action, int mods)
{
    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
    C3DViewer* self = (C3DViewer*)glfwGetWindowUserPointer(window);
    if (self)
        self->onMouseButton(button, action, mods);
}

void C3DViewer::cursorPosCallbackStatic(GLFWwindow* window, double xpos, double ypos)
{
    ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);
    C3DViewer* self = (C3DViewer*)glfwGetWindowUserPointer(window);
    if (self)
        self->onCursorPos(xpos, ypos);
}

void C3DViewer::setupTable() {
    // Definimos un cubo completo para la mesa (posiciones, normales, color)
    // Para simplificar ahora, usamos 6 caras: Pos(3), Color(3)
    float tableVertices[] = {
        // Cara Trasera (normal 0,0,-1)
        -0.5f, -0.5f, -0.5f,  0,0,-1,  0.4f, 0.2f, 0.1f,
         0.5f, -0.5f, -0.5f,  0,0,-1,  0.4f, 0.2f, 0.1f,
         0.5f,  0.5f, -0.5f,  0,0,-1,  0.4f, 0.2f, 0.1f,
         0.5f,  0.5f, -0.5f,  0,0,-1,  0.4f, 0.2f, 0.1f,
        -0.5f,  0.5f, -0.5f,  0,0,-1,  0.4f, 0.2f, 0.1f,
        -0.5f, -0.5f, -0.5f,  0,0,-1,  0.4f, 0.2f, 0.1f,

        // Cara Frontal (normal 0,0,1)
        -0.5f, -0.5f,  0.5f,  0,0,1,  0.4f, 0.2f, 0.1f,
         0.5f, -0.5f,  0.5f,  0,0,1,  0.4f, 0.2f, 0.1f,
         0.5f,  0.5f,  0.5f,  0,0,1,  0.4f, 0.2f, 0.1f,
         0.5f,  0.5f,  0.5f,  0,0,1,  0.4f, 0.2f, 0.1f,
        -0.5f,  0.5f,  0.5f,  0,0,1,  0.4f, 0.2f, 0.1f,
        -0.5f, -0.5f,  0.5f,  0,0,1,  0.4f, 0.2f, 0.1f,

        // Cara Izquierda (normal -1,0,0)
        -0.5f,  0.5f,  0.5f, -1,0,0,  0.3f, 0.15f, 0.05f,
        -0.5f,  0.5f, -0.5f, -1,0,0,  0.3f, 0.15f, 0.05f,
        -0.5f, -0.5f, -0.5f, -1,0,0,  0.3f, 0.15f, 0.05f,
        -0.5f, -0.5f, -0.5f, -1,0,0,  0.3f, 0.15f, 0.05f,
        -0.5f, -0.5f,  0.5f, -1,0,0,  0.3f, 0.15f, 0.05f,
        -0.5f,  0.5f,  0.5f, -1,0,0,  0.3f, 0.15f, 0.05f,

        // Cara Derecha (normal 1,0,0)
         0.5f,  0.5f,  0.5f,  1,0,0,  0.3f, 0.15f, 0.05f,
         0.5f,  0.5f, -0.5f,  1,0,0,  0.3f, 0.15f, 0.05f,
         0.5f, -0.5f, -0.5f,  1,0,0,  0.3f, 0.15f, 0.05f,
         0.5f, -0.5f, -0.5f,  1,0,0,  0.3f, 0.15f, 0.05f,
         0.5f, -0.5f,  0.5f,  1,0,0,  0.3f, 0.15f, 0.05f,
         0.5f,  0.5f,  0.5f,  1,0,0,  0.3f, 0.15f, 0.05f,

         // Cara Inferior (normal 0,-1,0)
         -0.5f, -0.5f, -0.5f,  0,-1,0,  0.2f, 0.1f, 0.05f,
          0.5f, -0.5f, -0.5f,  0,-1,0,  0.2f, 0.1f, 0.05f,
          0.5f, -0.5f,  0.5f,  0,-1,0,  0.2f, 0.1f, 0.05f,
          0.5f, -0.5f,  0.5f,  0,-1,0,  0.2f, 0.1f, 0.05f,
         -0.5f, -0.5f,  0.5f,  0,-1,0,  0.2f, 0.1f, 0.05f,
         -0.5f, -0.5f, -0.5f,  0,-1,0,  0.2f, 0.1f, 0.05f,

         // Cara Superior (normal 0,1,0)
         -0.5f,  0.5f, -0.5f,  0,1,0,  0.5f, 0.25f, 0.15f,
          0.5f,  0.5f, -0.5f,  0,1,0,  0.5f, 0.25f, 0.15f,
          0.5f,  0.5f,  0.5f,  0,1,0,  0.5f, 0.25f, 0.15f,
          0.5f,  0.5f,  0.5f,  0,1,0,  0.5f, 0.25f, 0.15f,
         -0.5f,  0.5f,  0.5f,  0,1,0,  0.5f, 0.25f, 0.15f,
         -0.5f,  0.5f, -0.5f,  0,1,0,  0.5f, 0.25f, 0.15f
    };

    glGenVertexArrays(1, &m_tableVAO);
    glGenBuffers(1, &m_tableVBO);

    m_tableVertexCount = sizeof(tableVertices) / (9 * sizeof(float));

    m_tableHasTexCoords = false;
    m_tableMinY = -0.5f;
    m_tableMaxY = 0.5f;

    glBindVertexArray(m_tableVAO);
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cerr << "OpenGL error: " << err << std::endl;
    }
    glBindBuffer(GL_ARRAY_BUFFER, m_tableVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(tableVertices), tableVertices, GL_STATIC_DRAW);

    // Atributo Posici�n (0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Atributo Normal (1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // Atributo Color (2)
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void C3DViewer::setupSkybox() {
    // V�rtices para un cubo que nos rodea (solo posici�n)
    float skyboxVertices[] = {
        //   Posiciones (x, y, z)      // Colores (r, g, b)
        -1.0f,  1.0f, -1.0f,  0.5f, 0.8f, 1.0f,
        -1.0f, -1.0f, -1.0f,  0.5f, 0.8f, 1.0f,
         1.0f, -1.0f, -1.0f,  0.5f, 0.8f, 1.0f,
         1.0f, -1.0f, -1.0f,  0.5f, 0.8f, 1.0f,
         1.0f,  1.0f, -1.0f,  0.5f, 0.8f, 1.0f,
        -1.0f,  1.0f, -1.0f,  0.5f, 0.8f, 1.0f,

        -1.0f, -1.0f,  1.0f,  0.5f, 0.8f, 1.0f,
        -1.0f, -1.0f, -1.0f,  0.5f, 0.8f, 1.0f,
        -1.0f,  1.0f, -1.0f,  0.5f, 0.8f, 1.0f,
        -1.0f,  1.0f, -1.0f,  0.5f, 0.8f, 1.0f,
        -1.0f,  1.0f,  1.0f,  0.5f, 0.8f, 1.0f,
        -1.0f, -1.0f,  1.0f,  0.5f, 0.8f, 1.0f,

         1.0f, -1.0f, -1.0f,  0.5f, 0.8f, 1.0f,
         1.0f, -1.0f,  1.0f,  0.5f, 0.8f, 1.0f,
         1.0f,  1.0f,  1.0f,  0.5f, 0.8f, 1.0f,
         1.0f,  1.0f,  1.0f,  0.5f, 0.8f, 1.0f,
         1.0f,  1.0f, -1.0f,  0.5f, 0.8f, 1.0f,
         1.0f, -1.0f, -1.0f,  0.5f, 0.8f, 1.0f,

        -1.0f, -1.0f,  1.0f,  0.5f, 0.8f, 1.0f,
         1.0f, -1.0f,  1.0f,  0.5f, 0.8f, 1.0f,
         1.0f,  1.0f,  1.0f,  0.5f, 0.8f, 1.0f,
         1.0f,  1.0f,  1.0f,  0.5f, 0.8f, 1.0f,
        -1.0f,  1.0f,  1.0f,  0.5f, 0.8f, 1.0f,
        -1.0f, -1.0f,  1.0f,  0.5f, 0.8f, 1.0f,

        -1.0f,  1.0f, -1.0f,  0.5f, 0.8f, 1.0f,
         1.0f,  1.0f, -1.0f,  0.5f, 0.8f, 1.0f,
         1.0f,  1.0f,  1.0f,  0.5f, 0.8f, 1.0f,
         1.0f,  1.0f,  1.0f,  0.5f, 0.8f, 1.0f,
        -1.0f,  1.0f,  1.0f,  0.5f, 0.8f, 1.0f,
        -1.0f,  1.0f, -1.0f,  0.5f, 0.8f, 1.0f,

        -1.0f, -1.0f, -1.0f,  0.5f, 0.8f, 1.0f,
        -1.0f, -1.0f,  1.0f,  0.5f, 0.8f, 1.0f,
         1.0f, -1.0f, -1.0f,  0.5f, 0.8f, 1.0f,
         1.0f, -1.0f, -1.0f,  0.5f, 0.8f, 1.0f,
        -1.0f, -1.0f,  1.0f,  0.5f, 0.8f, 1.0f,
         1.0f, -1.0f,  1.0f,  0.5f, 0.8f, 1.0f
    };

    glGenVertexArrays(1, &m_skyboxVAO);
    glGenBuffers(1, &m_skyboxVBO);

    glBindVertexArray(m_skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);

    // Para el Skybox solo necesitamos posici�n (layout 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Atributo de color
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void C3DViewer::setupSphere() {
    std::vector<float> vertices;
    int stacks = 20, slices = 20;
    float radius = 1.0f;
    for (int i = 0; i <= stacks; ++i) {
        float phi = i * glm::pi<float>() / stacks; // de 0 a pi
        float sinPhi = sin(phi);
        float cosPhi = cos(phi);
        for (int j = 0; j <= slices; ++j) {
            float theta = j * 2 * glm::pi<float>() / slices;
            float sinTheta = sin(theta);
            float cosTheta = cos(theta);
            float x = radius * sinPhi * cosTheta;
            float y = radius * cosPhi;
            float z = radius * sinPhi * sinTheta;
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
        }
    }
    // indices para tri�ngulos
    std::vector<unsigned int> indices;
    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            int first = i * (slices + 1) + j;
            int second = first + slices + 1;
            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);
            indices.push_back(second);
            indices.push_back(second + 1);
            indices.push_back(first + 1);
        }
    }
    // Crear VAO, VBO, EBO
    glGenVertexArrays(1, &m_sphereVAO);
    glGenBuffers(1, &m_sphereVBO);
    glGenBuffers(1, &m_sphereEBO);
    glBindVertexArray(m_sphereVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_sphereEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    m_sphereIndexCount = indices.size();
}

bool C3DViewer::setupSimpleShader() {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &skyboxVertexSrc, nullptr);
    glCompileShader(vertexShader);
    if (!checkCompileErrors(vertexShader, "VERTEX")) return false;

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &skyboxFragmentSrc, nullptr);
    glCompileShader(fragmentShader);
    if (!checkCompileErrors(fragmentShader, "FRAGMENT")) return false;

    m_simpleShader = glCreateProgram();
    glAttachShader(m_simpleShader, vertexShader);
    glAttachShader(m_simpleShader, fragmentShader);
    glLinkProgram(m_simpleShader);
    if (!checkCompileErrors(m_simpleShader, "PROGRAM")) return false;

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    // Asegurar que el sampler del skybox está en la unidad 0
    glUseProgram(m_simpleShader);
    GLint loc = glGetUniformLocation(m_simpleShader, "skybox");
    if (loc >= 0) glUniform1i(loc, 0);
    // Ahora creamos un shader sencillo para las esferas de luz
    GLuint vert = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert, 1, &lightVertexSrc, nullptr);
    glCompileShader(vert);
    if (!checkCompileErrors(vert, "VERTEX")) return false;
    GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag, 1, &lightFragmentSrc, nullptr);
    glCompileShader(frag);
    if (!checkCompileErrors(frag, "FRAGMENT")) return false;
    m_lightShader = glCreateProgram();
    glAttachShader(m_lightShader, vert);
    glAttachShader(m_lightShader, frag);
    glLinkProgram(m_lightShader);
    if (!checkCompileErrors(m_lightShader, "PROGRAM")) return false;
    glDeleteShader(vert);
    glDeleteShader(frag);
    return true;
}

// Carga de mesa OBJ
void C3DViewer::loadOBJ(const std::string& path) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    std::cout << "Intentando cargar modelo desde: " << path << std::endl;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str(), "OBJs/table/")) {
        std::cout << "Error cargando OBJ: " << err << std::endl;
        return;
    }

    std::cout << "--- Modelo cargado con exito ---" << std::endl;
    std::cout << "Vertices encontrados: " << attrib.vertices.size() / 3 << std::endl;
    std::cout << "Caras/Formas encontradas: " << shapes.size() << std::endl;
    std::cout << "Materiales cargados: " << materials.size() << std::endl;

    std::vector<Vertex> vertices;
    bool hasTexCoords = false;
    bool hasNormals = false;
    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex{};
            vertex.Position = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };
            if (index.normal_index >= 0) {
                hasNormals = true;
                vertex.Normal = {
                    attrib.normals[3 * index.normal_index + 0],
                    attrib.normals[3 * index.normal_index + 1],
                    attrib.normals[3 * index.normal_index + 2]
                };
            }
            if (index.texcoord_index >= 0) {
                vertex.TexCoords = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    attrib.texcoords[2 * index.texcoord_index + 1]
                };
                hasTexCoords = true;
            }
            // Por defecto asignamos color blanco (si el OBJ no trae color por vértice)
            vertex.Color = glm::vec3(1.0f, 1.0f, 1.0f);
            vertices.push_back(vertex);
        }
    }
    m_tableVertexCount = vertices.size();
    m_tableHasTexCoords = hasTexCoords;

    // Calcular los límites en Y
    float minY = std::numeric_limits<float>::max();
    float maxY = -std::numeric_limits<float>::max();
    for (const auto& v : vertices) {
        if (v.Position.y < minY) minY = v.Position.y;
        if (v.Position.y > maxY) maxY = v.Position.y;
    }
    m_tableMinY = minY;
    m_tableMaxY = maxY;
    std::cout << "Altura de la mesa: minY = " << minY << ", maxY = " << maxY << ", altura total = " << (maxY - minY) << std::endl;

    // If OBJ didn't provide normals, compute smooth vertex normals from the triangle list
    if (!hasNormals && m_tableVertexCount >= 3) {
        for (size_t i = 0; i + 2 < vertices.size(); i += 3) {
            glm::vec3 v0 = vertices[i + 0].Position;
            glm::vec3 v1 = vertices[i + 1].Position;
            glm::vec3 v2 = vertices[i + 2].Position;
            glm::vec3 faceNormal = glm::normalize(glm::cross(v1 - v0, v2 - v0));
            if (glm::length(faceNormal) > 0.0f) {
                vertices[i + 0].Normal += faceNormal;
                vertices[i + 1].Normal += faceNormal;
                vertices[i + 2].Normal += faceNormal;
            }
        }
        // normalize accumulated normals
        for (size_t i = 0; i < vertices.size(); ++i) {
            if (glm::length(vertices[i].Normal) > 0.0001f)
                vertices[i].Normal = glm::normalize(vertices[i].Normal);
            else
                vertices[i].Normal = glm::vec3(0.0f, 1.0f, 0.0f); // fallback
        }
        std::cout << "Computed vertex normals for OBJ (was missing in file)." << std::endl;
    }
    if (m_tableHasTexCoords) {
        std::cout << "OBJ contiene UVs. Primeros TexCoords: ";
        if (!vertices.empty()) {
            std::cout << vertices[0].TexCoords.x << "," << vertices[0].TexCoords.y << std::endl;
        }
    }
    else {
        std::cout << "OBJ no contiene UVs." << std::endl;
    }
    std::cout << "Vertices totales procesados para dibujo: " << m_tableVertexCount << std::endl;

    glGenVertexArrays(1, &m_tableVAO);
    glGenBuffers(1, &m_tableVBO);
    glBindVertexArray(m_tableVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_tableVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    // Posición
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    // Normales
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
    // Color (default blanco para OBJ)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Color));
    // UVs
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
}

unsigned int C3DViewer::loadTexture(const char* path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    int width, height, nrComponents;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    std::cout << "Cargando textura: " << path << " -> " << (data ? "OK" : "FALLÓ") << std::endl;
    if (!data) {
        std::cerr << "ERROR: No se pudo encontrar la imagen en: " << path << std::endl;
        glDeleteTextures(1, &textureID);
        return 0;
    }
    std::cout << "Textura cargada correctamente: " << path << " (" << width << "x" << height << ")" << std::endl;
    GLenum format = GL_RGB;
    if (nrComponents == 1) format = GL_RED;
    else if (nrComponents == 3) format = GL_RGB;
    else if (nrComponents == 4) format = GL_RGBA;

    glBindTexture(GL_TEXTURE_2D, textureID);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Set texture wrapping and filtering options
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
    return textureID;
}

unsigned int C3DViewer::loadCubemap(std::vector<std::string> faces) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
    int width, height, nrChannels;
    int faceWidth = -1, faceHeight = -1;
    int targetFaceSize = -1; // we will enforce square faces for cubemap
    bool allFacesLoaded = true;
    stbi_set_flip_vertically_on_load(false);
    if (faces.size() != 6) {
        std::cerr << "Warning: cubemap expects 6 faces, got " << faces.size() << std::endl;
    }
    for (unsigned int i = 0; i < faces.size(); i++) {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, STBI_rgb_alpha);
        if (data) {
            //std::cout << "  -> face size: " << width << "x" << height << " channels(requested=4)\n";
            // Determine target square size (first successful face sets it)
            int curMin = std::min(width, height);
            if (targetFaceSize < 0) targetFaceSize = curMin;

            // If current face is not the target size or not square, crop center to targetFaceSize
            unsigned char* uploadData = data;
            bool allocatedTemp = false;
            if (width != targetFaceSize || height != targetFaceSize) {
                unsigned char* temp = resizeRGBA(data, width, height, targetFaceSize, targetFaceSize);
                if (temp) {
                    uploadData = temp;
                    allocatedTemp = true;
                    //std::cout << "  -> resized face to " << targetFaceSize << "x" << targetFaceSize << "\n";
                }
                else {
                    //std::cout << "  -> resize failed, will attempt center-crop fallback\n";
                    int xoff = (width - targetFaceSize) / 2;
                    int yoff = (height - targetFaceSize) / 2;
                    size_t outRowBytes = (size_t)targetFaceSize * 4;
                    unsigned char* tmp2 = new unsigned char[targetFaceSize * targetFaceSize * 4];
                    for (int r = 0; r < targetFaceSize; ++r) {
                        unsigned char* src = data + ((r + yoff) * width + xoff) * 4;
                        unsigned char* dst = tmp2 + r * outRowBytes;
                        memcpy(dst, src, outRowBytes);
                    }
                    uploadData = tmp2;
                    allocatedTemp = true;
                    //std::cout << "  -> cropped face to " << targetFaceSize << "x" << targetFaceSize << " (fallback)\n";
                }
            }

            // Update faceWidth/faceHeight expectations
            if (faceWidth < 0) {
                faceWidth = targetFaceSize;
                faceHeight = targetFaceSize;
            }
            else if (targetFaceSize != faceWidth || targetFaceSize != faceHeight) {
                /*std::cerr << "Cubemap face size mismatch at " << faces[i]
                    << " (cropped to " << targetFaceSize << "x" << targetFaceSize << ") expected "
                    << faceWidth << "x" << faceHeight << std::endl;*/
                allFacesLoaded = false;
            }

            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            // Se cargan en el orden: +X, -X, +Y, -Y, +Z, -Z
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                0, GL_RGBA, targetFaceSize, targetFaceSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, uploadData);
            GLenum gle = glGetError();
            if (gle != GL_NO_ERROR) {
                std::cerr << "GL error after glTexImage2D for face " << i << ": 0x" << std::hex << gle << std::dec << std::endl;
            }

            if (allocatedTemp) delete[] uploadData;
            stbi_image_free(data);
            //std::cout << "Cubemap cara " << i << " cargada: " << faces[i] << std::endl;
        }
        else {
            //std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
            allFacesLoaded = false;
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    if (!allFacesLoaded) {
        std::cerr << "Warning: cubemap is incomplete or inconsistent; rendering may appear black." << std::endl;
    }

    return textureID;
}

// Load OBJ into a provided mesh (doesn't overwrite table members)
void C3DViewer::loadOBJTo(const std::string& path, GLuint& outVAO, GLuint& outVBO, size_t& outVertexCount, bool& outHasTexCoords, float& outMinY, float& outMaxY) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    std::cout << "Intentando cargar modelo desde: " << path << std::endl;

    std::string baseDir = "";
    size_t pos = path.find_last_of("/\\");
    if (pos != std::string::npos) baseDir = path.substr(0, pos + 1);
    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str(), baseDir.c_str())) {
        std::cout << "Error cargando OBJ: " << err << std::endl;
        return;
    }

    std::cout << "--- Modelo cargado con exito ---" << std::endl;
    std::cout << "Vertices encontrados: " << attrib.vertices.size() / 3 << std::endl;
    std::cout << "Caras/Formas encontradas: " << shapes.size() << std::endl;
    std::cout << "Materiales cargados: " << materials.size() << std::endl;

    std::vector<Vertex> vertices;
    bool hasTexCoords = false;
    bool hasNormals = false;
    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex{};
            vertex.Position = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };
            if (index.normal_index >= 0) {
                hasNormals = true;
                vertex.Normal = {
                    attrib.normals[3 * index.normal_index + 0],
                    attrib.normals[3 * index.normal_index + 1],
                    attrib.normals[3 * index.normal_index + 2]
                };
            }
            if (index.texcoord_index >= 0) {
                vertex.TexCoords = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    attrib.texcoords[2 * index.texcoord_index + 1]
                };
                hasTexCoords = true;
            }
            vertex.Color = glm::vec3(1.0f, 1.0f, 1.0f);
            vertices.push_back(vertex);
        }
    }
    outVertexCount = vertices.size();
    outHasTexCoords = hasTexCoords;

    float minY = std::numeric_limits<float>::max();
    float maxY = -std::numeric_limits<float>::max();
    for (const auto& v : vertices) {
        if (v.Position.y < minY) minY = v.Position.y;
        if (v.Position.y > maxY) maxY = v.Position.y;
    }
    outMinY = minY;
    outMaxY = maxY;
    std::cout << "Altura del OBJ: minY = " << minY << ", maxY = " << maxY << ", altura total = " << (maxY - minY) << std::endl;

    if (!hasNormals && outVertexCount >= 3) {
        for (size_t i = 0; i + 2 < vertices.size(); i += 3) {
            glm::vec3 v0 = vertices[i + 0].Position;
            glm::vec3 v1 = vertices[i + 1].Position;
            glm::vec3 v2 = vertices[i + 2].Position;
            glm::vec3 faceNormal = glm::normalize(glm::cross(v1 - v0, v2 - v0));
            if (glm::length(faceNormal) > 0.0f) {
                vertices[i + 0].Normal += faceNormal;
                vertices[i + 1].Normal += faceNormal;
                vertices[i + 2].Normal += faceNormal;
            }
        }
        for (size_t i = 0; i < vertices.size(); ++i) {
            if (glm::length(vertices[i].Normal) > 0.0001f)
                vertices[i].Normal = glm::normalize(vertices[i].Normal);
            else
                vertices[i].Normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }
        std::cout << "Computed vertex normals for OBJ (was missing in file)." << std::endl;
    }

    if (outHasTexCoords) {
        std::cout << "OBJ contiene UVs. Primeros TexCoords: ";
        if (!vertices.empty()) {
            std::cout << vertices[0].TexCoords.x << "," << vertices[0].TexCoords.y << std::endl;
        }
    }
    else {
        std::cout << "OBJ no contiene UVs." << std::endl;
    }
    std::cout << "Vertices totales procesados para dibujo: " << outVertexCount << std::endl;

    glGenVertexArrays(1, &outVAO);
    glGenBuffers(1, &outVBO);
    glBindVertexArray(outVAO);
    glBindBuffer(GL_ARRAY_BUFFER, outVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Color));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
}