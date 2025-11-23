#include <SFML/Graphics.hpp>

#include "config.hpp"
#include "boid.hpp"

int main() {
    // Crea la finestra
    sf::RenderWindow window(
        sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT),
        "Boids Simulation"
    );
    window.setFramerateLimit(60);

    // Sistema di boids: ad es. 300 boids
    BoidSystem boids(300);

    sf::Clock clock;

    while (window.isOpen()) {
        // Gestione eventi
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed ||
               (event.type == sf::Event::KeyPressed &&
                event.key.code == sf::Keyboard::Escape)) {
                window.close();
                }
        }

        // Delta time
        float dt = clock.restart().asSeconds();

        // Aggiornamento della simulazione
        // --- scegli quale versione usare ---
        boids.updateSequential(dt);   // versione sequenziale
        // boids.updateParallel(dt);  // versione parallela con OpenMP

        // Rendering
        window.clear(sf::Color::Black);
        boids.draw(window);
        window.display();
    }

    return 0;
}


