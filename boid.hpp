#pragma once
#include <SFML/Graphics.hpp>
#include <vector>

struct Boid {
    sf::Vector2f position; // posizione
    sf::Vector2f velocity; // velocità
    float headingDeg = 0.f; // direzione in gradi
};

class BoidSystem {
public:
    explicit BoidSystem(std::size_t count);

    void updateSequential(float dt); // versione sequenziale
    void updateParallel(float dt);   // versione OpenMP

    void draw(sf::RenderWindow& window) const;

private:
    std::vector<Boid> m_boids;   // stato attuale
    std::vector<Boid> m_next;    // double buffering

    static void wrapPosition(sf::Vector2f& p);
    static sf::ConvexShape makeBoidShape(const Boid& b);
    Boid computeBoidUpdate(std::size_t i, float dt) const;
};
