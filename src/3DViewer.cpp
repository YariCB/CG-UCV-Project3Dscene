#include "Interface.h"
#include "3DViewer.h"
#include <iostream>
#include <cstdio>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

static UIState globalUIState;

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

    if (m_simpleShader) glDeleteProgram(m_simpleShader);
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

    m_window = glfwCreateWindow(width, height, "C3DViewer Window: Hello Triangle", NULL, NULL);
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
    loadOBJ("OBJs/table/table4.obj");
    m_tableTexture = loadTexture("OBJs/table/tipical.jpg");

    if (!setupSimpleShader()) return false;

    // Setup VAO y VBO para el triángulo
    //setupTriangle();

    // Mesa y Skybox
    // Si no se cargó un OBJ (m_tableVertexCount == 0) usamos el cubo de prueba;
    // de lo contrario conservamos el VAO creado por loadOBJ().
    if (m_tableVertexCount == 0) {
        setupTable();
    }
    setupSkybox();
    setupSphere();

    glEnable(GL_DEPTH_TEST);

    glViewport(0, 0, width, height);

    glfwSetWindowUserPointer(m_window, this);
    glfwSetKeyCallback(m_window, keyCallbackStatic);
    glfwSetMouseButtonCallback(m_window, mouseButtonCallbackStatic);
    glfwSetCursorPosCallback(m_window, cursorPosCallbackStatic);

    return true;
}

void C3DViewer::update() {
    double time = glfwGetTime();
    for (int i = 0; i < 3; i++) {
        float angle = time * m_lightAngularSpeed[i] * globalUIState.lightSpeed;
        m_lightPos[i].x = m_lightRadii[i] * cos(angle);
        m_lightPos[i].z = m_lightRadii[i] * sin(angle);
        m_lightPos[i].y = m_lightHeights[i];
    }
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

    if (action == GLFW_PRESS)
    {
        std::cout << "Key " << key << " pressed\n";
        if (key == GLFW_KEY_ESCAPE)
            glfwSetWindowShouldClose(m_window, GLFW_TRUE);
    }
    else if (action == GLFW_RELEASE)
        std::cout << "Key " << key << " released\n";
}

void C3DViewer::onMouseButton(int button, int action, int mods) 
{
    if (button >= 0 && button < 3)
    {
        double xpos, ypos;
        glfwGetCursorPos(m_window, &xpos, &ypos);
        if (action == GLFW_PRESS)
        {
            mouseButtonsDown[button] = true;
            // Obtener posici�n actual del cursor
            std::cout << "Mouse button " << button << " pressed at position (" << xpos << ", " << ypos << ")\n";
        }
        else if (action == GLFW_RELEASE)
        {

            mouseButtonsDown[button] = false;
            std::cout << "Mouse button " << button << " released at position (" << xpos << ", " << ypos << ")\n";
        }
    }
}

void C3DViewer::onCursorPos(double xpos, double ypos) 
{
    if (mouseButtonsDown[0] || mouseButtonsDown[1] || mouseButtonsDown[2]) 
    {
        std::cout << "Mouse move at (" << xpos << ", " << ypos << ")\n";
    }
}

