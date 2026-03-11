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
#include "tiny_obj_loader.h"
#include "ModelLoader.h"
#include "Animations.h"

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

    if (m_paramSphereVAO) glDeleteVertexArrays(1, &m_paramSphereVAO);
    if (m_paramSphereVBO) glDeleteBuffers(1, &m_paramSphereVBO);
    if (m_paramSphereEBO) glDeleteBuffers(1, &m_paramSphereEBO);
    if (m_sphereDiffuseTexture) glDeleteTextures(1, &m_sphereDiffuseTexture);
    if (m_sphereBumpTexture) glDeleteTextures(1, &m_sphereBumpTexture);

    if (m_bboxVBO) glDeleteBuffers(1, &m_bboxVBO);
    if (m_bboxVAO) glDeleteVertexArrays(1, &m_bboxVAO);

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

    // Inicializacion de ImGui
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

    // -- Pantalla de Carga --

    const char* stages[] = {
        "Inicializando...",
        "Cargando mesa...",
        "Cargando textura de mesa...",
        "Cargando tetera...",
        "Cargando tazas de café...",
        "Cargando taza...",
        "Cargando bowl de frutas...",
        "Cargando cartas...",
        "Cargando skybox...",
        "Cargando esfera paramétrica...",
        "Finalizando..."
    };
    int totalStages = sizeof(stages) / sizeof(stages[0]);

    // Funcion lambda para actualizar la pantalla de carga
    auto updateLoadingScreen = [&](int stage, const char* message) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Ventana de carga centrada
        ImGui::SetNextWindowPos(ImVec2(width / 2 - 200, height / 2 - 60), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(400, 120), ImGuiCond_Always);
        ImGui::Begin("Cargando", nullptr,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoSavedSettings);

        ImGui::Text("Cargando escena 3D...");
        ImGui::Spacing(); ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing(); ImGui::Spacing();
        ImGui::Text("%s", message);

        static float progress = 0.0f;
        progress = fmodf(progress + 9.1f, 1.0f);
        
        // Color de la barra
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, IM_COL32(142, 109, 232, 255));
        ImGui::ProgressBar(progress, ImVec2(380, 20), "Cargando...");
        ImGui::PopStyleColor();

        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(m_window);
        glfwPollEvents();
    };

    // Primera pantalla
    int currentStage = 1;
    updateLoadingScreen(currentStage, stages[currentStage - 1]);

    // -- Carga de Recursos -- 

    // Setup shader
    if (!setupShader()) return false;

    // Mesa OBJ
    std::cout << "--- Iniciando carga de: OBJs/table/table4.obj ---" << std::endl;
    loadOBJTo("OBJs/table/table4.obj", m_tableVAO, m_tableVBO, m_tableVertexCount, m_tableHasTexCoords, m_tableMinY, m_tableMaxY, m_tableMinX, m_tableMaxX, m_tableMinZ, m_tableMaxZ);
    currentStage++;
    updateLoadingScreen(currentStage, stages[currentStage - 1]);
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
    loadOBJToMulti("OBJs/teapot/teapot.obj", m_teapotSubmeshes, m_teapotMinY, m_teapotMaxY, m_teapotMinX, m_teapotMaxX, m_teapotMinZ, m_teapotMaxZ, m_teapotHasTexCoords);
    currentStage++;
    updateLoadingScreen(currentStage, stages[currentStage - 1]);

    // Carga de tazas de cafe
    std::cout << "--- Iniciando carga de: OBJs/coffee/coffee.obj ---" << std::endl;
    m_coffeeSubmeshes.clear();
    loadOBJToMulti("OBJs/coffee/coffee.obj", m_coffeeSubmeshes, m_coffeeMinY, m_coffeeMaxY, m_coffeeMinX, m_coffeeMaxX, m_coffeeMinZ, m_coffeeMaxZ, m_coffeeHasTexCoords);
    m_coffeeSpoonExtraTransforms.resize(2, glm::mat4(1.0f));
    currentStage++;
    updateLoadingScreen(currentStage, stages[currentStage - 1]);

    // Carga de taza
    std::cout << "--- Iniciando carga de: OBJs/cup/cup.obj ---" << std::endl;
    m_cupSubmeshes.clear();
    loadOBJToMulti("OBJs/cup/cup.obj", m_cupSubmeshes, m_cupMinY, m_cupMaxY, m_cupMinX, m_cupMaxX, m_cupMinZ, m_cupMaxZ, m_cupHasTexCoords);
    currentStage++;
    updateLoadingScreen(currentStage, stages[currentStage - 1]);

    // Bowl: Cada fruta es una submalla
    m_bowlSubmeshes.clear();
    loadOBJToMulti("OBJs/fruits/bowlWithFruits.obj", m_bowlSubmeshes, m_bowlMinY, m_bowlMaxY, m_bowlMinX, m_bowlMaxX, m_bowlMinZ, m_bowlMaxZ, m_bowlHasTexCoords);
    currentStage++;
    updateLoadingScreen(currentStage, stages[currentStage - 1]);

    // Cards: Cada carta es una submalla
    std::cout << "--- Iniciando carga de: OBJs/cards/cards.obj ---" << std::endl;
    m_cardsSubmeshes.clear();
    loadOBJToMulti("OBJs/cards/cards.obj", m_cardsSubmeshes, m_cardsMinY, m_cardsMaxY, m_cardsMinX, m_cardsMaxX, m_cardsMinZ, m_cardsMaxZ, m_cardsHasTexCoords);
    currentStage++;
    updateLoadingScreen(currentStage, stages[currentStage - 1]);

    if (!setupSimpleShader()) return false;

    // Buffers para dibujar bounding boxes (lineas)
    glGenVertexArrays(1, &m_bboxVAO);
    glGenBuffers(1, &m_bboxVBO);
    glBindVertexArray(m_bboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_bboxVBO);
    // Espacio para 24 vértices (12 aristas * 2) de vec3
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * 24, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);

    // Activacion de cubemap
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    glGenTextures(1, &m_reflectionCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_reflectionCubemap);
    for (int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, 512, 512, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    // FBO para renderizar a la cubemap
    glGenFramebuffers(1, &m_reflectionFBO);
    // Depth buffer
    glGenRenderbuffers(1, &m_reflectionDepthRB);
    glBindRenderbuffer(GL_RENDERBUFFER, m_reflectionDepthRB);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);

    // Mesa y Skybox
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

    glBindTexture(GL_TEXTURE_CUBE_MAP, m_skyboxTexture);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    currentStage++;
    updateLoadingScreen(currentStage, stages[currentStage - 1]);

    setupSkybox();
    setupSphere();
    setupParamSphere();

    // Cargar texturas para la esfera paramétrica
    m_sphereDiffuseTexture = loadTexture("OBJs/bumpMapping/xerxes.jpg");
    m_sphereBumpTexture = loadTexture("OBJs/bumpMapping/normalMapTexture.jpg");
    currentStage++;
    updateLoadingScreen(currentStage, stages[currentStage - 1]);

    glEnable(GL_DEPTH_TEST);

    glViewport(0, 0, width, height);

    // Configuracion de callbacks de entrada
    glfwSetWindowUserPointer(m_window, this);
    glfwSetKeyCallback(m_window, keyCallbackStatic);
    glfwSetMouseButtonCallback(m_window, mouseButtonCallbackStatic);
    glfwSetCursorPosCallback(m_window, cursorPosCallbackStatic);

    // Inicializar temporizador de frames y posición del cursor
    m_lastFrame = glfwGetTime();
    lastX = width / 2.0;
    lastY = height / 2.0;

    // Mostrar mensaje final y pausa antes de iniciar escena
    updateLoadingScreen(totalStages, "¡Listo! Iniciando...");
    glfwWaitEventsTimeout(0.5);

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
    updateCoffeeAnimation(deltaTime);

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

    // --- Configuracion de camara ---
    glm::vec3 camPos = cameraPos;
    glm::mat4 view = glm::lookAt(camPos, camPos + cameraFront, cameraUp);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 500.0f);

    // Auxiliar para aplicar texturas segun objeto seleccionado
    auto applySelectionAndBBox = [&](int objectIndex, const glm::mat4& modelMatrix, const glm::vec3& bboxMin, const glm::vec3& bboxMax) {
        glUseProgram(m_shaderProgram);
        GLint locTexGenMode = glGetUniformLocation(m_shaderProgram, "texGenMode");
        GLint locS = glGetUniformLocation(m_shaderProgram, "sMapping");
        GLint locO = glGetUniformLocation(m_shaderProgram, "oMapping");
        GLint locMin = glGetUniformLocation(m_shaderProgram, "bboxMin");
        GLint locMax = glGetUniformLocation(m_shaderProgram, "bboxMax");

        if (globalUIState.selectedObj == objectIndex) {
            glUniform1i(locTexGenMode, globalUIState.texGenMode);
            glUniform1i(locS, globalUIState.sMapping);
            glUniform1i(locO, globalUIState.oMapping);
            glUniform3f(locMin, bboxMin.x, bboxMin.y, bboxMin.z);
            glUniform3f(locMax, bboxMax.x, bboxMax.y, bboxMax.z);

            // --- Dibujar bounding box ---
            glm::vec3 corners[8] = {
                glm::vec3(bboxMin.x, bboxMin.y, bboxMin.z),
                glm::vec3(bboxMax.x, bboxMin.y, bboxMin.z),
                glm::vec3(bboxMax.x, bboxMax.y, bboxMin.z),
                glm::vec3(bboxMin.x, bboxMax.y, bboxMin.z),
                glm::vec3(bboxMin.x, bboxMin.y, bboxMax.z),
                glm::vec3(bboxMax.x, bboxMin.y, bboxMax.z),
                glm::vec3(bboxMax.x, bboxMax.y, bboxMax.z),
                glm::vec3(bboxMin.x, bboxMax.y, bboxMax.z)
            };
            int edges[24] = { 0,1, 1,2, 2,3, 3,0, 4,5, 5,6, 6,7, 7,4, 0,4, 1,5, 2,6, 3,7 };
            float pts[24 * 3];
            for (int i = 0; i < 24; ++i) {
                glm::vec4 w = modelMatrix * glm::vec4(corners[edges[i]], 1.0f);
                pts[i * 3 + 0] = w.x;
                pts[i * 3 + 1] = w.y;
                pts[i * 3 + 2] = w.z;
            }
            glBindBuffer(GL_ARRAY_BUFFER, m_bboxVBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(pts), pts);

            glUseProgram(m_lightShader);
            glUniformMatrix4fv(glGetUniformLocation(m_lightShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(glGetUniformLocation(m_lightShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
            glm::mat4 identity = glm::mat4(1.0f);
            glUniformMatrix4fv(glGetUniformLocation(m_lightShader, "model"), 1, GL_FALSE, glm::value_ptr(identity));
            glUniform3f(glGetUniformLocation(m_lightShader, "color"), 1.0f, 0.9f, 0.2f);
            glBindVertexArray(m_bboxVAO);
            glLineWidth(2.0f);
            glDrawArrays(GL_LINES, 0, 24);
            glLineWidth(1.0f);
            glBindVertexArray(0);

            // Restaurar shader principal
            glUseProgram(m_shaderProgram);
        }
        else {
            // Objetos no seleccionados usan coordenadas originales
            glUniform1i(locTexGenMode, 0);
            // Reseteo de uniforms
            glUniform1i(locS, 0);
            glUniform1i(locO, 0);
        }
        };

    // --- Dibujo de skybox ---
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDepthMask(GL_FALSE);

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
    
    glEnable(GL_DEPTH_TEST);
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

    // Calculos globales de la mesa
    float scale = 0.5f;
    float tableHeight = scale * (m_tableMaxY - m_tableMinY);
    float tableHalfX = ((m_tableMaxX - m_tableMinX) * 0.5f) * scale;
    float tableHalfZ = ((m_tableMaxZ - m_tableMinZ) * 0.5f) * scale;

    // Calculos globales del bowl de frutas
    float bowlHeightModel = (m_bowlMaxY - m_bowlMinY);
    float bowlScale = (bowlHeightModel > 0.0001f) ? (tableHeight * 0.15f) / bowlHeightModel : 1.0f;
    float bowlDisplacementY = -m_bowlMinY * bowlScale + tableHeight + 0.01f;
    glm::vec3 bowlCenter = glm::vec3(-tableHalfX + 6.0f, bowlDisplacementY, -4.0f);

    // Calculos globales de la tetera
    glm::mat4 teapotModel = glm::mat4(1.0f);
    if (!m_teapotSubmeshes.empty()) {
        float teapotHeightModel = (m_teapotMaxY - m_teapotMinY);
        float teapotScale = (teapotHeightModel > 0.0001f) ? (tableHeight * 0.30f) / teapotHeightModel : 1.0f;
        float teapotDisplacementY = -m_teapotMinY * teapotScale + tableHeight + 0.02f;

        teapotModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, teapotDisplacementY, 0.0f));
        teapotModel = glm::scale(teapotModel, glm::vec3(teapotScale));
        // Aplicar animación
        teapotModel = glm::translate(teapotModel, m_teapotExtraPos);
        teapotModel = glm::rotate(teapotModel, m_teapotExtraYaw, glm::vec3(0, 1, 0));
        teapotModel = glm::rotate(teapotModel, -m_teapotExtraPitch, glm::vec3(0, 0, 1));
    }

    // --- Cubemap dinamico ---
    // Guardado del estado actual del framebuffer y viewport
    GLint prevFBO;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFBO);
    GLint prevViewport[4];
    glGetIntegerv(GL_VIEWPORT, prevViewport);

    glm::vec3 teapotPos = glm::vec3(teapotModel[3]); // posición de la tetera en coordenadas mundo

    // Configuracion de proyeccion para cubemap
    glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 500.0f);

    // Matrices de vista para las 6 caras con orientación estándar de OpenGL
    glm::mat4 captureViews[] = {
        // Cara           | Objetivo (Posición + Dirección)          | Vector Up
        glm::lookAt(teapotPos, teapotPos + glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)), // +X (Derecha)
        glm::lookAt(teapotPos, teapotPos + glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)), // -X (Izquierda)
        glm::lookAt(teapotPos, teapotPos + glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)), // +Y (Techo)
        glm::lookAt(teapotPos, teapotPos + glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)), // -Y (Suelo)
        glm::lookAt(teapotPos, teapotPos + glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)), // +Z (Atrás)
        glm::lookAt(teapotPos, teapotPos + glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))  // -Z (Frente)
    };

    glBindFramebuffer(GL_FRAMEBUFFER, m_reflectionFBO);
    glViewport(0, 0, 512, 512); // Resolucion de la cubemap
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    GLboolean wasCull = glIsEnabled(GL_CULL_FACE);
    if (wasCull) glDisable(GL_CULL_FACE);

    // Shader principal
    glUseProgram(m_shaderProgram);

    for (int i = 0; i < 6; ++i) {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_reflectionCubemap, 0);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_reflectionDepthRB);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "ERROR: Framebuffer not complete for face " << i << std::endl;
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_CULL_FACE);

        // Configuracion de matrices de vista y proyección para esta cara
        glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(captureViews[i]));
        glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(captureProjection));
        glUniform3fv(glGetUniformLocation(m_shaderProgram, "viewPos"), 1, &teapotPos[0]); // el observador está en la tetera

        // Desactivacion de reflexion para todos los objetos en la captura
        glUniform1i(glGetUniformLocation(m_shaderProgram, "isReflective"), 0);
        // Evitar aplicacion de mapeo especial
        glUniform1i(glGetUniformLocation(m_shaderProgram, "texGenMode"), 0);
        // Reseteo de uniforms que pueden quedar activados de objetos anteriores
        glUniform1i(glGetUniformLocation(m_shaderProgram, "useNormalMap"), 0);
        glUniform1i(glGetUniformLocation(m_shaderProgram, "hasAmbientMap"), 0);
        glUniform1i(glGetUniformLocation(m_shaderProgram, "hasSpecularMap"), 0);
        glUniform1i(glGetUniformLocation(m_shaderProgram, "useTexture"), 0);

        // --- Dibujo de skybox para reflejo del entorno ---
        glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_FALSE);
        glUseProgram(m_simpleShader);
        glm::mat4 skyViewCapture = glm::mat4(glm::mat3(captureViews[i]));
        glUniformMatrix4fv(glGetUniformLocation(m_simpleShader, "view"), 1, GL_FALSE, glm::value_ptr(skyViewCapture));
        glUniformMatrix4fv(glGetUniformLocation(m_simpleShader, "projection"), 1, GL_FALSE, glm::value_ptr(captureProjection));
        glBindVertexArray(m_skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_skyboxTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);

        // Restauracion del shader principal para dibujo del resto de objetos
        glUseProgram(m_shaderProgram);
        glEnable(GL_CULL_FACE);

        // --- Dibujo de la Mesa ---
        // Reseteo de uniforms específicos para la mesa
        glUniform1i(glGetUniformLocation(m_shaderProgram, "useNormalMap"), 0);
        glUniform1i(glGetUniformLocation(m_shaderProgram, "hasAmbientMap"), 0);
        glUniform1i(glGetUniformLocation(m_shaderProgram, "hasSpecularMap"), 0);
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

        // --- Dibujo de Objetos Sobre la Mesa ---

        // Cards
        if (!m_cardsSubmeshes.empty()) {
            glUniform1i(glGetUniformLocation(m_shaderProgram, "isReflective"), 0);
            // Reseteo de uniforms que puedan estar activos
            glUniform1i(glGetUniformLocation(m_shaderProgram, "useNormalMap"), 0);
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

            for (size_t j = 0; j < m_cardsSubmeshes.size(); ++j) {
                const auto& sm = m_cardsSubmeshes[j];
                glm::mat4 model = baseModel;

                if (m_cardsCollapsed) {
                    float maxX = tableHalfX * 0.04f;
                    float maxZ = tableHalfZ * 0.04f;
                    float spreadX = sin((float)j * 2.0f) * maxX;
                    float spreadZ = cos((float)j * 3.0f) * maxZ;
                    float rotY = (float)j * 1.5f;
                    float tiltX = sin((float)j * 2.5f) * glm::radians(5.0f);
                    float tiltZ = cos((float)j * 4.0f) * glm::radians(5.0f);
                    model = glm::translate(model, glm::vec3(spreadX * collapseEase, 0.0f, spreadZ * collapseEase));
                    model = glm::rotate(model, rotY * collapseEase, glm::vec3(0, 1, 0));
                    model = glm::rotate(model, tiltX * collapseEase, glm::vec3(1, 0, 0));
                    model = glm::rotate(model, tiltZ * collapseEase, glm::vec3(0, 0, 1));
                }
                else {
                    float bounce = (1.0f - collapseEase) * 0.01f * sin((float)tnow * 6.0f + (float)j);
                    model = glm::translate(model, glm::vec3(0.0f, bounce, 0.0f));
                }

                glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, sm.texDiffuse ? sm.texDiffuse : m_cardsTexture);
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, sm.texAmbient);
                glActiveTexture(GL_TEXTURE3);
                glBindTexture(GL_TEXTURE_2D, sm.texSpecular);

                glUniform3f(glGetUniformLocation(m_shaderProgram, "materialAmbient"), sm.Ka.r, sm.Ka.g, sm.Ka.b);
                glUniform3f(glGetUniformLocation(m_shaderProgram, "materialDiffuse"), sm.Kd.r, sm.Kd.g, sm.Kd.b);
                glUniform3f(glGetUniformLocation(m_shaderProgram, "materialSpecular"), sm.Ks.r, sm.Ks.g, sm.Ks.b);
                glUniform1f(glGetUniformLocation(m_shaderProgram, "materialShininess"), sm.Ns);

                glUniform1i(glGetUniformLocation(m_shaderProgram, "useTexture"), (sm.texDiffuse != 0 && sm.hasTexCoords) ? 1 : 0);
                glUniform1i(glGetUniformLocation(m_shaderProgram, "hasAmbientMap"), sm.texAmbient != 0 ? 1 : 0);
                glUniform1i(glGetUniformLocation(m_shaderProgram, "hasSpecularMap"), sm.texSpecular != 0 ? 1 : 0);
                glUniform1i(glGetUniformLocation(m_shaderProgram, "useNormalMap"), 0);

                glBindVertexArray(sm.vao);
                glDrawArrays(GL_TRIANGLES, 0, sm.vertexCount);
            }
        }
        else if (m_cardsVertexCount > 0 && m_cardsVAO != 0) {
            // Fallback
            glUniform1i(glGetUniformLocation(m_shaderProgram, "isReflective"), 0);
            glUniform1i(glGetUniformLocation(m_shaderProgram, "useNormalMap"), 0);
            glUniform1i(glGetUniformLocation(m_shaderProgram, "hasAmbientMap"), 0);
            glUniform1i(glGetUniformLocation(m_shaderProgram, "hasSpecularMap"), 0);
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

        // Coffee (tazas de café)
        if (!m_coffeeSubmeshes.empty()) {
            glUniform1i(glGetUniformLocation(m_shaderProgram, "isReflective"), 0);
            glUniform1i(glGetUniformLocation(m_shaderProgram, "useNormalMap"), 0); // CORRECIÓN

            float coffeeHeightModel = (m_coffeeMaxY - m_coffeeMinY);
            float coffeeScale = (coffeeHeightModel > 0.0001f) ? (tableHeight * 0.05f) / coffeeHeightModel : 1.0f;
            float coffeeDisplacementY = -m_coffeeMinY * coffeeScale + tableHeight + 0.01f;

            glm::mat4 coffeeModelBase = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, coffeeDisplacementY, tableHalfZ - 6.0f + 1.0f));
            coffeeModelBase = glm::scale(coffeeModelBase, glm::vec3(coffeeScale));

            int spoonIndex = 0;

            for (const auto& sm : m_coffeeSubmeshes) {
                glm::mat4 model = coffeeModelBase;

                if (sm.materialId == 3) {
                    if (spoonIndex < m_coffeeSpoonExtraTransforms.size()) {
                        model = model * m_coffeeSpoonExtraTransforms[spoonIndex];
                    }
                    spoonIndex++;
                }

                glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

                glUniform3f(glGetUniformLocation(m_shaderProgram, "materialAmbient"), sm.Ka.r, sm.Ka.g, sm.Ka.b);
                glUniform3f(glGetUniformLocation(m_shaderProgram, "materialDiffuse"), sm.Kd.r, sm.Kd.g, sm.Kd.b);
                glUniform3f(glGetUniformLocation(m_shaderProgram, "materialSpecular"), sm.Ks.r, sm.Ks.g, sm.Ks.b);
                glUniform1f(glGetUniformLocation(m_shaderProgram, "materialShininess"), sm.Ns);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, sm.texDiffuse ? sm.texDiffuse : 0);
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, sm.texAmbient);
                glActiveTexture(GL_TEXTURE3);
                glBindTexture(GL_TEXTURE_2D, sm.texSpecular);

                glUniform1i(glGetUniformLocation(m_shaderProgram, "useTexture"), (sm.texDiffuse != 0 && sm.hasTexCoords) ? 1 : 0);
                glUniform1i(glGetUniformLocation(m_shaderProgram, "hasAmbientMap"), sm.texAmbient != 0 ? 1 : 0);
                glUniform1i(glGetUniformLocation(m_shaderProgram, "hasSpecularMap"), sm.texSpecular != 0 ? 1 : 0);
                glUniform1i(glGetUniformLocation(m_shaderProgram, "useNormalMap"), 0); // CORRECIÓN

                glBindVertexArray(sm.vao);
                glDrawArrays(GL_TRIANGLES, 0, sm.vertexCount);
            }
        }

        // Bowl de Frutas
        if (!m_bowlSubmeshes.empty()) {
            glUniform1i(glGetUniformLocation(m_shaderProgram, "isReflective"), 0);
            glUniform1i(glGetUniformLocation(m_shaderProgram, "useNormalMap"), 0); // CORRECIÓN
            glUniform3f(glGetUniformLocation(m_shaderProgram, "materialAmbient"), 0.06f, 0.06f, 0.06f);
            glUniform3f(glGetUniformLocation(m_shaderProgram, "materialDiffuse"), 0.95f, 0.95f, 0.95f);
            glUniform3f(glGetUniformLocation(m_shaderProgram, "materialSpecular"), 0.2f, 0.2f, 0.2f);
            glUniform1f(glGetUniformLocation(m_shaderProgram, "materialShininess"), 32.0f);

            glm::mat4 bowlModelBase = glm::translate(glm::mat4(1.0f), glm::vec3(-tableHalfX + 6.0f, bowlDisplacementY, -4.0f));
            bowlModelBase = glm::scale(bowlModelBase, glm::vec3(bowlScale));

            double tnow = glfwGetTime();

            for (size_t j = 0; j < m_bowlSubmeshes.size(); ++j) {
                const auto& sm = m_bowlSubmeshes[j];
                glm::mat4 model = bowlModelBase;
                if (sm.isFruit) {
                    float rise = sin((float)tnow * 3.0f + (float)j) * 0.02f + 0.02f;
                    model = glm::translate(model, glm::vec3(0.0f, rise, 0.0f));
                    model = glm::rotate(model, sin((float)tnow * 1.5f + (float)j) * glm::radians(4.0f), glm::vec3(0, 0, 1));
                }
                glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, sm.texDiffuse);
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, sm.texAmbient);
                glActiveTexture(GL_TEXTURE3);
                glBindTexture(GL_TEXTURE_2D, sm.texSpecular);

                glUniform3f(glGetUniformLocation(m_shaderProgram, "materialAmbient"), sm.Ka.r, sm.Ka.g, sm.Ka.b);
                glUniform3f(glGetUniformLocation(m_shaderProgram, "materialDiffuse"), sm.Kd.r, sm.Kd.g, sm.Kd.b);
                glUniform3f(glGetUniformLocation(m_shaderProgram, "materialSpecular"), sm.Ks.r, sm.Ks.g, sm.Ks.b);
                glUniform1f(glGetUniformLocation(m_shaderProgram, "materialShininess"), sm.Ns);

                glUniform1i(glGetUniformLocation(m_shaderProgram, "useTexture"), (sm.texDiffuse != 0 && sm.hasTexCoords) ? 1 : 0);
                glUniform1i(glGetUniformLocation(m_shaderProgram, "hasAmbientMap"), sm.texAmbient != 0 ? 1 : 0);
                glUniform1i(glGetUniformLocation(m_shaderProgram, "hasSpecularMap"), sm.texSpecular != 0 ? 1 : 0);
                glUniform1i(glGetUniformLocation(m_shaderProgram, "useNormalMap"), 0); // CORRECIÓN

                glBindVertexArray(sm.vao);
                glDrawArrays(GL_TRIANGLES, 0, sm.vertexCount);
            }
        }

        // Cup (taza)
        if (!m_cupSubmeshes.empty()) {
            glUniform1i(glGetUniformLocation(m_shaderProgram, "isReflective"), 0);
            glUniform1i(glGetUniformLocation(m_shaderProgram, "useNormalMap"), 0); // CORRECIÓN

            float cupHeightModel = (m_cupMaxY - m_cupMinY);
            float cupScale = (cupHeightModel > 0.0001f) ? (tableHeight * 0.12f) / cupHeightModel : 1.0f;
            float cupDisplacementY = -m_cupMinY * cupScale + tableHeight + 0.01f;

            glm::mat4 cupModelBase = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, cupDisplacementY, -tableHalfZ + 3.0f));
            cupModelBase = glm::scale(cupModelBase, glm::vec3(cupScale));

            for (size_t j = 0; j < m_cupSubmeshes.size(); ++j) {
                const auto& sm = m_cupSubmeshes[j];
                glm::mat4 model = cupModelBase;
                glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

                glUniform3f(glGetUniformLocation(m_shaderProgram, "materialAmbient"), sm.Ka.r, sm.Ka.g, sm.Ka.b);
                glUniform3f(glGetUniformLocation(m_shaderProgram, "materialDiffuse"), sm.Kd.r, sm.Kd.g, sm.Kd.b);
                glUniform3f(glGetUniformLocation(m_shaderProgram, "materialSpecular"), sm.Ks.r, sm.Ks.g, sm.Ks.b);
                glUniform1f(glGetUniformLocation(m_shaderProgram, "materialShininess"), sm.Ns);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, sm.texDiffuse ? sm.texDiffuse : 0);
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, sm.texAmbient);
                glActiveTexture(GL_TEXTURE3);
                glBindTexture(GL_TEXTURE_2D, sm.texSpecular);

                glUniform1i(glGetUniformLocation(m_shaderProgram, "useTexture"), (sm.texDiffuse != 0 && sm.hasTexCoords) ? 1 : 0);
                glUniform1i(glGetUniformLocation(m_shaderProgram, "hasAmbientMap"), sm.texAmbient != 0 ? 1 : 0);
                glUniform1i(glGetUniformLocation(m_shaderProgram, "hasSpecularMap"), sm.texSpecular != 0 ? 1 : 0);
                glUniform1i(glGetUniformLocation(m_shaderProgram, "useNormalMap"), 0); // CORRECIÓN

                glBindVertexArray(sm.vao);
                glDrawArrays(GL_TRIANGLES, 0, sm.vertexCount);
            }
        }

        // Dibujo de la esfera paramétrica (si existe)
        if (globalUIState.updateTextures) {
            // Recargar textura difusa
            if (m_sphereDiffuseTexture) glDeleteTextures(1, &m_sphereDiffuseTexture);
            std::string diffusePath = "OBJs/bumpMapping/" + std::string(globalUIState.diffuseFiles[globalUIState.currentDiffuseIndex]);
            m_sphereDiffuseTexture = loadTexture(diffusePath.c_str());

            // Recargar textura bump
            if (m_sphereBumpTexture) glDeleteTextures(1, &m_sphereBumpTexture);
            std::string bumpPath = "OBJs/bumpMapping/" + std::string(globalUIState.bumpFiles[globalUIState.currentBumpIndex]);
            m_sphereBumpTexture = loadTexture(bumpPath.c_str());

            globalUIState.updateTextures = false;
        }

        if (m_paramSphereVAO != 0) {
            glUseProgram(m_shaderProgram);
            glUniform1i(glGetUniformLocation(m_shaderProgram, "isReflective"), 0);
            // Material: textura difusa
            glUniform3f(glGetUniformLocation(m_shaderProgram, "materialAmbient"), 0.2f, 0.2f, 0.2f);
            glUniform3f(glGetUniformLocation(m_shaderProgram, "materialDiffuse"), 1.0f, 1.0f, 1.0f);
            glUniform3f(glGetUniformLocation(m_shaderProgram, "materialSpecular"), 0.5f, 0.5f, 0.5f);
            glUniform1f(glGetUniformLocation(m_shaderProgram, "materialShininess"), 32.0f);
            // Activar textura difusa
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m_sphereDiffuseTexture);
            glUniform1i(glGetUniformLocation(m_shaderProgram, "useTexture"), 1);
            // Desactivar mapas ambient y specular
            glUniform1i(glGetUniformLocation(m_shaderProgram, "hasAmbientMap"), 0);
            glUniform1i(glGetUniformLocation(m_shaderProgram, "hasSpecularMap"), 0);
            // Configurar bump mapping
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, m_sphereBumpTexture);
            glUniform1i(glGetUniformLocation(m_shaderProgram, "normalMap"), 4);
            glUniform1i(glGetUniformLocation(m_shaderProgram, "useNormalMap"), globalUIState.useBumpMap ? 1 : 0);
            glUniform1f(glGetUniformLocation(m_shaderProgram, "bumpIntensity"), globalUIState.bumpIntensity);

            float sphereRadius = 2.0f;
            glm::vec3 bowlCenter = glm::vec3(-tableHalfX + 12.0f, bowlDisplacementY, 8.0f);
            glm::vec3 spherePos = bowlCenter + glm::vec3(3.0f, sphereRadius, 2.0f);
            spherePos.y = tableHeight + sphereRadius;

            glm::mat4 sphereModel = glm::translate(glm::mat4(1.0f), spherePos);
            sphereModel = glm::scale(sphereModel, glm::vec3(sphereRadius));

            glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(sphereModel));

            glBindVertexArray(m_paramSphereVAO);
            glDrawElements(GL_TRIANGLES, m_paramSphereIndexCount, GL_UNSIGNED_INT, 0);
        }
        // Desactivacion de normal mapping para no afecta objetos siguientes
        glUniform1i(glGetUniformLocation(m_shaderProgram, "useNormalMap"), 0);

        // --- Dibujo de esferas de luz ---
        glUseProgram(m_lightShader);
        glUniformMatrix4fv(glGetUniformLocation(m_lightShader, "view"), 1, GL_FALSE, glm::value_ptr(captureViews[i]));
        glUniformMatrix4fv(glGetUniformLocation(m_lightShader, "projection"), 1, GL_FALSE, glm::value_ptr(captureProjection));
        glBindVertexArray(m_sphereVAO);
        for (int j = 0; j < 3; j++) {
            if (!globalUIState.lightEnabled[j]) continue;
            glm::mat4 lModel = glm::translate(glm::mat4(1.0f), m_lightPos[j]);
            lModel = glm::scale(lModel, glm::vec3(2.0f));
            glUniformMatrix4fv(glGetUniformLocation(m_lightShader, "model"), 1, GL_FALSE, glm::value_ptr(lModel));
            glUniform3fv(glGetUniformLocation(m_lightShader, "color"), 1, globalUIState.lightColors[j]);
            glDrawElements(GL_TRIANGLES, m_sphereIndexCount, GL_UNSIGNED_INT, 0);
        }

        // Restauracion de shader principal para la siguiente cara
        glUseProgram(m_shaderProgram);
    }

    // Generacion de mipmaps para la cubemap dinámica y asegurar filtros adecuados
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_reflectionCubemap);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    // Restauracion de FBO y viewport para el render normal
    glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
    glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);

    // ---- Render normal (camara del usuario) ----

    // --- Dibujo de skybox ---
    glUseProgram(m_simpleShader);
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

    // --- Objetos Iluminados ---
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

    // --- Dibujo de la Mesa ---
    glUniform1i(glGetUniformLocation(m_shaderProgram, "isReflective"), 0);
    // Reseteao de uniforms que puedan estar activos
    glUniform1i(glGetUniformLocation(m_shaderProgram, "useNormalMap"), 0);
    glUniform1i(glGetUniformLocation(m_shaderProgram, "hasAmbientMap"), 0);
    glUniform1i(glGetUniformLocation(m_shaderProgram, "hasSpecularMap"), 0);
    glUniform3f(glGetUniformLocation(m_shaderProgram, "materialAmbient"), 0.35f, 0.35f, 0.35f);
    glUniform3f(glGetUniformLocation(m_shaderProgram, "materialDiffuse"), 0.8f, 0.8f, 0.8f);
    glUniform3f(glGetUniformLocation(m_shaderProgram, "materialSpecular"), 0.2f, 0.2f, 0.2f);
    glUniform1f(glGetUniformLocation(m_shaderProgram, "materialShininess"), 16.0f);

    float displacementY = -m_tableMinY * scale;
    glm::mat4 tableModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, displacementY, 0.0f));
    tableModel = glm::scale(tableModel, glm::vec3(scale));
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(tableModel));

    applySelectionAndBBox((int)C3DViewer::OBJ_TABLE, tableModel, glm::vec3(m_tableMinX, m_tableMinY, m_tableMinZ), glm::vec3(m_tableMaxX, m_tableMaxY, m_tableMaxZ));

    glBindTexture(GL_TEXTURE_2D, m_tableTexture);
    glUniform1i(glGetUniformLocation(m_shaderProgram, "useTexture"), (m_tableTexture != 0 && m_tableHasTexCoords) ? 1 : 0);
    glBindVertexArray(m_tableVAO);
    glDrawArrays(GL_TRIANGLES, 0, m_tableVertexCount);

    // --- Dibujo de la Tetera (con reflexión dinámica) ---
    if (!m_teapotSubmeshes.empty()) {
        // Activar la cubemap dinámica en la unidad 5
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_reflectionCubemap);
        glUniform1i(glGetUniformLocation(m_shaderProgram, "reflectionMap"), 5);

        GLint locUseDynamic = glGetUniformLocation(m_shaderProgram, "useDynamicReflection");
        glUniform1i(locUseDynamic, 1);

        glUniform1i(glGetUniformLocation(m_shaderProgram, "isReflective"), 1);

        // Misma transformacion de la captura
        glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(teapotModel));

        applySelectionAndBBox((int)C3DViewer::OBJ_TEAPOT, teapotModel,
            glm::vec3(m_teapotMinX, m_teapotMinY, m_teapotMinZ),
            glm::vec3(m_teapotMaxX, m_teapotMaxY, m_teapotMaxZ));

        for (const auto& sm : m_teapotSubmeshes) {
            // Reseteo de uniforms por si acaso
            glUniform1i(glGetUniformLocation(m_shaderProgram, "useNormalMap"), 0);
            glm::mat4 model = teapotModel;
            glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

            glm::vec3 Ka = (sm.Ka != glm::vec3(0.02f)) ? sm.Ka : glm::vec3(0.02f);
            glm::vec3 Kd = (sm.Kd != glm::vec3(0.8f)) ? sm.Kd : glm::vec3(0.1f);
            glm::vec3 Ks = (sm.Ks != glm::vec3(0.2f)) ? sm.Ks : glm::vec3(1.0f);
            float Ns = (sm.Ns != 32.0f) ? sm.Ns : 256.0f;

            glUniform3f(glGetUniformLocation(m_shaderProgram, "materialAmbient"), Ka.r, Ka.g, Ka.b);
            glUniform3f(glGetUniformLocation(m_shaderProgram, "materialDiffuse"), Kd.r, Kd.g, Kd.b);
            glUniform3f(glGetUniformLocation(m_shaderProgram, "materialSpecular"), Ks.r, Ks.g, Ks.b);
            glUniform1f(glGetUniformLocation(m_shaderProgram, "materialShininess"), Ns);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, sm.texDiffuse ? sm.texDiffuse : 0);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, sm.texAmbient);
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, sm.texSpecular);

            glUniform1i(glGetUniformLocation(m_shaderProgram, "useTexture"),
                (sm.texDiffuse != 0 && sm.hasTexCoords) ? 1 : 0);
            glUniform1i(glGetUniformLocation(m_shaderProgram, "hasAmbientMap"),
                sm.texAmbient != 0 ? 1 : 0);
            glUniform1i(glGetUniformLocation(m_shaderProgram, "hasSpecularMap"),
                sm.texSpecular != 0 ? 1 : 0);

            glBindVertexArray(sm.vao);
            glDrawArrays(GL_TRIANGLES, 0, sm.vertexCount);
        }

        glUniform1i(locUseDynamic, 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_skyboxTexture);
        glActiveTexture(GL_TEXTURE0);
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

        applySelectionAndBBox((int)C3DViewer::OBJ_CARDS_TOWER, baseModel, glm::vec3(m_cardsMinX, m_cardsMinY, m_cardsMinZ), glm::vec3(m_cardsMaxX, m_cardsMaxY, m_cardsMaxZ));

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
                // Modo reconstrucción: pequeño rebote vertical
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

    // Coffee (tazas de café)
    if (!m_coffeeSubmeshes.empty()) {
        glUniform1i(glGetUniformLocation(m_shaderProgram, "isReflective"), 0);

        float coffeeHeightModel = (m_coffeeMaxY - m_coffeeMinY);
        float coffeeScale = (coffeeHeightModel > 0.0001f) ? (tableHeight * 0.05f) / coffeeHeightModel : 1.0f;
        float coffeeDisplacementY = -m_coffeeMinY * coffeeScale + tableHeight + 0.01f;

        glm::mat4 coffeeModelBase = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, coffeeDisplacementY, tableHalfZ - 6.0f + 1.0f));
        coffeeModelBase = glm::scale(coffeeModelBase, glm::vec3(coffeeScale));

        int spoonIndex = 0;

        applySelectionAndBBox((int)C3DViewer::OBJ_COFFEE, coffeeModelBase, glm::vec3(m_coffeeMinX, m_coffeeMinY, m_coffeeMinZ), glm::vec3(m_coffeeMaxX, m_coffeeMaxY, m_coffeeMaxZ));
        for (const auto& sm : m_coffeeSubmeshes) {
            glm::mat4 model = coffeeModelBase;

            // Si es una cuchara (material 3), aplicamos la transformación extra
            if (sm.materialId == 3) {
                // Aplicar la transformación individual si el índice es válido
                if (spoonIndex < m_coffeeSpoonExtraTransforms.size()) {
                    model = model * m_coffeeSpoonExtraTransforms[spoonIndex];
                }
                spoonIndex++;
            }

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

        glm::mat4 bowlModelBase = glm::translate(glm::mat4(1.0f), glm::vec3(-tableHalfX + 6.0f, bowlDisplacementY, -4.0f));
        bowlModelBase = glm::scale(bowlModelBase, glm::vec3(bowlScale));

        double tnow = glfwGetTime();

        applySelectionAndBBox((int)C3DViewer::OBJ_BOWL, bowlModelBase, glm::vec3(m_bowlMinX, m_bowlMinY, m_bowlMinZ), glm::vec3(m_bowlMaxX, m_bowlMaxY, m_bowlMaxZ));
        for (size_t i = 0; i < m_bowlSubmeshes.size(); ++i) {
            const auto& sm = m_bowlSubmeshes[i];
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

        applySelectionAndBBox((int)C3DViewer::OBJ_CUP, cupModelBase, glm::vec3(m_cupMinX, m_cupMinY, m_cupMinZ), glm::vec3(m_cupMaxX, m_cupMaxY, m_cupMaxZ));
        for (size_t i = 0; i < m_cupSubmeshes.size(); ++i) {
            const auto& sm = m_cupSubmeshes[i];
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

    // Dibujo de la esfera paramétrica

    if (globalUIState.updateTextures) {
        // Recargar textura difusa
        if (m_sphereDiffuseTexture) glDeleteTextures(1, &m_sphereDiffuseTexture);
        std::string diffusePath = "OBJs/bumpMapping/" + std::string(globalUIState.diffuseFiles[globalUIState.currentDiffuseIndex]);
        m_sphereDiffuseTexture = loadTexture(diffusePath.c_str());

        // Recargar textura bump
        if (m_sphereBumpTexture) glDeleteTextures(1, &m_sphereBumpTexture);
        std::string bumpPath = "OBJs/bumpMapping/" + std::string(globalUIState.bumpFiles[globalUIState.currentBumpIndex]);
        m_sphereBumpTexture = loadTexture(bumpPath.c_str());

        globalUIState.updateTextures = false; // Resetear flag
    }

    if (m_paramSphereVAO != 0) {
        glUseProgram(m_shaderProgram);
        glUniform1i(glGetUniformLocation(m_shaderProgram, "isReflective"), 0);
        // Material: textura difusa
        glUniform3f(glGetUniformLocation(m_shaderProgram, "materialAmbient"), 0.2f, 0.2f, 0.2f);
        glUniform3f(glGetUniformLocation(m_shaderProgram, "materialDiffuse"), 1.0f, 1.0f, 1.0f);
        glUniform3f(glGetUniformLocation(m_shaderProgram, "materialSpecular"), 0.5f, 0.5f, 0.5f);
        glUniform1f(glGetUniformLocation(m_shaderProgram, "materialShininess"), 32.0f);
        // Activar textura difusa
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_sphereDiffuseTexture);
        glUniform1i(glGetUniformLocation(m_shaderProgram, "useTexture"), 1);
        // Desactivar mapas ambient y specular
        glUniform1i(glGetUniformLocation(m_shaderProgram, "hasAmbientMap"), 0);
        glUniform1i(glGetUniformLocation(m_shaderProgram, "hasSpecularMap"), 0);
        // Configurar bump mapping
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, m_sphereBumpTexture);
        glUniform1i(glGetUniformLocation(m_shaderProgram, "normalMap"), 4);
        glUniform1i(glGetUniformLocation(m_shaderProgram, "useNormalMap"), globalUIState.useBumpMap ? 1 : 0);
        glUniform1f(glGetUniformLocation(m_shaderProgram, "bumpIntensity"), globalUIState.bumpIntensity);

        float sphereRadius = 2.0f;
        glm::vec3 bowlCenter = glm::vec3(-tableHalfX + 12.0f, bowlDisplacementY, 8.0f);
        glm::vec3 spherePos = bowlCenter + glm::vec3(3.0f, sphereRadius, 2.0f);
        spherePos.y = tableHeight + sphereRadius;

        glm::mat4 sphereModel = glm::translate(glm::mat4(1.0f), spherePos);
        sphereModel = glm::scale(sphereModel, glm::vec3(sphereRadius));

        // Apply mapping overrides / draw bbox if param sphere is selected
        applySelectionAndBBox((int)C3DViewer::OBJ_PARAM_SPHERE, sphereModel, glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(1.0f, 1.0f, 1.0f));

        glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(sphereModel));

        glBindVertexArray(m_paramSphereVAO);
        glDrawElements(GL_TRIANGLES, m_paramSphereIndexCount, GL_UNSIGNED_INT, 0);
    }
    glUniform1i(glGetUniformLocation(m_shaderProgram, "useNormalMap"), 0);

    // --- Dibujo de esferas de luz (normal) ---
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
    GLint locNormalMap = glGetUniformLocation(m_shaderProgram, "normalMap");
    if (locNormalMap >= 0) glUniform1i(locNormalMap, 4);
    GLint locReflectionMap = glGetUniformLocation(m_shaderProgram, "reflectionMap");
    if (locReflectionMap >= 0) glUniform1i(locReflectionMap, 5);

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

