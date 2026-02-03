#pragma once

constexpr unsigned int WINDOW_WIDTH  = 1600;  // larghezza finestra
constexpr unsigned int WINDOW_HEIGHT = 1200;  // altezza finestra

// Parametri del modello boids
constexpr float MAX_SPEED          = 120.0f;  // velocità di crociera massima
constexpr float MIN_SPEED          = 40.f;    // velocità di crociera minima
constexpr float PERCEPTION_RADIUS  = 60.0f;   // raggio vicinato
constexpr float SEPARATION_RADIUS  = 20.0f;   // distanza minima
constexpr float ALIGN_WEIGHT       = 0.5f;    // peso dell'allineamento
constexpr float COHESION_WEIGHT    = 0.3f;    // peso della coesione
constexpr float SEPARATION_WEIGHT  = 0.8f;    // peso della separazione
