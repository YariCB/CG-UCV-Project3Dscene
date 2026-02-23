#ifndef INTERFACE_H
#define INTERFACE_H

#include "imgui.h"

// Estructura para agrupar los datos de la interfaz (diseño visual)
struct UIState {
    float lightSpeed = 1.0f;
    bool attenuation = true;
    int cameraMode = 0; // 0: FPS, 1: GOD
    float lightColors[3][3] = { {1,0,0}, {0,1,0}, {0,0,1} };
    int shadingModels[3] = { 0, 0, 0 };
    int selectedObj = 0;
    int sMapping = 0;
    int oMapping = 0;
};

// Funciones de dibujo de la interfaz
void DrawMainPanel(UIState& state);

#endif