//void C3DViewer::setupParamSphere() {
//    const int stacks = 20, slices = 20;
//    const float radius = 1.0f;
//    std::vector<float> vertices;
//    std::vector<unsigned int> indices;
//
//    for (int i = 0; i <= stacks; ++i) {
//        float phi = i * glm::pi<float>() / stacks; // 0 a pi
//        float sinPhi = sin(phi);
//        float cosPhi = cos(phi);
//        float v = 1.0f - (float)i / stacks; // V coord (0 en el polo norte, 1 en el sur)
//
//        for (int j = 0; j <= slices; ++j) {
//            float theta = j * 2.0f * glm::pi<float>() / slices; // 0 a 2pi
//            float sinTheta = sin(theta);
//            float cosTheta = cos(theta);
//            float u = (float)j / slices; // U coord
//
//            // Posición
//            float x = radius * sinPhi * cosTheta;
//            float y = radius * cosPhi;
//            float z = radius * sinPhi * sinTheta;
//            vertices.push_back(x);
//            vertices.push_back(y);
//            vertices.push_back(z);
//
//            // Normal
//            vertices.push_back(x / radius);
//            vertices.push_back(y / radius);
//            vertices.push_back(z / radius);
//
//            // Coordenadas de textura
//            vertices.push_back(u);
//            vertices.push_back(v);
//        }
//    }
//
//    // Índices para triángulos
//    for (int i = 0; i < stacks; ++i) {
//        for (int j = 0; j < slices; ++j) {
//            int first = i * (slices + 1) + j;
//            int second = first + slices + 1;
//            indices.push_back(first);
//            indices.push_back(second);
//            indices.push_back(first + 1);
//            indices.push_back(second);
//            indices.push_back(second + 1);
//            indices.push_back(first + 1);
//        }
//    }
//
//    // Crear y llenar buffers
//    glGenVertexArrays(1, &m_paramSphereVAO);
//    glGenBuffers(1, &m_paramSphereVBO);
//    glGenBuffers(1, &m_paramSphereEBO);
//
//    glBindVertexArray(m_paramSphereVAO);
//    glBindBuffer(GL_ARRAY_BUFFER, m_paramSphereVBO);
//    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
//    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_paramSphereEBO);
//    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
//
//    // Atributo 0: posición
//    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
//    glEnableVertexAttribArray(0);
//    // Atributo 1: normal
//    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
//    glEnableVertexAttribArray(1);
//    // Atributo 3: coordenadas de textura (layout 3 en el shader)
//    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
//    glEnableVertexAttribArray(3);
//
//    m_paramSphereIndexCount = indices.size();
//    glBindVertexArray(0);
//}

