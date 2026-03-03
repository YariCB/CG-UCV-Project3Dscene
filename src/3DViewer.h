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
    // Carga OBJ a una malla específica (no sobrescribe la mesa)
    void loadOBJTo(const std::string& path, GLuint& outVAO, GLuint& outVBO, size_t& outVertexCount, bool& outHasTexCoords, float& outMinY, float& outMaxY);
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

    // Datos para la mesa
    GLuint m_tableVAO = 0;
    GLuint m_tableVBO = 0;
    size_t m_tableVertexCount = 0;
    GLuint m_tableTexture = 0;
    bool m_tableHasTexCoords = false;
    float m_tableMinY;
    float m_tableMaxY;

    // Datos para la tetera
    GLuint m_teapotVAO = 0;
    GLuint m_teapotVBO = 0;
    size_t m_teapotVertexCount = 0;
    float m_teapotMinY, m_teapotMaxY;
    GLuint m_teapotTexture = 0;
    bool m_teapotHasTexCoords = false;

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

        uniform bool isReflective;

        void main() {
            vec3 norm = normalize(Normal);
            vec3 viewDir = normalize(viewPos - FragPos);

            // 1. COLOR BASE
            vec4 texColor = texture(texture_diffuse, TexCoords);
            vec3 baseColor;

            if (isReflective) {
                // Plateado oscuro (metal)
                baseColor = vec3(0.12, 0.12, 0.12);
            } else {
                baseColor = useTexture ? texColor.rgb : (length(Color) > 0.1 ? Color : vec3(0.6));
            }

            // 2. AMBIENTE (contribución del skybox)
            vec3 skyColor = texture(skybox, norm).rgb;
            vec3 ambient = skyColor * 0.3 * baseColor;

            vec3 lightingSum = vec3(0.0);

            // 3. LUCES PUNTUALES
            for (int i = 0; i < 3; i++) {
                if (!lightEnabled[i]) continue;

                vec3 lightDir = normalize(lightPos[i] - FragPos);
                float distance = length(lightPos[i] - FragPos);

                float attenuation = 1.0;
                if (attenuationEnabled) {
                    attenuation = 1.0 / (1.0 + 0.01 * distance + 0.0002 * distance * distance);
                }

                // Difuso
                float diff = max(dot(norm, lightDir), 0.0);
                vec3 diffuse;
                if (isReflective) {
                    // Para la tetera, el color de la luz se suma directamente con bastante intensidad
                    // y se mezcla con el color base oscuro.
                    diffuse = lightDiffuse[i] * diff * 2.0; // factor alto para que se note el color
                } else {
                    diffuse = lightDiffuse[i] * diff * materialDiffuse * baseColor;
                }

                // Especular (Blinn-Phong)
                vec3 halfwayDir = normalize(lightDir + viewDir);
                float spec = pow(max(dot(norm, halfwayDir), 0.0), materialShininess);
                vec3 specular;
                if (isReflective) {
                    // Especular muy brillante, con el color de la luz
                    specular = lightSpecular[i] * spec * materialSpecular * 12.0;
                } else {
                    specular = lightSpecular[i] * spec * materialSpecular * baseColor;
                }

                lightingSum += (diffuse + specular) * attenuation;
            }

            // 4. REFLEXIÓN (para objetos reflectivos)
            vec3 finalColor = ambient + lightingSum;

            if (isReflective) {
                vec3 reflectDir = reflect(-viewDir, norm);
                vec3 reflectionColor = texture(skybox, reflectDir).rgb;
                reflectionColor *= baseColor; // el metal oscuro tiñe el reflejo

                // Fresnel para metales
                float cosTheta = max(dot(norm, viewDir), 0.0);
                float F0 = 0.92;
                float fresnel = F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);

                finalColor = mix(finalColor, reflectionColor, fresnel);
            }

            // Tonemapping suave
            finalColor = finalColor / (finalColor + vec3(0.2));
            finalColor = pow(finalColor, vec3(1.0 / 1.2));

            FragColor = vec4(clamp(finalColor, 0.0, 1.0), 1.0);
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

    // Shaders para dibujar las esferas de luz
    const char* lightVertexSrc = R"glsl(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        out vec3 LocalPos;
        out vec3 FragPos;
        out vec3 Normal;
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        void main() {
            LocalPos = aPos; // unit-sphere local position
            FragPos = vec3(model * vec4(aPos, 1.0));
            Normal = mat3(transpose(inverse(model))) * aPos;
            gl_Position = projection * view * model * vec4(aPos, 1.0);
        }
    )glsl";

    const char* lightFragmentSrc = R"glsl(
        #version 330 core
        in vec3 LocalPos;
        in vec3 FragPos;
        in vec3 Normal;
        out vec4 FragColor;
        uniform vec3 color;
        uniform vec3 viewPos;
        // glow control: 0 = core, 1 = glow
        uniform int glowPass;
        uniform float glowIntensity;
        uniform float glowFalloff; // larger -> tighter
        void main() {
            if (glowPass == 0) {
                // Core pass: solid sphere color
                FragColor = vec4(color, 1.0);
            } else {
                // Glow pass: compute radial falloff in local sphere space
                float r = length(LocalPos);
                // Local sphere has radius ~1. Remap so edges (r near 1.0)
                float edge = smoothstep(1.0, 1.0 - (0.5 / glowFalloff), r);
                // Also add a rim by view angle for thin highlights
                vec3 N = normalize(Normal);
                vec3 V = normalize(viewPos - FragPos);
                float rim = pow(1.0 - max(dot(N, V), 0.0), 2.0);
                float alpha = (1.0 - edge) * glowIntensity * rim;
                // output additive color (alpha used by blending)
                FragColor = vec4(color * alpha, alpha);
            }
        }
    )glsl";

    glm::vec3 m_lightPos[3];
    float m_lightSpeedFactor;
    float m_lightRadii[3] = { 45.0f, 25.0f, 40.0f };
    float m_lightHeights[3] = { 45.0f, 42.0f, 48.0f };
    float m_lightAngularSpeed[3] = { 0.8f, 1.1f, 0.6f };

    // Camera / navigation state
    glm::vec3 cameraPos = glm::vec3(0.0f, 38.0f, 10.0f);
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
