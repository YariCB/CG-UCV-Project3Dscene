#include "3DViewer.h"
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Animación de las cartas
void C3DViewer::updateCardsAnimation(double deltaTime) {
    m_cardsAnimTimer += (float)deltaTime;
    if (m_cardsAnimTimer >= m_cardsCyclePeriod) {
        m_cardsAnimTimer = 0.0f;
        m_cardsCollapsed = !m_cardsCollapsed;
        m_cardsAnimPhase = 0.0f;
    }
    else {
        m_cardsAnimPhase = m_cardsAnimTimer / m_cardsCyclePeriod;
    }
}

// Animación de la tetera
void C3DViewer::updateTeapotAnimation(double deltaTime) {
    const float moveFactor = 0.33f; // Desplazamiento
    const float targetYaw = glm::radians(-90.0f);

    m_teapotAnimTimer += (float)deltaTime;
    if (m_teapotAnimTimer >= m_teapotCyclePeriod) {
        m_teapotAnimTimer = 0.0f;
        m_teapotAnimStage = 0;
    }

    // Duración de animación (40 segs)
    const float stageDurations[] = {
        2.0f, // 0: levantar
        3.0f, // 1: desplazar hacia taza
        2.0f, // 2: rotar para apuntar
        2.0f, // 3: inclinar
        2.0f, // 4: volver vertical
        3.0f, // 5: regresar
        2.0f, // 6: bajar
        24.0f // 7: reposo
    };
    const int numStages = sizeof(stageDurations) / sizeof(float);

    float elapsed = m_teapotAnimTimer;
    int stage = 0;
    while (stage < numStages && elapsed > stageDurations[stage]) {
        elapsed -= stageDurations[stage];
        stage++;
    }
    if (stage >= numStages) stage = numStages - 1;
    m_teapotAnimStage = stage;
    float progress = elapsed / stageDurations[stage]; // 0..1

    // Parametros de la mesa
    float scale = 0.5f;
    float tableHeight = scale * (m_tableMaxY - m_tableMinY);
    float tableHalfZ = ((m_tableMaxZ - m_tableMinZ) * 0.5f) * scale;

    // Posicion de la taza cup
    float cupHeightModel = (m_cupMaxY - m_cupMinY);
    float cupScale = (cupHeightModel > 0.0001f) ? (tableHeight * 0.12f) / cupHeightModel : 1.0f;
    float cupDisplacementY = -m_cupMinY * cupScale + tableHeight + 0.01f;
    glm::vec3 cupPos(0.0f, cupDisplacementY, -tableHalfZ + 3.0f);

    // Altura base de la tetera (sin animación)
    float teapotHeightModel = (m_teapotMaxY - m_teapotMinY);
    float teapotScale = (teapotHeightModel > 0.0001f) ? (tableHeight * 0.30f) / teapotHeightModel : 1.0f;
    float teapotBaseY = -m_teapotMinY * teapotScale + tableHeight + 0.02f;

    const float liftHeight = 1.0f;        // cuánto se eleva
    const float maxPitch = glm::radians(45.0f); // inclinación máxima

    glm::vec3 targetExtraPos(0.0f);
    float targetExtraPitch = 0.0f;
    float targetExtraYaw = 0.0f;

    switch (stage) {
    case 0: // levantar
        targetExtraPos.y = liftHeight * progress;
        targetExtraYaw = 0.0f;
        break;
    case 1: // desplazar hacia la taza
        targetExtraPos.y = liftHeight;
        targetExtraPos.z = cupPos.z * moveFactor * progress;
        targetExtraYaw = 0.0f;
        break;
    case 2: // rotar para apuntar
        targetExtraPos.y = liftHeight;
        targetExtraPos.z = cupPos.z * moveFactor;
        targetExtraYaw = targetYaw * progress;
        break;
    case 3: // inclinar
        targetExtraPos.y = liftHeight;
        targetExtraPos.z = cupPos.z * moveFactor;
        targetExtraYaw = targetYaw;
        targetExtraPitch = maxPitch * progress;
        break;
    case 4: // volver vertical
        targetExtraPos.y = liftHeight;
        targetExtraPos.z = cupPos.z * moveFactor;
        targetExtraYaw = targetYaw;
        targetExtraPitch = maxPitch * (1.0f - progress);
        break;
    case 5: // regresar
        targetExtraPos.y = liftHeight;
        targetExtraPos.z = cupPos.z * moveFactor * (1.0f - progress);
        targetExtraYaw = targetYaw * (1.0f - progress);
        break;
    case 6: // bajar
        targetExtraPos.y = liftHeight * (1.0f - progress);
        targetExtraYaw = 0.0f;
        break;
    case 7: // reposo
        targetExtraPos = glm::vec3(0.0f);
        targetExtraYaw = 0.0f;
        targetExtraPitch = 0.0f;
        break;
    }

    m_teapotExtraPos = targetExtraPos;
    m_teapotExtraPitch = targetExtraPitch;
    m_teapotExtraYaw = targetExtraYaw;
}

