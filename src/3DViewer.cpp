#include "Interface.h"
#include "3DViewer.h"
#include <iostream>
#include <cstdio>
#include <limits>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>
#include <unordered_map>

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

    for (auto& sm : m_teapotSubmeshes) {
        if (sm.vao) glDeleteVertexArrays(1, &sm.vao);
        if (sm.vbo) glDeleteBuffers(1, &sm.vbo);
        if (sm.texDiffuse) glDeleteTextures(1, &sm.texDiffuse);
        if (sm.texAmbient) glDeleteTextures(1, &sm.texAmbient);
        if (sm.texSpecular) glDeleteTextures(1, &sm.texSpecular);
    }

	for (auto& sm : m_coffeeSubmeshes) {
		if (sm.vao) glDeleteVertexArrays(1, &sm.vao);
		if (sm.vbo) glDeleteBuffers(1, &sm.vbo);
		if (sm.texDiffuse) glDeleteTextures(1, &sm.texDiffuse);
		if (sm.texAmbient) glDeleteTextures(1, &sm.texAmbient);
		if (sm.texSpecular) glDeleteTextures(1, &sm.texSpecular);
	}
	for (auto& sm : m_cupSubmeshes) {
		if (sm.vao) glDeleteVertexArrays(1, &sm.vao);
		if (sm.vbo) glDeleteBuffers(1, &sm.vbo);
		if (sm.texDiffuse) glDeleteTextures(1, &sm.texDiffuse);
		if (sm.texAmbient) glDeleteTextures(1, &sm.texAmbient);
		if (sm.texSpecular) glDeleteTextures(1, &sm.texSpecular);
	}
	for (auto& sm : m_bowlSubmeshes) {
		if (sm.vao) glDeleteVertexArrays(1, &sm.vao);
		if (sm.vbo) glDeleteBuffers(1, &sm.vbo);
		if (sm.texDiffuse) glDeleteTextures(1, &sm.texDiffuse);
		if (sm.texAmbient) glDeleteTextures(1, &sm.texAmbient);
		if (sm.texSpecular) glDeleteTextures(1, &sm.texSpecular);
	}
	for (auto& sm : m_cardsSubmeshes) {
		if (sm.vao) glDeleteVertexArrays(1, &sm.vao);
		if (sm.vbo) glDeleteBuffers(1, &sm.vbo);
		if (sm.texDiffuse) glDeleteTextures(1, &sm.texDiffuse);
		if (sm.texAmbient) glDeleteTextures(1, &sm.texAmbient);
		if (sm.texSpecular) glDeleteTextures(1, &sm.texSpecular);
	}

	if (m_skyboxVAO) glDeleteVertexArrays(1, &m_skyboxVAO);
	if (m_skyboxVBO) glDeleteBuffers(1, &m_skyboxVBO);
	if (m_skyboxTexture) glDeleteTextures(1, &m_skyboxTexture);

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
    std::cout << "--- Iniciando carga de: OBJs/table/table4.obj ---" << std::endl;
    loadOBJTo("OBJs/table/table4.obj", m_tableVAO, m_tableVBO, m_tableVertexCount, m_tableHasTexCoords, m_tableMinY, m_tableMaxY);
    // Extensión X/Z de la mesa para colocar objetos en los extremos
    {
        tinyobj::attrib_t attrib; std::vector<tinyobj::shape_t> shapes; std::vector<tinyobj::material_t> materials; std::string warn, err;
        std::string path = "OBJs/table/table4.obj";
        size_t pos = path.find_last_of("/\\"); std::string baseDir = (pos!=std::string::npos)? path.substr(0,pos+1) : std::string("");
        if (tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str(), baseDir.c_str())) {
            float minX = std::numeric_limits<float>::max();
            float maxX = -std::numeric_limits<float>::max();
            float minZ = std::numeric_limits<float>::max();
            float maxZ = -std::numeric_limits<float>::max();

            for (size_t i = 0; i + 2 < attrib.vertices.size(); i += 3) {
                float x = attrib.vertices[i + 0];
                float y = attrib.vertices[i + 1];
                float z = attrib.vertices[i + 2];
                if (x < minX) minX = x;
                if (x > maxX) maxX = x;
                if (z < minZ) minZ = z;
                if (z > maxZ) maxZ = z;
            }
            m_tableMinX = minX;
            m_tableMaxX = maxX;
            m_tableMinZ = minZ;
            m_tableMaxZ = maxZ;
            std::cout << "Table bounds X:["<<minX<<","<<maxX<<"] Z:["<<minZ<<","<<maxZ<<"]\n";
        }
        else {
            m_tableMinX = -70.0f; m_tableMaxX = 70.0f; m_tableMinZ = -45.0f; m_tableMaxZ = 45.0f; // fallback
        }
    }
    m_tableTexture = loadTexture("OBJs/table/tipical.jpg");
    if (m_tableTexture == 0) std::cout << "Warning: table texture not loaded (m_tableTexture==0)" << std::endl;

    // Carga de tetera
    std::cout << "--- Iniciando carga de: OBJs/teapot/teapot.obj ---" << std::endl;
    m_teapotSubmeshes.clear();
    loadOBJToMulti("OBJs/teapot/teapot.obj", m_teapotSubmeshes, m_teapotMinY, m_teapotMaxY, m_teapotHasTexCoords);
    
    // Carga de tazas de cafe
    std::cout << "--- Iniciando carga de: OBJs/coffee/coffee.obj ---" << std::endl;
    m_coffeeSubmeshes.clear();
    loadOBJToMulti("OBJs/coffee/coffee.obj", m_coffeeSubmeshes, m_coffeeMinY, m_coffeeMaxY, m_coffeeHasTexCoords);

    // Carga de taza
    std::cout << "--- Iniciando carga de: OBJs/cup/cup.obj ---" << std::endl;
    m_cupSubmeshes.clear();
    loadOBJToMulti("OBJs/cup/cup.obj", m_cupSubmeshes, m_cupMinY, m_cupMaxY, m_cupHasTexCoords);

    // Bowl: Cada fruta es una submalla
    m_bowlSubmeshes.clear();
    loadOBJToMulti("OBJs/fruits/bowlWithFruits.obj", m_bowlSubmeshes, m_bowlMinY, m_bowlMaxY, m_bowlHasTexCoords);

    // Cards: Cada carta es una submalla
    std::cout << "--- Iniciando carga de: OBJs/cards/cards.obj ---" << std::endl;
    m_cardsSubmeshes.clear();
    loadOBJToMulti("OBJs/cards/cards.obj", m_cardsSubmeshes, m_cardsMinY, m_cardsMaxY, m_cardsHasTexCoords);

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

    // Actualizar posiciones de las luces usando el factor de la UI
    for (int i = 0; i < 3; i++) {
        float angle = (float)(currentTime * m_lightAngularSpeed[i] * globalUIState.lightSpeed);
        m_lightPos[i].x = m_lightRadii[i] * cos(angle);
        m_lightPos[i].z = m_lightRadii[i] * sin(angle);
        m_lightPos[i].y = m_lightHeights[i];
    }

    // Actualizar animaciones de cartas y tetera
    updateCardsAnimation(deltaTime);
    updateTeapotAnimation(deltaTime);

    // Cambio de modo de camara (caida del miniman si se pasa de GOD a FPS)
    if (globalUIState.cameraMode != m_prevCameraMode) {
        if (globalUIState.cameraMode == 0) { // Cambio a modo FPS
            m_fallAnimationActive = true;
            m_fallStartY = cameraPos.y;
            float scale = 0.5f;
            float tableHeight = scale * (m_tableMaxY - m_tableMinY);
            m_fallTargetY = tableHeight + 1.5f;
            m_fallTimer = 0.0f;
        }
        // El cambio a modo GOD no tiene animacion
        m_prevCameraMode = globalUIState.cameraMode;
    }
    // Animacion de caida
    if (m_fallAnimationActive) {
        m_fallTimer += (float)deltaTime;
        if (m_fallTimer >= m_fallDuration) {
            m_fallAnimationActive = false;
            cameraPos.y = m_fallTargetY;
        }
        else {
            float t = m_fallTimer / m_fallDuration;
            // Curva de easing para caida natural
            t = t * (2.0f - t);
            cameraPos.y = glm::mix(m_fallStartY, m_fallTargetY, t);
        }
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
    if (glfwGetKey(m_window, GLFW_KEY_UP) == GLFW_PRESS || glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += moveDir * velocity;
    if (glfwGetKey(m_window, GLFW_KEY_DOWN) == GLFW_PRESS || glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= moveDir * velocity;
    if (glfwGetKey(m_window, GLFW_KEY_LEFT) == GLFW_PRESS || glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= cameraRight * velocity;
    if (glfwGetKey(m_window, GLFW_KEY_RIGHT) == GLFW_PRESS || glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS)
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
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_skyboxTexture); 
    glUniform1i(glGetUniformLocation(m_shaderProgram, "skybox"), 1);
    glActiveTexture(GL_TEXTURE0);

    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3fv(glGetUniformLocation(m_shaderProgram, "viewPos"), 1, &camPos[0]);
    glUniform1i(glGetUniformLocation(m_shaderProgram, "attenuationEnabled"), globalUIState.attenuation);

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
        snprintf(name, sizeof(name), "lightModel[%d]", i);
        glUniform1i(glGetUniformLocation(m_shaderProgram, name), globalUIState.shadingModels[i]);
    }

    // Cálculos globales de la mesa
    float scale = 0.5f; 
    float tableHeight = scale * (m_tableMaxY - m_tableMinY);
    float tableHalfX = ((m_tableMaxX - m_tableMinX) * 0.5f) * scale;
    float tableHalfZ = ((m_tableMaxZ - m_tableMinZ) * 0.5f) * scale;

    // --- Dibujo de la Mesa ---
    glUniform1i(glGetUniformLocation(m_shaderProgram, "isReflective"), 0);
    glUniform3f(glGetUniformLocation(m_shaderProgram, "materialAmbient"), 0.35f, 0.35f, 0.35f);
    glUniform3f(glGetUniformLocation(m_shaderProgram, "materialDiffuse"), 0.8f, 0.8f, 0.8f);
    glUniform3f(glGetUniformLocation(m_shaderProgram, "materialSpecular"), 0.2f, 0.2f, 0.2f);
    glUniform1f(glGetUniformLocation(m_shaderProgram, "materialShininess"), 16.0f);

    float displacementY = -m_tableMinY * scale;
    glm::mat4 tableModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, displacementY, 0.0f));
    tableModel = glm::scale(tableModel, glm::vec3(scale));
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(tableModel));

    glBindTexture(GL_TEXTURE_2D, m_tableTexture);
    glUniform1i(glGetUniformLocation(m_shaderProgram, "useTexture"), (m_tableTexture != 0 && m_tableHasTexCoords) ? 1 : 0);
    glBindVertexArray(m_tableVAO);
    glDrawArrays(GL_TRIANGLES, 0, m_tableVertexCount);

    // --- Dibujo de la Tetera ---
    if (!m_teapotSubmeshes.empty()) {
        glUniform1i(glGetUniformLocation(m_shaderProgram, "isReflective"), 1);

        float teapotHeightModel = (m_teapotMaxY - m_teapotMinY);
        float teapotScale = (teapotHeightModel > 0.0001f) ? (tableHeight * 0.30f) / teapotHeightModel : 1.0f;
        float teapotDisplacementY = -m_teapotMinY * teapotScale + tableHeight + 0.02f;

        glm::mat4 teapotModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, teapotDisplacementY, 0.0f));
        teapotModel = glm::scale(teapotModel, glm::vec3(teapotScale));
        // Aplicar animación
        teapotModel = glm::translate(teapotModel, m_teapotExtraPos);
        teapotModel = glm::rotate(teapotModel, m_teapotExtraYaw, glm::vec3(0, 1, 0));
        teapotModel = glm::rotate(teapotModel, -m_teapotExtraPitch, glm::vec3(0, 0, 1));

        for (const auto& sm : m_teapotSubmeshes) {
            glm::mat4 model = teapotModel;
            glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

            // Para la tetera, si los coeficientes son los por defecto (porque no tiene MTL),
            // usamos valores metálicos. Si tiene MTL, respetamos los del archivo.
            glm::vec3 Ka = (sm.Ka != glm::vec3(0.02f)) ? sm.Ka : glm::vec3(0.02f);
            glm::vec3 Kd = (sm.Kd != glm::vec3(0.8f)) ? sm.Kd : glm::vec3(0.1f);
            glm::vec3 Ks = (sm.Ks != glm::vec3(0.2f)) ? sm.Ks : glm::vec3(1.0f);
            float Ns = (sm.Ns != 32.0f) ? sm.Ns : 256.0f;

            glUniform3f(glGetUniformLocation(m_shaderProgram, "materialAmbient"), Ka.r, Ka.g, Ka.b);
            glUniform3f(glGetUniformLocation(m_shaderProgram, "materialDiffuse"), Kd.r, Kd.g, Kd.b);
            glUniform3f(glGetUniformLocation(m_shaderProgram, "materialSpecular"), Ks.r, Ks.g, Ks.b);
            glUniform1f(glGetUniformLocation(m_shaderProgram, "materialShininess"), Ns);

            // Texturas (si las tuviera)
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, sm.texDiffuse ? sm.texDiffuse : 0);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, sm.texAmbient);
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, sm.texSpecular);

            glUniform1i(glGetUniformLocation(m_shaderProgram, "useTexture"), (sm.texDiffuse != 0 && sm.hasTexCoords) ? 1 : 0);
            glUniform1i(glGetUniformLocation(m_shaderProgram, "hasAmbientMap"), sm.texAmbient != 0 ? 1 : 0);
            glUniform1i(glGetUniformLocation(m_shaderProgram, "hasSpecularMap"), sm.texSpecular != 0 ? 1 : 0);

            glBindVertexArray(sm.vao);
            glDrawArrays(GL_TRIANGLES, 0, sm.vertexCount);
        }
    }

    // --- Dibujo de Objetos Sobre la Mesa ---
    
    // Cards
    if (!m_cardsSubmeshes.empty()) {
        glUniform1i(glGetUniformLocation(m_shaderProgram, "isReflective"), 0);
        glUniform3f(glGetUniformLocation(m_shaderProgram, "materialAmbient"), 0.05f, 0.05f, 0.05f);
        glUniform3f(glGetUniformLocation(m_shaderProgram, "materialDiffuse"), 0.85f, 0.85f, 0.85f);
        glUniform3f(glGetUniformLocation(m_shaderProgram, "materialSpecular"), 0.2f, 0.2f, 0.2f);
        glUniform1f(glGetUniformLocation(m_shaderProgram, "materialShininess"), 32.0f);

        float cardHeightModel = (m_cardsMaxY - m_cardsMinY);
        float cardScale = (cardHeightModel > 0.0001f) ? (tableHeight * 0.35f) / cardHeightModel : 1.0f;
        float cardDisplacementY = -m_cardsMinY * cardScale + tableHeight + 0.01f;

        glm::mat4 baseModel = glm::translate(glm::mat4(1.0f), glm::vec3(tableHalfX - 6.0f, cardDisplacementY, 0.0f));
        baseModel = glm::rotate(baseModel, glm::radians(90.0f), glm::vec3(0, 1, 0));
        baseModel = glm::scale(baseModel, glm::vec3(cardScale));

        float progress = m_cardsAnimPhase;
        float collapseEase = progress * progress * (3.0f - 2.0f * progress); // smoothstep
        double tnow = glfwGetTime();

        for (size_t i = 0; i < m_cardsSubmeshes.size(); ++i) {
            const auto& sm = m_cardsSubmeshes[i];
            glm::mat4 model = baseModel;

            if (m_cardsCollapsed) {
                // --- Modo colapsado: cartas desordenadas ---
                float maxX = tableHalfX * 0.04f;
                float maxZ = tableHalfZ * 0.04f;

                // Desplazamientos horizontales pseudoaleatorios fijos por carta
                float spreadX = sin((float)i * 2.0f) * maxX;
                float spreadZ = cos((float)i * 3.0f) * maxZ;

                // Rotación sobre Y aleatoria fija
                float rotY = (float)i * 1.5f; // radianes

                // Pequeñas inclinaciones para dar sensación de desorden
                float tiltX = sin((float)i * 2.5f) * glm::radians(5.0f);
                float tiltZ = cos((float)i * 4.0f) * glm::radians(5.0f);

                // Aplicar transformaciones interpoladas por collapseEase (suavizado)
                model = glm::translate(model, glm::vec3(spreadX * collapseEase, 0.0f, spreadZ * collapseEase));
                model = glm::rotate(model, rotY * collapseEase, glm::vec3(0, 1, 0));
                model = glm::rotate(model, tiltX * collapseEase, glm::vec3(1, 0, 0));
                model = glm::rotate(model, tiltZ * collapseEase, glm::vec3(0, 0, 1));
            }
            else {
                // --- Modo reconstrucción: pequeño rebote vertical ---
                float bounce = (1.0f - collapseEase) * 0.01f * sin((float)tnow * 6.0f + (float)i);
                model = glm::translate(model, glm::vec3(0.0f, bounce, 0.0f));
            }

            glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

            // --- Enlace de texturas por submalla ---
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, sm.texDiffuse ? sm.texDiffuse : m_cardsTexture);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, sm.texAmbient);
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, sm.texSpecular);

                // Aplicar coeficientes de material de la submalla (si los tiene)
                glUniform3f(glGetUniformLocation(m_shaderProgram, "materialAmbient"), sm.Ka.r, sm.Ka.g, sm.Ka.b);
                glUniform3f(glGetUniformLocation(m_shaderProgram, "materialDiffuse"), sm.Kd.r, sm.Kd.g, sm.Kd.b);
                glUniform3f(glGetUniformLocation(m_shaderProgram, "materialSpecular"), sm.Ks.r, sm.Ks.g, sm.Ks.b);
                glUniform1f(glGetUniformLocation(m_shaderProgram, "materialShininess"), sm.Ns);

                glUniform1i(glGetUniformLocation(m_shaderProgram, "useTexture"), (sm.texDiffuse != 0 && sm.hasTexCoords) ? 1 : 0);
                glUniform1i(glGetUniformLocation(m_shaderProgram, "hasAmbientMap"), sm.texAmbient != 0 ? 1 : 0);
                glUniform1i(glGetUniformLocation(m_shaderProgram, "hasSpecularMap"), sm.texSpecular != 0 ? 1 : 0);

            glBindVertexArray(sm.vao);
            glDrawArrays(GL_TRIANGLES, 0, sm.vertexCount);
        }
    }
    else if (m_cardsVertexCount > 0 && m_cardsVAO != 0) {
        // Fallback: renderizado con un solo VAO (sin submeshes)
        glUniform1i(glGetUniformLocation(m_shaderProgram, "isReflective"), 0);
        glUniform3f(glGetUniformLocation(m_shaderProgram, "materialAmbient"), 0.05f, 0.05f, 0.05f);
        glUniform3f(glGetUniformLocation(m_shaderProgram, "materialDiffuse"), 0.85f, 0.85f, 0.85f);
        glUniform3f(glGetUniformLocation(m_shaderProgram, "materialSpecular"), 0.2f, 0.2f, 0.2f);
        glUniform1f(glGetUniformLocation(m_shaderProgram, "materialShininess"), 32.0f);

        float cardHeightModel = (m_cardsMaxY - m_cardsMinY);
        float cardScale = (cardHeightModel > 0.0001f) ? (tableHeight * 0.35f) / cardHeightModel : 1.0f;
        float cardDisplacementY = -m_cardsMinY * cardScale + tableHeight + 0.01f;

        glm::mat4 cardModel = glm::translate(glm::mat4(1.0f), glm::vec3(tableHalfX - 6.0f, cardDisplacementY, 0.0f));
        cardModel = glm::rotate(cardModel, glm::radians(90.0f), glm::vec3(0, 1, 0));
        cardModel = glm::scale(cardModel, glm::vec3(cardScale));
        glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(cardModel));
        glBindTexture(GL_TEXTURE_2D, m_cardsTexture);
        glUniform1i(glGetUniformLocation(m_shaderProgram, "useTexture"), (m_cardsTexture != 0 && m_cardsHasTexCoords) ? 1 : 0);
        glBindVertexArray(m_cardsVAO);
        glDrawArrays(GL_TRIANGLES, 0, m_cardsVertexCount);
    }

    // Coffee (dos tazas juntas)
    //if (m_coffeeVertexCount > 0 && m_coffeeVAO != 0) {
    //    glUniform1i(glGetUniformLocation(m_shaderProgram, "isReflective"), 0);
    //    // Usar materiales leídos al cargar el OBJ (si están disponibles)
    //    glUniform3f(glGetUniformLocation(m_shaderProgram, "materialAmbient"), m_coffeeKa.r, m_coffeeKa.g, m_coffeeKa.b);
    //    glUniform3f(glGetUniformLocation(m_shaderProgram, "materialDiffuse"), m_coffeeKd.r, m_coffeeKd.g, m_coffeeKd.b);
    //    glUniform3f(glGetUniformLocation(m_shaderProgram, "materialSpecular"), m_coffeeKs.r, m_coffeeKs.g, m_coffeeKs.b);
    //    glUniform1f(glGetUniformLocation(m_shaderProgram, "materialShininess"), m_coffeeNs);

    //    float coffeeHeightModel = (m_coffeeMaxY - m_coffeeMinY);
    //    float coffeeScale = (coffeeHeightModel > 0.0001f) ? (tableHeight * 0.10f) / coffeeHeightModel : 1.0f;
    //    float coffeeDisplacementY = -m_coffeeMinY * coffeeScale + tableHeight + 0.01f;

    //    glm::mat4 coffeeModel1 = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, coffeeDisplacementY, tableHalfZ - 6.0f + 2.0f));
    //    coffeeModel1 = glm::scale(coffeeModel1, glm::vec3(coffeeScale));
    //    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(coffeeModel1));
    //    glBindTexture(GL_TEXTURE_2D, m_coffeeTexture);
    //    glUniform1i(glGetUniformLocation(m_shaderProgram, "useTexture"), (m_coffeeTexture != 0 && m_coffeeHasTexCoords) ? 1 : 0);
    //    glBindVertexArray(m_coffeeVAO);
    //    glDrawArrays(GL_TRIANGLES, 0, m_coffeeVertexCount);
    //}
    // Coffee (tazas de café) - con submallas
    if (!m_coffeeSubmeshes.empty()) {
        glUniform1i(glGetUniformLocation(m_shaderProgram, "isReflective"), 0);

        float coffeeHeightModel = (m_coffeeMaxY - m_coffeeMinY);
        float coffeeScale = (coffeeHeightModel > 0.0001f) ? (tableHeight * 0.10f) / coffeeHeightModel : 1.0f;
        float coffeeDisplacementY = -m_coffeeMinY * coffeeScale + tableHeight + 0.01f;

        glm::mat4 coffeeModelBase = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, coffeeDisplacementY, tableHalfZ - 6.0f + 2.0f));
        coffeeModelBase = glm::scale(coffeeModelBase, glm::vec3(coffeeScale));

        for (const auto& sm : m_coffeeSubmeshes) {
            glm::mat4 model = coffeeModelBase; // misma transformación para todas las partes
            glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

            // Coeficientes de material desde la submalla
            glUniform3f(glGetUniformLocation(m_shaderProgram, "materialAmbient"), sm.Ka.r, sm.Ka.g, sm.Ka.b);
            glUniform3f(glGetUniformLocation(m_shaderProgram, "materialDiffuse"), sm.Kd.r, sm.Kd.g, sm.Kd.b);
            glUniform3f(glGetUniformLocation(m_shaderProgram, "materialSpecular"), sm.Ks.r, sm.Ks.g, sm.Ks.b);
            glUniform1f(glGetUniformLocation(m_shaderProgram, "materialShininess"), sm.Ns);

            // Texturas
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, sm.texDiffuse ? sm.texDiffuse : 0);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, sm.texAmbient);
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, sm.texSpecular);

            glUniform1i(glGetUniformLocation(m_shaderProgram, "useTexture"), (sm.texDiffuse != 0 && sm.hasTexCoords) ? 1 : 0);
            glUniform1i(glGetUniformLocation(m_shaderProgram, "hasAmbientMap"), sm.texAmbient != 0 ? 1 : 0);
            glUniform1i(glGetUniformLocation(m_shaderProgram, "hasSpecularMap"), sm.texSpecular != 0 ? 1 : 0);

            glBindVertexArray(sm.vao);
            glDrawArrays(GL_TRIANGLES, 0, sm.vertexCount);
        }
    }

    // Bowl de Frutas
    if (!m_bowlSubmeshes.empty()) {
        glUniform1i(glGetUniformLocation(m_shaderProgram, "isReflective"), 0);
        glUniform3f(glGetUniformLocation(m_shaderProgram, "materialAmbient"), 0.06f, 0.06f, 0.06f);
        glUniform3f(glGetUniformLocation(m_shaderProgram, "materialDiffuse"), 0.95f, 0.95f, 0.95f);
        glUniform3f(glGetUniformLocation(m_shaderProgram, "materialSpecular"), 0.2f, 0.2f, 0.2f);
        glUniform1f(glGetUniformLocation(m_shaderProgram, "materialShininess"), 32.0f);

        float bowlHeightModel = (m_bowlMaxY - m_bowlMinY);
        float bowlScale = (bowlHeightModel > 0.0001f) ? (tableHeight * 0.15f) / bowlHeightModel : 1.0f;
        float bowlDisplacementY = -m_bowlMinY * bowlScale + tableHeight + 0.01f;

        glm::mat4 bowlModelBase = glm::translate(glm::mat4(1.0f), glm::vec3(-tableHalfX + 6.0f, bowlDisplacementY, -4.0f));
        bowlModelBase = glm::scale(bowlModelBase, glm::vec3(bowlScale));

        double tnow = glfwGetTime();
        for (size_t i = 0; i < m_bowlSubmeshes.size(); ++i) {
            const auto &sm = m_bowlSubmeshes[i];
            glm::mat4 model = bowlModelBase;
            if (sm.isFruit) {
                float rise = sin((float)tnow * 3.0f + (float)i) * 0.02f + 0.02f;
                model = glm::translate(model, glm::vec3(0.0f, rise, 0.0f));
                model = glm::rotate(model, sin((float)tnow * 1.5f + (float)i) * glm::radians(4.0f), glm::vec3(0, 0, 1));
            }
            glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, sm.texDiffuse);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, sm.texAmbient);
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, sm.texSpecular);

            // Aplicar coeficientes de material de la submalla (frutas o bowl)
            glUniform3f(glGetUniformLocation(m_shaderProgram, "materialAmbient"), sm.Ka.r, sm.Ka.g, sm.Ka.b);
            glUniform3f(glGetUniformLocation(m_shaderProgram, "materialDiffuse"), sm.Kd.r, sm.Kd.g, sm.Kd.b);
            glUniform3f(glGetUniformLocation(m_shaderProgram, "materialSpecular"), sm.Ks.r, sm.Ks.g, sm.Ks.b);
            glUniform1f(glGetUniformLocation(m_shaderProgram, "materialShininess"), sm.Ns);

            glUniform1i(glGetUniformLocation(m_shaderProgram, "useTexture"), (sm.texDiffuse != 0 && sm.hasTexCoords) ? 1 : 0);
            glUniform1i(glGetUniformLocation(m_shaderProgram, "hasAmbientMap"), sm.texAmbient != 0 ? 1 : 0);
            glUniform1i(glGetUniformLocation(m_shaderProgram, "hasSpecularMap"), sm.texSpecular != 0 ? 1 : 0);

            glBindVertexArray(sm.vao);
            glDrawArrays(GL_TRIANGLES, 0, sm.vertexCount);
        }
    }

    // Cup (taza)
    if (!m_cupSubmeshes.empty()) {
        glUniform1i(glGetUniformLocation(m_shaderProgram, "isReflective"), 0); // No reflectante

        float cupHeightModel = (m_cupMaxY - m_cupMinY);
        float cupScale = (cupHeightModel > 0.0001f) ? (tableHeight * 0.12f) / cupHeightModel : 1.0f;
        float cupDisplacementY = -m_cupMinY * cupScale + tableHeight + 0.01f;

        glm::mat4 cupModelBase = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, cupDisplacementY, -tableHalfZ + 3.0f));
        cupModelBase = glm::scale(cupModelBase, glm::vec3(cupScale));

        for (size_t i = 0; i < m_cupSubmeshes.size(); ++i) {
            const auto &sm = m_cupSubmeshes[i];
            glm::mat4 model = cupModelBase; // misma transformación para todas las partes
            glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

            // --- Coeficientes de material desde la submalla ---
            glUniform3f(glGetUniformLocation(m_shaderProgram, "materialAmbient"), sm.Ka.r, sm.Ka.g, sm.Ka.b);
            glUniform3f(glGetUniformLocation(m_shaderProgram, "materialDiffuse"), sm.Kd.r, sm.Kd.g, sm.Kd.b);
            glUniform3f(glGetUniformLocation(m_shaderProgram, "materialSpecular"), sm.Ks.r, sm.Ks.g, sm.Ks.b);
            glUniform1f(glGetUniformLocation(m_shaderProgram, "materialShininess"), sm.Ns);

            // Texturas
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, sm.texDiffuse ? sm.texDiffuse : 0);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, sm.texAmbient);
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, sm.texSpecular);

            glUniform1i(glGetUniformLocation(m_shaderProgram, "useTexture"), (sm.texDiffuse != 0 && sm.hasTexCoords) ? 1 : 0);
            glUniform1i(glGetUniformLocation(m_shaderProgram, "hasAmbientMap"), sm.texAmbient != 0 ? 1 : 0);
            glUniform1i(glGetUniformLocation(m_shaderProgram, "hasSpecularMap"), sm.texSpecular != 0 ? 1 : 0);

            glBindVertexArray(sm.vao);
            glDrawArrays(GL_TRIANGLES, 0, sm.vertexCount);
        }
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

    // Actualización del viewport cada vez que haya un resize
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
    glUseProgram(m_shaderProgram);
    GLint locTex = glGetUniformLocation(m_shaderProgram, "texture_diffuse");
    if (locTex >= 0) glUniform1i(locTex, 0);
    GLint locSky = glGetUniformLocation(m_shaderProgram, "skybox");
    if (locSky >= 0) glUniform1i(locSky, 1);
    GLint locAmb = glGetUniformLocation(m_shaderProgram, "texture_ambient");
    if (locAmb >= 0) glUniform1i(locAmb, 2);
    GLint locSpec = glGetUniformLocation(m_shaderProgram, "texture_specular");
    if (locSpec >= 0) glUniform1i(locSpec, 3);
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
    // Definición de un cubo completo para la mesa (posiciones, normales, color)
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

    // Atributo Posicion (0)
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
    // Vertices para un cubo que nos rodea (solo posicion)
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

    // Para el Skybox solo se necesita la posicion (layout 0)
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
    // Indices para triangulos
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
    // Shader sencillo para las esferas de luz
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

    std::cout << "Modelo cargado con exito (loadOBJ)" << std::endl;
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

    // Calculo de los límites en Y
    float minY = std::numeric_limits<float>::max();
    float maxY = -std::numeric_limits<float>::max();
    for (const auto& v : vertices) {
        if (v.Position.y < minY) minY = v.Position.y;
        if (v.Position.y > maxY) maxY = v.Position.y;
    }
        // Calculo de normales si el OBJ no las proporciona
        if (!hasNormals && vertices.size() >= 3) {
            for (size_t i = 0; i + 2 < vertices.size(); i += 3) {
                glm::vec3 v0 = vertices[i + 0].Position;
                glm::vec3 v1 = vertices[i + 1].Position;
                glm::vec3 v2 = vertices[i + 2].Position;
                glm::vec3 faceNormal = glm::normalize(glm::cross(v1 - v0, v2 - v0));
                if (glm::length(faceNormal) > 0.0001f) {
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
        }

    // Creacion de búfer de GPU
    m_tableMaxY = maxY;
    std::cout << "Altura de la mesa: minY = " << minY << ", maxY = " << maxY << ", altura total = " << (maxY - minY) << std::endl;

	// Calculo de vertices de normales si no se proporcionan en el OBJ
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
        for (size_t i = 0; i < vertices.size(); ++i) {
            if (glm::length(vertices[i].Normal) > 0.0001f)
                vertices[i].Normal = glm::normalize(vertices[i].Normal);
            else
                vertices[i].Normal = glm::vec3(0.0f, 1.0f, 0.0f);
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

    // Posicion
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
    std::cout << "Intentando cargar textura desde: " << path << std::endl;
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
    int targetFaceSize = -1;
    bool allFacesLoaded = true;
    stbi_set_flip_vertically_on_load(false);
    if (faces.size() != 6) {
        std::cerr << "Warning: cubemap expects 6 faces, got " << faces.size() << std::endl;
    }
    for (unsigned int i = 0; i < faces.size(); i++) {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, STBI_rgb_alpha);
        if (data) {
            int curMin = std::min(width, height);
            if (targetFaceSize < 0) targetFaceSize = curMin;

            unsigned char* uploadData = data;
            bool allocatedTemp = false;
            if (width != targetFaceSize || height != targetFaceSize) {
                unsigned char* temp = resizeRGBA(data, width, height, targetFaceSize, targetFaceSize);
                if (temp) {
                    uploadData = temp;
                    allocatedTemp = true;
                }
                else {
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
                }
            }

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

void C3DViewer::loadOBJTo(const std::string& path, GLuint& outVAO, GLuint& outVBO, size_t& outVertexCount, bool& outHasTexCoords, float& outMinY, float& outMaxY, GLuint* outTexture, glm::vec3* outKa, glm::vec3* outKd, glm::vec3* outKs, float* outNs) {
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

    std::cout << "Modelo cargado con exito (loadOBJTo)" << std::endl;
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

    // Si el modelo no tiene UVs, asignar un color sólido
    if (!hasTexCoords) {
        glm::vec3 coffeeColor = glm::vec3(0.9f, 0.8f, 0.7f); // tono crema/café
        for (auto& v : vertices) {
            v.Color = coffeeColor;
        }
    }

    float minY = std::numeric_limits<float>::max();
    float maxY = -std::numeric_limits<float>::max();
    for (const auto& v : vertices) {
        if (v.Position.y < minY) minY = v.Position.y;
        if (v.Position.y > maxY) maxY = v.Position.y;
    }
    outMinY = minY;
    outMaxY = maxY;
    std::cout << "Altura del OBJ: minY = " << minY << ", maxY = " << maxY << ", altura total = " << (maxY - minY) << std::endl;

    // Si se solicita, intentar cargar la textura diffuse desde el material asociado
    if (outTexture) {
        *outTexture = 0;
        if (!materials.empty()) {
            int chosenMat = -1;
            int bestCount = -1;
            
            // Primero, buscar materiales que tengan textura difusa
            for (size_t i = 0; i < materials.size(); ++i) {
                if (!materials[i].diffuse_texname.empty()) {
                    // Contar cuántas caras usan este material (para elegir el más usado entre los texturizados)
                    int count = 0;
                    for (const auto &shape : shapes) {
                        for (int mid : shape.mesh.material_ids) {
                            if (mid == (int)i) count++;
                        }
                    }
                    if (count > bestCount) {
                        bestCount = count;
                        chosenMat = i;
                    }
                }
            }
            
            // Si no se encontró ningún material con textura, usar el material más frecuente en general
            if (chosenMat == -1) {
                std::unordered_map<int,int> counts;
                for (const auto &shape : shapes) {
                    for (int mid : shape.mesh.material_ids) counts[mid]++;
                }
                bestCount = -1;
                for (auto &kv : counts) {
                    if (kv.second > bestCount && kv.first >= 0) {
                        bestCount = kv.second;
                        chosenMat = kv.first;
                    }
                }
                // Fallback al primer material
                if (chosenMat == -1) chosenMat = 0;
            }
            
            if (chosenMat >= 0 && chosenMat < (int)materials.size()) {
                const auto &mat = materials[chosenMat];
                if (!mat.diffuse_texname.empty()) {
                    std::string texPath = baseDir + mat.diffuse_texname;
                    GLuint tid = loadTexture(texPath.c_str());
                    if (tid == 0) {
                        std::cout << "Warning: failed to load diffuse texture '" << texPath << "' for OBJ '" << path << "'" << std::endl;
                    } else {
                        std::cout << "Loaded material diffuse for OBJ: " << texPath << std::endl;
                        *outTexture = tid;
                    }
                } else {
                    std::cout << "Material " << chosenMat << " has no diffuse texture." << std::endl;
                }
                // Si se pidieron los parámetros de material, rellenarlos desde tinyobj
                if (outKa) *outKa = glm::vec3(mat.ambient[0], mat.ambient[1], mat.ambient[2]);
                if (outKd) *outKd = glm::vec3(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]);
                if (outKs) *outKs = glm::vec3(mat.specular[0], mat.specular[1], mat.specular[2]);
                if (outNs) *outNs = (mat.shininess > 0.0f) ? mat.shininess : 32.0f;
            }
        }
    }

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

// Carga OBJ generando un Submesh por cada 'shape' del archivo OBJ.
void C3DViewer::loadOBJToMulti(const std::string& path, std::vector<Submesh>& outSubmeshes, float& outMinY, float& outMaxY, bool& outHasTexCoords) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    outSubmeshes.clear();
    std::string baseDir = "";
    size_t pos = path.find_last_of("/\\");
    if (pos != std::string::npos) baseDir = path.substr(0, pos + 1);

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str(), baseDir.c_str())) {
        std::cout << "Error cargando OBJ (multi): " << err << std::endl;
        return;
    }

    std::cout << "Materiales cargados: " << materials.size() << std::endl;
    for (size_t i = 0; i < materials.size(); ++i) {
        std::cout << "  Material " << i << ": " << materials[i].name << " diffuse_tex: " << materials[i].diffuse_texname << std::endl;
    }

    bool anyHasTex = false;
    float globalMinY = std::numeric_limits<float>::max();
    float globalMaxY = -std::numeric_limits<float>::max();

    // Precarga de texturas de materiales
    std::vector<GLuint> matAmbientTex(materials.size(), 0);
    std::vector<GLuint> matDiffuseTex(materials.size(), 0);
    std::vector<GLuint> matSpecularTex(materials.size(), 0);
    for (size_t mi = 0; mi < materials.size(); ++mi) {
        const auto &m = materials[mi];
        if (!m.ambient_texname.empty()) {
            std::string p = baseDir + m.ambient_texname;
            matAmbientTex[mi] = loadTexture(p.c_str());
            if (matAmbientTex[mi] == 0) std::cout << "Warning: failed to load ambient map '" << p << "' for material " << m.name << std::endl;
        }
        if (!m.diffuse_texname.empty()) {
            std::string p = baseDir + m.diffuse_texname;
            matDiffuseTex[mi] = loadTexture(p.c_str());
            if (matDiffuseTex[mi] == 0) std::cout << "Warning: failed to load diffuse map '" << p << "' for material " << m.name << std::endl;
        }
        if (!m.specular_texname.empty()) {
            std::string p = baseDir + m.specular_texname;
            matSpecularTex[mi] = loadTexture(p.c_str());
            if (matSpecularTex[mi] == 0) std::cout << "Warning: failed to load specular map '" << p << "' for material " << m.name << std::endl;
        }
    }

    std::vector<glm::vec3> matKa(materials.size(), glm::vec3(0.02f));
    std::vector<glm::vec3> matKd(materials.size(), glm::vec3(0.8f));
    std::vector<glm::vec3> matKs(materials.size(), glm::vec3(0.2f));
    std::vector<float> matNs(materials.size(), 32.0f);

    for (size_t mi = 0; mi < materials.size(); ++mi) {
        const auto &m = materials[mi];
        matKa[mi] = glm::vec3(m.ambient[0], m.ambient[1], m.ambient[2]);
        matKd[mi] = glm::vec3(m.diffuse[0], m.diffuse[1], m.diffuse[2]);
        matKs[mi] = glm::vec3(m.specular[0], m.specular[1], m.specular[2]);
        matNs[mi] = (m.shininess > 0.0f) ? m.shininess : 32.0f; // si es 0, valor por defecto
    }

    for (size_t s = 0; s < shapes.size(); ++s) {
        const auto& shape = shapes[s];
        std::vector<Vertex> vertices;
        bool hasTex = false;
        bool hasNormals = false;

        // Material para la forma (en caso de existir)
        int chosenMat = -1;
        if (!shape.mesh.material_ids.empty()) {
            std::unordered_map<int,int> counts;
            size_t faceIdx = 0;
            for (int mid : shape.mesh.material_ids) {
                counts[mid]++;
                ++faceIdx;
            }
            int best = -1; int bestCount = -1;
            for (auto &kv : counts) {
                if (kv.second > bestCount) { best = kv.first; bestCount = kv.second; }
            }
            chosenMat = best;
        }
        // Si no hay material asignado, intentar usar el primer material con textura
        if (chosenMat == -1 && !materials.empty()) {
            for (size_t i = 0; i < materials.size(); ++i) {
                if (!materials[i].diffuse_texname.empty()) {
                    chosenMat = i;
                    break;
                }
            }
        }

        // Construcción de vértices para esta forma
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
                hasTex = true;
                vertex.TexCoords = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    attrib.texcoords[2 * index.texcoord_index + 1]
                };
            }
            vertex.Color = glm::vec3(1.0f);
            vertices.push_back(vertex);
        }

        // Cálculo de normales si no son proporcionadas por el OBJ
        if (!hasNormals && vertices.size() >= 3) {
            std::vector<glm::vec3> accumulatedNormals(vertices.size(), glm::vec3(0.0f));
            for (size_t i = 0; i + 2 < vertices.size(); i += 3) {
                glm::vec3 v0 = vertices[i + 0].Position;
                glm::vec3 v1 = vertices[i + 1].Position;
                glm::vec3 v2 = vertices[i + 2].Position;
                glm::vec3 faceNormal = glm::normalize(glm::cross(v1 - v0, v2 - v0));
                if (glm::length(faceNormal) > 0.0001f) {
                    accumulatedNormals[i + 0] += faceNormal;
                    accumulatedNormals[i + 1] += faceNormal;
                    accumulatedNormals[i + 2] += faceNormal;
                }
            }
            for (size_t i = 0; i < vertices.size(); ++i) {
                if (glm::length(accumulatedNormals[i]) > 0.0001f)
                    vertices[i].Normal = glm::normalize(accumulatedNormals[i]);
                else
                    vertices[i].Normal = glm::vec3(0.0f, 1.0f, 0.0f); // normal por defecto
            }
        }

        // Si no hay material asignado, dar color por vértice a las cartas
        if (chosenMat == -1) {
            static const glm::vec3 cardColors[] = {
                glm::vec3(1.0f, 0.8f, 0.8f), // rosa claro
                glm::vec3(0.8f, 1.0f, 0.8f), // verde claro
                glm::vec3(0.8f, 0.8f, 1.0f), // azul claro
                glm::vec3(1.0f, 1.0f, 0.8f), // amarillo claro
                glm::vec3(1.0f, 0.8f, 1.0f), // magenta claro
                glm::vec3(0.8f, 1.0f, 1.0f), // cian claro
            };
            glm::vec3 color = cardColors[s % (sizeof(cardColors) / sizeof(cardColors[0]))];
            for (auto& v : vertices) {
                v.Color = color;
            }
        }

		// Valor minY/max Y para esta forma
        float minY = std::numeric_limits<float>::max();
        float maxY = -std::numeric_limits<float>::max();
        for (const auto &v : vertices) {
            if (v.Position.y < minY) minY = v.Position.y;
            if (v.Position.y > maxY) maxY = v.Position.y;
        }
        if (minY < globalMinY) globalMinY = minY;
        if (maxY > globalMaxY) globalMaxY = maxY;

        // Buffers de GPU
        Submesh sm;
        sm.vertexCount = vertices.size();
        sm.hasTexCoords = hasTex;
        sm.minY = minY; sm.maxY = maxY;
        sm.materialId = chosenMat;
        sm.name = shape.name;

        glGenVertexArrays(1, &sm.vao);
        glGenBuffers(1, &sm.vbo);
        glBindVertexArray(sm.vao);
        glBindBuffer(GL_ARRAY_BUFFER, sm.vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Color));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

        if (chosenMat >= 0 && chosenMat < (int)materials.size()) {
            sm.texAmbient = matAmbientTex[chosenMat];
            sm.texDiffuse = matDiffuseTex[chosenMat];
            sm.texSpecular = matSpecularTex[chosenMat];
            sm.Ka = matKa[chosenMat];
            sm.Kd = matKd[chosenMat];
            sm.Ks = matKs[chosenMat];
            sm.Ns = matNs[chosenMat];
            if (sm.texDiffuse == 0 && sm.hasTexCoords) {
                const auto &mat = materials[chosenMat];
                if (!mat.diffuse_texname.empty()) {
                    std::string p = baseDir + mat.diffuse_texname;
                    sm.texDiffuse = loadTexture(p.c_str());
                }
            }
        }

        std::cout << "Submesh loaded: '" << sm.name << "' material=" << sm.materialId
            << " hasTexCoords=" << (sm.hasTexCoords?"yes":"no")
            << " texDiffuse=" << sm.texDiffuse << " texAmbient=" << sm.texAmbient
            << " texSpecular=" << sm.texSpecular << std::endl;
        outSubmeshes.push_back(sm);
    }

    outMinY = globalMinY;
    outMaxY = globalMaxY;
    outHasTexCoords = false;
    for (auto &s : outSubmeshes) if (s.hasTexCoords) { outHasTexCoords = true; break; }

	// Marcar submeshes que podrían ser frutas basándonos en su altura relativa al mínimo global
    float totalHeight = outMaxY - outMinY;
    for (size_t i = 0; i < outSubmeshes.size(); ++i) {
        float centroidY = (outSubmeshes[i].minY + outSubmeshes[i].maxY) * 0.5f;
        outSubmeshes[i].isFruit = (centroidY > outMinY + totalHeight * 0.25f);
    }

    std::cout << "Loaded multi-shape OBJ: shapes=" << outSubmeshes.size() << " globalMinY=" << outMinY << " globalMaxY=" << outMaxY << std::endl;
}

