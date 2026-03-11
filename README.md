# Escena 3D — Proyecto 3

Proyecto desarrollado en C++ y OpenGL como parte de la asignatura "Introducción a la Computación Gráfica" (Universidad Central de Venezuela, semestre 2-2025). El repositorio contiene un proyecto de Visual Studio listo para compilar y ejecutar en Windows.

**Descripción:**
- Muestra una escena 3D que representa una sala cálida y tranquila. El usuario controla a un "miniman" (una cámara en primera persona a escala reducida) que camina sobre una mesa en la escena.
- Sobre la mesa hay varios objetos (tetera, cartas, tazas de café, una taza, un bowl con frutas y una esfera paramétrica) que incluyen animaciones y comportamiento interactivo.

**Características principales:**
- Renderizado con OpenGL (core profile 3.3).
- Skybox y cubemap dinámico para reflexiones en la tetera.
- Iluminación con múltiples fuentes puntuales y control de atenuación.
- Bump mapping (normal mapping) aplicado a una esfera paramétrica.
- Generación de coordenadas de textura: O-mapping (planar / cúbico) y S-mapping (esférico / cilíndrico) aplicables a objetos seleccionados.
- Interfaz GUI (ImGui) para ajustar parámetros en tiempo real: luces, velocidades de animación, atenuación, selección de texturas, opciones de mapeo, etc.

**Ejecución:**
- Abrir `Proyecto2.sln` en Visual Studio (x64). Compilar en la configuración `Debug` o `Release` y ejecutar el binario situado en `x64\Debug\` o `x64\Release\`.

**Controles y funciones:**
- Movimiento: mover la cámara/miniman con las teclas `W/A/S/D` o flechas.
- Look: capturar el cursor con el botón derecho del ratón y mover para rotar la cámara.
- GUI: ajustar luces, atenuación, texturas y modos de mapeo desde el panel de control (ImGui) en pantalla.
- Reflexiones dinámicas: la tetera usa una cubemap dinámica que captura el entorno (skybox + objetos) para producir reflexiones en tiempo real.

**Archivos relevantes:**
- `src/` — Código fuente en C++ (principalmente `3DViewer.cpp`, `ModelLoader.cpp`, `Interface.cpp`, `main.cpp`).
- `include/` — Librerías y dependencias empaquetadas (glfw, glad, glm, assimp reducida, stb, etc.).
- `OBJs/` — Modelos y texturas usadas en la escena.
- `Proyecto2.sln` — Solución de Visual Studio.

**Trabajo práctico:**
- El enunciado completo del proyecto (requisitos y entregables) se encuentra en `proyecto 3.pdf` incluido en la entrega del curso.