// Animación de batido del café
void C3DViewer::updateCoffeeAnimation(double deltaTime) {
    // Duraciones de cada etapa (en segundos)
    const float stageDurations[] = {
        2.0f, // 0: levantar y desplazar
        1.0f, // 1: inclinar
        3.0f, // 2: batir
        1.0f, // 3: desinclinar
        2.0f, // 4: bajar y regresar
		21.0f // 5: reposo
    };
    const int numStages = sizeof(stageDurations) / sizeof(float);
    float cycleTotal = 0.0f;
    for (int i = 0; i < numStages; ++i) cycleTotal += stageDurations[i];

    m_coffeeAnimTimer += (float)deltaTime;
    if (m_coffeeAnimTimer >= cycleTotal) {
        m_coffeeAnimTimer = 0.0f;
    }

    // Determinar etapa actual
    float elapsed = m_coffeeAnimTimer;
    int stage = 0;
    while (stage < numStages && elapsed > stageDurations[stage]) {
        elapsed -= stageDurations[stage];
        stage++;
    }
    if (stage >= numStages) stage = numStages - 1;
    float progress = (stageDurations[stage] > 0.0f) ? elapsed / stageDurations[stage] : 0.0f;

    // Parámetros de animación
    const float maxLift = 0.4f;          // altura maxima
    const float maxShiftLeftX = -0.5f;    // desplazamiento en X para la izquierda
    const float maxShiftRightX = 0.3f;   // desplazamiento en X para la derecha
    const float maxShiftZ = 0.2f;        // desplazamiento en Z
    const float tiltAngle = glm::radians(90.0f); // ángulo de inclinacion

    // Valores por defecto (sin transformación)
    glm::vec3 leftPos(0.0f), rightPos(0.0f);
    float leftRotZ = 0.0f, rightRotZ = 0.0f;
    float leftRotY = 0.0f, rightRotY = 0.0f; // para rotación durante batido

    switch (stage) {
    case 0: // Levantar y desplazar
    {
        // Interpolación lineal de 0 a 1
        leftPos = glm::vec3(maxShiftLeftX * progress, maxLift * progress, (-maxShiftZ*2) * progress);
        rightPos = glm::vec3(maxShiftRightX * progress, maxLift * progress, maxShiftZ * progress);
        break;
    }
    case 1: // Inclinar
    {
        // Mantener la posición máxima alcanzada en stage 0
        leftPos = glm::vec3(maxShiftLeftX, maxLift, -maxShiftZ*2);
        rightPos = glm::vec3(maxShiftRightX, maxLift, maxShiftZ);
        // Rotación progresiva
        leftRotZ = -tiltAngle * progress;
        rightRotZ = tiltAngle * progress;
        break;
    }
    case 2: // Batir (movimiento circular)
    {
        glm::vec3 centerLeft(maxShiftLeftX, maxLift, -maxShiftZ*2);
        glm::vec3 centerRight(maxShiftRightX, maxLift, maxShiftZ);

        float angle = progress * 2.0f * glm::pi<float>() * 2.0f; // 2 vueltas
        float radius = 0.15f;
        glm::vec3 offset(sin(angle) * radius, 0.0f, cos(angle) * radius);
        leftPos = centerLeft + offset;
        rightPos = centerRight + offset;

        // Mantener la inclinación constante
        leftRotZ = -tiltAngle;
        rightRotZ = tiltAngle;
        break;
    }
    case 3: // Desinclinar
    {
        float angle_end = 2.0f * glm::pi<float>() * 2.0f;
        glm::vec3 offset_end(sin(angle_end) * 0.15f, 0.0f, cos(angle_end) * 0.15f);
        glm::vec3 centerLeft(maxShiftLeftX, maxLift, -maxShiftZ*2);
        glm::vec3 centerRight(maxShiftRightX, maxLift, maxShiftZ);
        leftPos = centerLeft + offset_end;
        rightPos = centerRight + offset_end;

        // Desinclinacion: de tiltAngle a 0
        leftRotZ = -tiltAngle * (1.0f - progress);
        rightRotZ = tiltAngle * (1.0f - progress);
        break;
    }
    case 4: // Bajar y regresar
    {
        float angle_end = 2.0f * glm::pi<float>() * 2.0f;
        glm::vec3 offset_end(sin(angle_end) * 0.15f, 0.0f, cos(angle_end) * 0.15f);
        glm::vec3 centerLeft(maxShiftLeftX, maxLift, -maxShiftZ*2);
        glm::vec3 centerRight(maxShiftRightX, maxLift, maxShiftZ);
        glm::vec3 leftEnd = centerLeft + offset_end;
        glm::vec3 rightEnd = centerRight + offset_end;

        // Interpolar hacia cero
        leftPos = glm::mix(leftEnd, glm::vec3(0.0f), progress);
        rightPos = glm::mix(rightEnd, glm::vec3(0.0f), progress);
        // Rotacion vuelve a cero
        leftRotZ = 0.0f;
        rightRotZ = 0.0f;
        break;
    }
	case 5: // Reposo
	{
		leftPos = glm::vec3(0.0f);
		rightPos = glm::vec3(0.0f);
		leftRotZ = 0.0f;
		rightRotZ = 0.0f;
		break;
	}
	}

    // Construccion de matrices: trasladar, luego rotar
    glm::mat4 animLeft = glm::mat4(1.0f);
    animLeft = glm::translate(animLeft, leftPos);
    if (leftRotZ != 0.0f) animLeft = glm::rotate(animLeft, leftRotZ, glm::vec3(0, 0, 1));
    if (leftRotY != 0.0f) animLeft = glm::rotate(animLeft, leftRotY, glm::vec3(0, 1, 0));

    glm::mat4 animRight = glm::mat4(1.0f);
    animRight = glm::translate(animRight, rightPos);
    if (rightRotZ != 0.0f) animRight = glm::rotate(animRight, rightRotZ, glm::vec3(0, 0, 1));
    if (rightRotY != 0.0f) animRight = glm::rotate(animRight, rightRotY, glm::vec3(0, 1, 0));

    // Asignacion al vector
    if (m_coffeeSpoonExtraTransforms.size() >= 2) {
        m_coffeeSpoonExtraTransforms[0] = animLeft;
        m_coffeeSpoonExtraTransforms[1] = animRight;
    }
}
