#include "boid.hpp"
#include "config.hpp"
#include <random>
#include <cmath>
#include <omp.h>

// Inizializza i boids con posizioni e velocità casuali
BoidSystem::BoidSystem(std::size_t count)
    : m_boids(count), m_next(count)
{
    std::mt19937 rng{42};
    std::uniform_real_distribution<float> xDist(0.f, static_cast<float>(WINDOW_WIDTH));
    std::uniform_real_distribution<float> yDist(0.f, static_cast<float>(WINDOW_HEIGHT));
    std::uniform_real_distribution<float> vDist(-50.f, 50.f);

    for (auto &b : m_boids) {
        b.position = {xDist(rng), yDist(rng)};
        b.velocity = {vDist(rng), vDist(rng)};
    }
}

// Gestisce il wrapping delle posizioni ai bordi della finestra
void BoidSystem::wrapPosition(sf::Vector2f& p) {
    if (p.x < 0.f) p.x += WINDOW_WIDTH;
    else if (p.x >= WINDOW_WIDTH) p.x -= WINDOW_WIDTH;

    if (p.y < 0.f) p.y += WINDOW_HEIGHT;
    else if (p.y >= WINDOW_HEIGHT) p.y -= WINDOW_HEIGHT;
}

// Disegna tutti i boids
void BoidSystem::draw(sf::RenderWindow& window) const {
    for (const auto& b : m_boids) {
        sf::ConvexShape shape = makeBoidShape(b);
        window.draw(shape);
    }
}

// Crea la forma grafica di un boid
sf::ConvexShape BoidSystem::makeBoidShape(const Boid& boid) {
    sf::ConvexShape shape;
    shape.setPointCount(3);
    float size = 8.f;
    shape.setPoint(0, {0.f, -size});
    shape.setPoint(1, {-size, size});
    shape.setPoint(2, {size, size});
    shape.setFillColor(sf::Color::White);
    shape.setPosition(boid.position);

    float angle = std::atan2(boid.velocity.y, boid.velocity.x) * 180.f / 3.14159265f + 90.f;
    shape.setRotation(angle);
    return shape;
}


// Versione sequenziale
void BoidSystem::updateSequential(float dt) {
    const std::size_t n = m_boids.size();

    for (std::size_t i = 0; i < n; ++i) {
        const Boid& self = m_boids[i];

        sf::Vector2f alignment{0.f, 0.f};
        sf::Vector2f cohesion{0.f, 0.f};
        sf::Vector2f separation{0.f, 0.f};
        int neighbors = 0;

        for (std::size_t j = 0; j < n; ++j) {
            if (j == i) continue;

            sf::Vector2f diff = m_boids[j].position - self.position;
            float dist2 = diff.x * diff.x + diff.y * diff.y;

            if (dist2 < PERCEPTION_RADIUS * PERCEPTION_RADIUS) {
                ++neighbors;
                alignment += m_boids[j].velocity;
                cohesion  += m_boids[j].position;

                if (dist2 < SEPARATION_RADIUS * SEPARATION_RADIUS) {
                    separation -= diff; // spinge lontano
                }
            }
        }

        sf::Vector2f vel = self.velocity;
        if (neighbors > 0) {
            float inv = 1.0f / neighbors;
            alignment *= inv;
            cohesion  *= inv;
            cohesion  -= self.position;  // verso il centro

            vel += ALIGN_WEIGHT      * (alignment - self.velocity)
                 + COHESION_WEIGHT   * cohesion
                 + SEPARATION_WEIGHT * separation;
        }

        // Limita la velocità
        float speed2 = vel.x * vel.x + vel.y * vel.y;
        if (speed2 > MAX_SPEED * MAX_SPEED) {
            float factor = MAX_SPEED / std::sqrt(speed2);
            vel.x *= factor;
            vel.y *= factor;
        }

        Boid out;
        out.velocity  = vel;
        out.position  = self.position + vel * dt;
        wrapPosition(out.position);

        m_next[i] = out;
    }

    // commit dello step
    m_boids.swap(m_next);
}

// Versione parallela con OpenMP
void BoidSystem::updateParallel(float dt) {
    const std::size_t n = m_boids.size();

#pragma omp parallel for schedule(static)
    for (int i = 0; i < static_cast<int>(n); ++i) {
        const Boid& self = m_boids[i];

        sf::Vector2f alignment{0.f, 0.f};
        sf::Vector2f cohesion{0.f, 0.f};
        sf::Vector2f separation{0.f, 0.f};
        int neighbors = 0;

        for (std::size_t j = 0; j < n; ++j) {
            if (j == static_cast<std::size_t>(i)) continue;

            sf::Vector2f diff = m_boids[j].position - self.position;
            float dist2 = diff.x * diff.x + diff.y * diff.y;

            if (dist2 < PERCEPTION_RADIUS * PERCEPTION_RADIUS) {
                ++neighbors;
                alignment += m_boids[j].velocity;
                cohesion  += m_boids[j].position;

                if (dist2 < SEPARATION_RADIUS * SEPARATION_RADIUS) {
                    separation -= diff;
                }
            }
        }

        sf::Vector2f vel = self.velocity;
        if (neighbors > 0) {
            float inv = 1.0f / neighbors;
            alignment *= inv;
            cohesion  *= inv;
            cohesion  -= self.position;

            vel += ALIGN_WEIGHT      * (alignment - self.velocity)
                 + COHESION_WEIGHT   * cohesion
                 + SEPARATION_WEIGHT * separation;
        }

        float speed2 = vel.x * vel.x + vel.y * vel.y;
        if (speed2 > MAX_SPEED * MAX_SPEED) {
            float factor = MAX_SPEED / std::sqrt(speed2);
            vel.x *= factor;
            vel.y *= factor;
        }

        Boid out;
        out.velocity  = vel;
        out.position  = self.position + vel * dt;
        wrapPosition(out.position);

        m_next[static_cast<std::size_t>(i)] = out;
    }

    m_boids.swap(m_next);
}