// Actualiza animaciones simples (cartas)
void C3DViewer::updateCardsAnimation(double deltaTime) {
    m_cardsAnimTimer += (float)deltaTime;
    if (m_cardsAnimTimer >= m_cardsCyclePeriod) {
        m_cardsAnimTimer = 0.0f;
        m_cardsCollapsed = !m_cardsCollapsed;
        m_cardsAnimPhase = 0.0f;
    }
    else {
        m_cardsAnimPhase = m_cardsAnimTimer / m_cardsCyclePeriod;
    }
}

void C3DViewer::updateTeapotAnimation(double deltaTime) {
    const float moveFactor = 0.32f; // Desplazamiento
    const float targetYaw = glm::radians(-90.0f);

    m_teapotAnimTimer += (float)deltaTime;
    if (m_teapotAnimTimer >= m_teapotCyclePeriod) {
        m_teapotAnimTimer = 0.0f;
        m_teapotAnimStage = 0;
    }

    // Duración de animación (40 segs)
    const float stageDurations[] = {
        2.0f, // 0: levantar
        3.0f, // 1: desplazar hacia taza
        2.0f, // 2: rotar para apuntar
        2.0f, // 3: inclinar
        2.0f, // 4: volver vertical
        3.0f, // 5: regresar
        2.0f, // 6: bajar
        24.0f // 7: reposo
    };
    const int numStages = sizeof(stageDurations) / sizeof(float);

    float elapsed = m_teapotAnimTimer;
    int stage = 0;
    while (stage < numStages && elapsed > stageDurations[stage]) {
        elapsed -= stageDurations[stage];
        stage++;
    }
    if (stage >= numStages) stage = numStages - 1;
    m_teapotAnimStage = stage;
    float progress = elapsed / stageDurations[stage]; // 0..1

    // Parametros de la mesa
    float scale = 0.5f;
    float tableHeight = scale * (m_tableMaxY - m_tableMinY);
    float tableHalfZ = ((m_tableMaxZ - m_tableMinZ) * 0.5f) * scale;

    // Posicion de la taza cup
    float cupHeightModel = (m_cupMaxY - m_cupMinY);
    float cupScale = (cupHeightModel > 0.0001f) ? (tableHeight * 0.12f) / cupHeightModel : 1.0f;
    float cupDisplacementY = -m_cupMinY * cupScale + tableHeight + 0.01f;
    glm::vec3 cupPos(0.0f, cupDisplacementY, -tableHalfZ + 3.0f);

    // Altura base de la tetera (sin animación)
    float teapotHeightModel = (m_teapotMaxY - m_teapotMinY);
    float teapotScale = (teapotHeightModel > 0.0001f) ? (tableHeight * 0.30f) / teapotHeightModel : 1.0f;
    float teapotBaseY = -m_teapotMinY * teapotScale + tableHeight + 0.02f;

    const float liftHeight = 0.5f;        // cuánto se eleva
    const float maxPitch = glm::radians(30.0f); // inclinación máxima

    glm::vec3 targetExtraPos(0.0f);
    float targetExtraPitch = 0.0f;
    float targetExtraYaw = 0.0f;

    switch (stage) {
    case 0: // levantar
        targetExtraPos.y = liftHeight * progress;
        targetExtraYaw = 0.0f;
        break;
    case 1: // desplazar hacia la taza
        targetExtraPos.y = liftHeight;
        targetExtraPos.z = cupPos.z * moveFactor * progress;
        targetExtraYaw = 0.0f;
        break;
    case 2: // rotar para apuntar
        targetExtraPos.y = liftHeight;
        targetExtraPos.z = cupPos.z * moveFactor;
        targetExtraYaw = targetYaw * progress;
        break;
    case 3: // inclinar
        targetExtraPos.y = liftHeight;
        targetExtraPos.z = cupPos.z * moveFactor;
        targetExtraYaw = targetYaw;
        targetExtraPitch = maxPitch * progress;
        break;
    case 4: // volver vertical
        targetExtraPos.y = liftHeight;
        targetExtraPos.z = cupPos.z * moveFactor;
        targetExtraYaw = targetYaw;
        targetExtraPitch = maxPitch * (1.0f - progress);
        break;
    case 5: // regresar
        targetExtraPos.y = liftHeight;
        targetExtraPos.z = cupPos.z * moveFactor * (1.0f - progress);
        targetExtraYaw = targetYaw * (1.0f - progress);
        break;
    case 6: // bajar
        targetExtraPos.y = liftHeight * (1.0f - progress);
        targetExtraYaw = 0.0f;
        break;
    case 7: // reposo
        targetExtraPos = glm::vec3(0.0f);
        targetExtraYaw = 0.0f;
        targetExtraPitch = 0.0f;
        break;
    }

    m_teapotExtraPos = targetExtraPos;
    m_teapotExtraPitch = targetExtraPitch;
    m_teapotExtraYaw = targetExtraYaw;
}