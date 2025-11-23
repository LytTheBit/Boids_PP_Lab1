#pragma once

constexpr unsigned int WINDOW_WIDTH  = 800;
constexpr unsigned int WINDOW_HEIGHT = 600;

// Parametri del modello boids
constexpr float MAX_SPEED          = 120.0f;
constexpr float PERCEPTION_RADIUS  = 60.0f;   // raggio vicinato
constexpr float SEPARATION_RADIUS  = 20.0f;   // distanza minima
constexpr float ALIGN_WEIGHT       = 0.5f;
constexpr float COHESION_WEIGHT    = 0.3f;
constexpr float SEPARATION_WEIGHT  = 0.8f;
