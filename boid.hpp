#pragma once
#include <SFML/Graphics.hpp>
#include <vector>

struct boid {
    sf::Vector2f position;
    sf::Vector2f velocity;
};

class BoidSystem {
public:
    explicit BoidSystem(std::size_t count);

    void updateSequential(float dt);
    void updateParallel(float dt);   // versione OpenMP

    void draw(sf::RenderWindow& window) const;

private:
    std::vector<boid> m_boids;   // stato attuale
    std::vector<boid> m_next;    // double buffering

    static void wrapPosition(sf::Vector2f& p);
    static sf::ConvexShape makeBoidShape(const boid& b);
};
