#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"

struct UIState;

// Estructura completa para cumplir con iluminación y texturas
struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec3 Color;
    glm::vec2 TexCoords;
};

class C3DViewer
{

public:
    C3DViewer();
    bool setup();
    void mainLoop();
    virtual ~C3DViewer();

private:
    // Métodos de carga
    void loadOBJ(const std::string& path);
    unsigned int loadTexture(const char* path);

    // Callbacks y lógica
    virtual void onKey(int key, int scancode, int action, int mods);
    virtual void onMouseButton(int button, int action, int mods);
    virtual void onCursorPos(double xpos, double ypos);
    virtual void update();
    virtual void render();
    virtual void drawInterface();
    void resize(int new_width, int new_height);
    bool setupShader();
    bool checkCompileErrors(GLuint shader, const char* type);

    void setupTriangle();

    static void keyCallbackStatic(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouseButtonCallbackStatic(GLFWwindow* window, int button, int action, int mods);
    static void cursorPosCallbackStatic(GLFWwindow* window, double xpos, double ypos);

    bool setupSimpleShader();

    unsigned int loadCubemap(std::vector<std::string> faces);

protected:
    int width = 1280;
    int height = 720;
    GLFWwindow* m_window = nullptr;

    // Buffers para la mesa OBJ
    GLuint m_tableVAO = 0;
    GLuint m_tableVBO = 0;
    size_t m_tableVertexCount = 0;
    GLuint m_tableTexture = 0;
    bool m_tableHasTexCoords = false;
    float m_tableMinY;
    float m_tableMaxY;

    GLuint m_shaderProgram = 0;
    double lastTime = 0.0;

    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    GLuint m_skyboxVAO = 0, m_skyboxVBO = 0;
    GLuint m_sphereVAO = 0, m_sphereVBO = 0, m_sphereEBO = 0;
    GLuint m_skyboxTexture = 0;

    bool mouseButtonsDown[3] = { false, false, false };
    int m_sphereIndexCount = 0;

    const char* vertexShaderSrc = R"glsl(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec3 aNormal;
        layout(location = 2) in vec3 aColor;
        layout(location = 3) in vec2 aTexCoords;

        out vec3 FragPos;
        out vec3 Normal;
        out vec3 Color;
        out vec2 TexCoords;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        // material uniforms (populated from C++ side)
        uniform vec3 materialAmbient;
        uniform vec3 materialDiffuse;
        uniform vec3 materialSpecular;
        uniform float materialShininess;

        void main() {
            FragPos = vec3(model * vec4(aPos, 1.0));
            Normal = mat3(transpose(inverse(model))) * aNormal; // normal en mundo
            TexCoords = aTexCoords;
            Color = aColor;
            gl_Position = projection * view * vec4(FragPos, 1.0);
        }
    )glsl";

    const char* fragmentShaderSrc = R"glsl(
        #version 330 core
        in vec3 FragPos;
        in vec3 Normal;
        in vec3 Color;
        in vec2 TexCoords;

        out vec4 FragColor;

        uniform vec3 viewPos;
        uniform bool attenuationEnabled;
        uniform sampler2D texture_diffuse;
        uniform bool useTexture;
        uniform samplerCube skybox;

        uniform vec3 materialAmbient;
        uniform vec3 materialDiffuse;
        uniform vec3 materialSpecular;
        uniform float materialShininess;

        uniform vec3 lightPos[3];
        uniform vec3 lightAmbient[3];
        uniform vec3 lightDiffuse[3];
        uniform vec3 lightSpecular[3];
        uniform bool lightEnabled[3];
        uniform int lightModel[3]; 