void C3DViewer::setupParamSphere() {
    const int stacks = 20, slices = 20;
    const float radius = 1.0f;
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    for (int i = 0; i <= stacks; ++i) {
        float phi = i * glm::pi<float>() / stacks; // 0 a pi
        float sinPhi = sin(phi);
        float cosPhi = cos(phi);
        float v = 1.0f - (float)i / stacks; // V coord (0 en polo norte, 1 en sur)

        for (int j = 0; j <= slices; ++j) {
            float theta = j * 2.0f * glm::pi<float>() / slices; // 0 a 2pi
            float sinTheta = sin(theta);
            float cosTheta = cos(theta);
            float u = (float)j / slices; // U coord

            // Posición
            float x = radius * sinPhi * cosTheta;
            float y = radius * cosPhi;
            float z = radius * sinPhi * sinTheta;
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);

            // Normal
            vertices.push_back(x / radius);
            vertices.push_back(y / radius);
            vertices.push_back(z / radius);

            // Tangente (derivada respecto a theta)
            float tx = -sinPhi * sinTheta;
            float ty = 0.0f;
            float tz = sinPhi * cosTheta;
            float lenT = sqrt(tx * tx + ty * ty + tz * tz);
            if (lenT > 0.001f) {
                tx /= lenT; ty /= lenT; tz /= lenT;
            }
            else {
                // En los polos, dirección arbitraria
                tx = 1.0f; ty = 0.0f; tz = 0.0f;
            }
            vertices.push_back(tx);
            vertices.push_back(ty);
            vertices.push_back(tz);

            // Coordenadas de textura
            vertices.push_back(u);
            vertices.push_back(v);
        }
    }

    // Índices para triángulos (igual que antes)
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

    // Crear buffers
    glGenVertexArrays(1, &m_paramSphereVAO);
    glGenBuffers(1, &m_paramSphereVBO);
    glGenBuffers(1, &m_paramSphereEBO);

    glBindVertexArray(m_paramSphereVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_paramSphereVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_paramSphereEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Atributos
    // Posición (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Normal (location 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // Tangente (location 4)
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(4);
    // TexCoords (location 3)
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(9 * sizeof(float)));
    glEnableVertexAttribArray(3);

    m_paramSphereIndexCount = indices.size();
    glBindVertexArray(0);
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