void C3DViewer::render() {
    update();

    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // --- Configuración de cámara ---
    glm::vec3 camPos(0.0f, 1.5f, 15.0f);
    glm::mat4 view = glm::lookAt(camPos, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 500.0f);

    // --- Dibujo de objetos iluminados ---
    glUseProgram(m_shaderProgram);
    glm::mat4 model = glm::mat4(1.0f);
    /*model = glm::scale(model, glm::vec3(0.1f));*/
    // Matrices
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    // Posición de la cámara
    glUniform3fv(glGetUniformLocation(m_shaderProgram, "viewPos"), 1, &camPos[0]);
    // Atenuación
    glUniform1i(glGetUniformLocation(m_shaderProgram, "attenuationEnabled"), globalUIState.attenuation);

    // Configuración de materiales (coherente con textura de madera)
    glUniform3f(glGetUniformLocation(m_shaderProgram, "materialAmbient"), 0.12f, 0.05f, 0.02f);
    glUniform3f(glGetUniformLocation(m_shaderProgram, "materialDiffuse"), 0.8f, 0.7f, 0.6f);
    glUniform3f(glGetUniformLocation(m_shaderProgram, "materialSpecular"), 0.15f, 0.15f, 0.15f);
    glUniform1f(glGetUniformLocation(m_shaderProgram, "materialShininess"), 32.0f);

    // Luces: posiciones, colores, etc.
    for (int i = 0; i < 3; i++) {
        char name[64];
        snprintf(name, sizeof(name), "lightPos[%d]", i);
        glUniform3fv(glGetUniformLocation(m_shaderProgram, name), 1, &m_lightPos[i][0]);
    }
    for (int i = 0; i < 3; i++) {
        char name[64];
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

    // --- Dibujo de la Mesa OBJ ---
    glUseProgram(m_shaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glm::mat4 tableModel = glm::mat4(1.0f);
    // Escala
    tableModel = glm::scale(tableModel, glm::vec3(0.5f));  
    // Posición
    tableModel = glm::translate(tableModel, glm::vec3(0.0f, -80.0f, -40.0f));
    // Rotación
    tableModel = glm::rotate(tableModel, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    tableModel = glm::rotate(tableModel, glm::radians(10.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(tableModel));
    // Textura
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_tableTexture);
    glUniform1i(glGetUniformLocation(m_shaderProgram, "texture_diffuse"), 0);
    glUniform1i(glGetUniformLocation(m_shaderProgram, "useTexture"), 1);
    // Dibujo
    glBindVertexArray(m_tableVAO);
    glDrawArrays(GL_TRIANGLES, 0, m_tableVertexCount);

    // --- Dibujo de skybox ---
    glUseProgram(m_simpleShader);
    glm::mat4 skyView = glm::mat4(glm::mat3(view)); // Sin componente de traslación (es el cielo)
    glUniformMatrix4fv(glGetUniformLocation(m_simpleShader, "view"), 1, GL_FALSE, glm::value_ptr(skyView));
    glUniformMatrix4fv(glGetUniformLocation(m_simpleShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glm::mat4 skyModel = glm::scale(glm::mat4(1.0f), glm::vec3(100.0f));
    glUniformMatrix4fv(glGetUniformLocation(m_simpleShader, "model"), 1, GL_FALSE, glm::value_ptr(skyModel));
    glUniform3f(glGetUniformLocation(m_simpleShader, "color"), 0.8f, 0.8f, 0.8f); // gris claro
    GLboolean cullEnabled = glIsEnabled(GL_CULL_FACE);
    if (cullEnabled) glDisable(GL_CULL_FACE);
    glDepthMask(GL_FALSE);
    glBindVertexArray(m_skyboxVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glDepthMask(GL_TRUE);
    if (cullEnabled) glEnable(GL_CULL_FACE);

    // --- Dibujo de esferas de luz ---
    glUseProgram(m_simpleShader);
    glUniformMatrix4fv(glGetUniformLocation(m_simpleShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(m_simpleShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glBindVertexArray(m_sphereVAO);
    for (int i = 0; i < 3; i++) {
        if (!globalUIState.lightEnabled[i]) continue;
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, m_lightPos[i]);
        model = glm::scale(model, glm::vec3(0.8f));
        glUniformMatrix4fv(glGetUniformLocation(m_simpleShader, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniform3fv(glGetUniformLocation(m_simpleShader, "color"), 1, globalUIState.lightColors[i]);
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

    // Llamada al nuevo dise�o de interfaz
    DrawMainPanel(globalUIState);

    // Aqu� colocas el c�digo ImGui para el slider
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
    glShaderSource(vertexShader, 1, &simpleVertexSrc, nullptr);
    glCompileShader(vertexShader);
    if (!checkCompileErrors(vertexShader, "VERTEX")) return false;

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &simpleFragmentSrc, nullptr);
    glCompileShader(fragmentShader);
    if (!checkCompileErrors(fragmentShader, "FRAGMENT")) return false;

    m_simpleShader = glCreateProgram();
    glAttachShader(m_simpleShader, vertexShader);
    glAttachShader(m_simpleShader, fragmentShader);
    glLinkProgram(m_simpleShader);
    if (!checkCompileErrors(m_simpleShader, "PROGRAM")) return false;

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
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
    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex{};
            vertex.Position = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };
            if (index.normal_index >= 0) {
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
            }
            // Por defecto asignamos color blanco (si el OBJ no trae color por vértice)
            vertex.Color = glm::vec3(1.0f, 1.0f, 1.0f);
            vertices.push_back(vertex);
        }
    }
    m_tableVertexCount = vertices.size();
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
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data) {
        std::cout << "Textura cargada correctamente: " << path << " (" << width << "x" << height << ")" << std::endl;
        GLenum format = (nrComponents == 4) ? GL_RGBA : GL_RGB;
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        // Set texture wrapping and filtering options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else {
        std::cerr << "ERROR: No se pudo encontrar la imagen en: " << path << std::endl;
    }
    return textureID;
}