        void main() {
            vec3 norm = normalize(Normal);
            vec3 viewDir = normalize(viewPos - FragPos);
            vec3 result = vec3(0.0);

            // Priorizamos la textura sobre el color por vértice
            vec4 texColor = texture(texture_diffuse, TexCoords);
            vec3 baseColor = useTexture ? texColor.rgb : Color;

            for (int i = 0; i < 3; i++) {
                if (!lightEnabled[i]) continue;

                vec3 lightDir = normalize(lightPos[i] - FragPos);
                float distance = length(lightPos[i] - FragPos);
                
                // Atenuación ajustada para distancias cortas (mesa mini-man)
                float attenuation = 1.0;
                if (attenuationEnabled) {
                    float constant = 1.0;
                    // valores reducidos para que la luz alcance distancias mayores dentro de la sala
                    float linear = 0.002;
                    float quadratic = 0.0005;
                    attenuation = 1.0 / (constant + linear * distance + quadratic * distance * distance);
                }

                    // Ambiental: usar componente ambiental de la luz y del material
                    vec3 ambient = lightAmbient[i] * materialAmbient * baseColor;

                    // Añadimos una ambient global para simular luz ambiente (día) y contribución del skybox
                    vec3 envColor = vec3(0.0);
                    // Sample environment using normal direction for soft ambient contribution
                    envColor = texture(skybox, normalize(Normal)).rgb;
                    vec3 globalAmbient = 0.35 * baseColor + 0.45 * envColor * baseColor;
                    ambient += globalAmbient;

                // Difuso multiplicado por la propiedad difusa del material
                float diff = max(dot(norm, lightDir), 0.0);
                vec3 diffuse = lightDiffuse[i] * diff * (materialDiffuse * baseColor) * 1.25;

                // Especular (Phong / Blinn-Phong), usando propiedad especular y shininess
                vec3 specular = vec3(0.0);
                if (lightModel[i] != 2) {
                    if (lightModel[i] == 0) { // Phong
                        vec3 reflectDir = reflect(-lightDir, norm);
                        float spec = pow(max(dot(viewDir, reflectDir), 0.0), materialShininess);
                        specular = lightSpecular[i] * spec * materialSpecular;
                    } else { // Blinn-Phong
                        vec3 halfwayDir = normalize(lightDir + viewDir);
                        float spec = pow(max(dot(norm, halfwayDir), 0.0), materialShininess);
                        specular = lightSpecular[i] * spec * materialSpecular;
                    }
                }

                result += (ambient + diffuse + specular) * attenuation;
            }
            // Final lighting output
            FragColor = vec4(clamp(result, 0.0, 1.0), 1.0);
        }
    )glsl";

    GLuint m_simpleShader = 0;
    GLuint m_lightShader = 0;
    const char* skyboxVertexSrc = R"glsl(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        out vec3 TexCoords;
        uniform mat4 projection;
        uniform mat4 view;
        void main() {
            TexCoords = aPos;
            vec4 pos = projection * view * vec4(aPos, 1.0);
            gl_Position = pos.xyww; // Truco para que siempre esté al fondo
        }
    )glsl";

    const char* skyboxFragmentSrc = R"glsl(
        #version 330 core
        out vec4 FragColor;
        in vec3 TexCoords;
        uniform samplerCube skybox;
        void main() {    
            FragColor = texture(skybox, TexCoords);
        }
    )glsl";

    // Shaders para dibujar las esferas de luz (simple color, transformaciones)
    const char* lightVertexSrc = R"glsl(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        void main() {
            gl_Position = projection * view * model * vec4(aPos, 1.0);
        }
    )glsl";

    const char* lightFragmentSrc = R"glsl(
        #version 330 core
        out vec4 FragColor;
        uniform vec3 color;
        void main() {
            FragColor = vec4(color, 1.0);
        }
    )glsl";

    glm::vec3 m_lightPos[3];
    float m_lightSpeedFactor;
    float m_lightRadii[3] = { 35.0f, 25.0f, 40.0f };
    float m_lightHeights[3] = { 45.0f, 42.0f, 48.0f };
    float m_lightAngularSpeed[3] = { 0.8f, 1.1f, 0.6f };

    // Camera / navigation state
    glm::vec3 cameraPos = glm::vec3(0.0f, 24.0f, 10.0f);
    glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
    float yaw = -90.0f;
    float pitch = 0.0f;
    float movementSpeed = 5.0f;
    float mouseSensitivity = 0.1f;
    bool firstMouse = true;
    double lastX = 0.0, lastY = 0.0;
    bool cursorCaptured = false;
    bool moveForward = false;
    bool moveBackward = false;
    double m_lastFrame = 0.0;
    bool isGodMode = false;

    // Funciones para inicializar geometria
    void setupTable();
    void setupSphere();
    void setupSkybox();
};
