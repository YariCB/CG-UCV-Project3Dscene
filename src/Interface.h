#ifndef INTERFACE_H
#define INTERFACE_H

#include "imgui.h"

// Estructura para agrupar los datos de la interfaz (diseño visual)
struct UIState {
    float lightSpeed = 1.0f;
    bool attenuation = true;
    int cameraMode = 0; // 0: FPS, 1: GOD
    float lightColors[3][3] = { {1,0.2f,0.1f}, {0.1f,1,0.2f}, {0.1f,0.2f,1} }; // Rojo, Verde, Azul con 20% de otros
    float lightAmbient[3][3] = { {0.1f,0.02f,0.01f}, {0.01f,0.1f,0.02f}, {0.01f,0.02f,0.1f} };
    float lightSpecular[3][3] = { {1,1,1}, {1,1,1}, {1,1,1} };
    bool lightEnabled[3] = { true, true, true };
    int shadingModels[3] = { 0, 0, 0 }; // 0: Phong, 1: Blinn-Phong, 2: Flat
    int selectedObj = 0;
    int sMapping = 0;
    int oMapping = 0;
    float bumpIntensity = 1.0f;
    bool loadNewDiffuse = false;
    bool loadNewBump = false;
    const char* diffuseFiles[2] = { "xerxes.jpg", "chibiXerxes.jpg" };
    const char* bumpFiles[2] = { "normalMapTexture.jpg", "normalMapTexture_2.jpg" };
    int currentDiffuseIndex = 0;
    int currentBumpIndex = 0;
    bool updateTextures = false;
    bool useBumpMap = false;
};

// Funciones de dibujo de la interfaz
void DrawMainPanel(UIState& state);

#endif