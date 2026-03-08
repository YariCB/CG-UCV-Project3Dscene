#include "Interface.h"

void DrawMainPanel(UIState& state) {
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(400, 1000), ImGuiCond_FirstUseEver);
    
    ImGui::Begin("Control Panel - Project 3", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize);

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
        ImGui::RadioButton("FPS Mode", &state.cameraMode, 0);
        ImGui::SameLine();
        ImGui::RadioButton("GOD Mode", &state.cameraMode, 1);
        ImGui::Text("Right-click for free movement");
    }

    // 4. Mapeos, Selección de Objetos y Bump Mapping
    if (ImGui::CollapsingHeader("Objects and Textures")) {
        
        ImGui::Text("Parametric Surface - Bump Mapping:");
        ImGui::Text("Object: Parametric Sphere");
        ImGui::BulletText("Diffuse: %s", state.diffuseFiles[state.currentDiffuseIndex]);
        if (ImGui::Button("Next Diffuse Texture")) {
            state.currentDiffuseIndex = (state.currentDiffuseIndex + 1) % 2; // Rota entre 0 y 1
            state.updateTextures = true;
        }
        ImGui::Spacing();
        ImGui::Checkbox("Enable Bump Mapping", &state.useBumpMap);
        ImGui::BulletText("Bump: %s", state.bumpFiles[state.currentBumpIndex]);
        if (ImGui::Button("Next Bump Texture")) {
            state.currentBumpIndex = (state.currentBumpIndex + 1) % 2; // Rota entre 0 y 1
            state.updateTextures = true;
        }
        // Extra: Controlar que tan fuerte se ve el relieve
        ImGui::SliderFloat("Bump Intensity", &state.bumpIntensity, 0.0f, 5.0f);

        ImGui::Separator();
        ImGui::Text("Textures Selection:");
        const char* items[] = { "None", "Cards", "Cards Tower", "Coffee cups", "Cup", "Fruit bowl", "Teapot", "Wooden table", "Parametric Object" };
        ImGui::ListBox("Select Object", &state.selectedObj, items, 9);

        ImGui::Separator();
        ImGui::Text("Mapping Controls:");
        ImGui::Combo("S-Mapping", &state.sMapping, "Spherical\0Cylindrical\0");
        ImGui::Combo("O-Mapping", &state.oMapping, "Plane\0Cubic\0");
    }

    ImGui::End();
}