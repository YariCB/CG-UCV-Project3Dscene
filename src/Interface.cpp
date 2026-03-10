#include "Interface.h"

void DrawMainPanel(UIState& state) {
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(400, 1000), ImGuiCond_FirstUseEver);
    
    ImGui::Begin("Control Panel - Project 3", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize);

    // Iluminacion y Animacion
    if (ImGui::CollapsingHeader("Lighting and Animation", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SliderFloat("Lights Speed", &state.lightSpeed, 0.0f, 5.0f);
        ImGui::Checkbox("Enable Attenuation (f_att)", &state.attenuation);
    }

    // Control de las 3 Luciernagas (R, G, B)
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

    // Navegacion
    if (ImGui::CollapsingHeader("Navigation / Camera")) {
        ImGui::RadioButton("FPS Mode", &state.cameraMode, 0);
        ImGui::SameLine();
        ImGui::RadioButton("GOD Mode", &state.cameraMode, 1);
        ImGui::Text("Right-click to look and move freely.\n- In FPS mode, you cannot move up or down.\n- In GOD mode, movement is unrestricted\nand in any direction you are looking.\nKeys: Arrow keys or AWSD.");
    }

    // Mapeos, Seleccion de Objetos y Bump Mapping
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
        ImGui::Text("Texture Generation Mode:");
        const char* items[] = { "None", "Cards Tower", "Coffee cups", "Cup", "Fruit bowl", "Teapot", "Wooden table", "Parametric Sphere" };
        ImGui::ListBox("Select Object", &state.selectedObj, items, 8);
        ImGui::RadioButton("Original", &state.texGenMode, 0); ImGui::SameLine();
        ImGui::RadioButton("O-Mapping", &state.texGenMode, 1); ImGui::SameLine();
        ImGui::RadioButton("S-Mapping", &state.texGenMode, 2);

        if (state.texGenMode == 1) {
            ImGui::Combo("O-Mapping Type", &state.oMapping, "Plane\0Cubic\0");
        }
        else if (state.texGenMode == 2) {
            ImGui::Combo("S-Mapping Type", &state.sMapping, "Spherical\0Cylindrical\0");
        }
    }

    ImGui::End();
}