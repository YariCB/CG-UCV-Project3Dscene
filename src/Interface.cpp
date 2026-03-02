#include "Interface.h"

void DrawMainPanel(UIState& state) {
    ImGui::Begin("Control Panel - Project 3");

    // 1. Iluminación y Animación
    if (ImGui::CollapsingHeader("Lighting and Animation", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SliderFloat("Lights Speed", &state.lightSpeed, 0.0f, 5.0f);
        ImGui::Checkbox("Enable Attenuation (f_att)", &state.attenuation);
    }

    // 2. Control de las 3 Luciérnagas (R, G, B)
    if (ImGui::CollapsingHeader("Lighting Settings")) {
        const char* names[] = { "Red Light", "Green Light", "Blue Light" };
        const char* models[] = { "Phong", "Blinn-Phong", "Flat" };

        for (int i = 0; i < 3; i++) {
            ImGui::PushID(i);
            ImGui::Separator();
            ImGui::Text("%s", names[i]);
            ImGui::Checkbox("Enable", &state.lightEnabled[i]);
            ImGui::ColorEdit3("Diffuse", state.lightColors[i]);
            ImGui::ColorEdit3("Ambient", state.lightAmbient[i]);
            ImGui::ColorEdit3("Specular", state.lightSpecular[i]);
            ImGui::Combo("Model", &state.shadingModels[i], models, 3);
            ImGui::PopID();
        }
    }

    // 3. Navegación
    if (ImGui::CollapsingHeader("Navigation / Camera")) {
        ImGui::RadioButton("FPS Mode (Mini-man)", &state.cameraMode, 0);
        ImGui::SameLine();
        ImGui::RadioButton("GOD Mode", &state.cameraMode, 1);
        ImGui::Text("Right-click for free movement");
    }

    // 4. Mapeos y Selección de Objetos (Group Box)
    if (ImGui::CollapsingHeader("Objects and Textures")) {
        const char* items[] = { "Wooden table", "Reflective object", "Box", "Teapot" };
        ImGui::ListBox("Select Object", &state.selectedObj, items, 4);

        ImGui::Text("Surface Mapping:");
        ImGui::Combo("S-Mapping", &state.sMapping, "Spherical\0Cylindrical\0");
        ImGui::Combo("O-Mapping", &state.oMapping, "Plane\0Cubic\0");

        if (ImGui::Button("Change Bump Texture")) {
            // Lógica para el selector de archivos
        }
    }

    ImGui::End();
}