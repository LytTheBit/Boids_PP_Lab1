#include <SFML/Graphics.hpp>
#include <chrono>
#include <iostream>

#include "config.hpp"
#include "boid.hpp"

enum class UpdateMode {
    Sequential,
    Parallel
};

int main() {
    // --- Finestra SFML ---
    sf::RenderWindow window(
        sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT),
        "Boids Simulation - Perf Test"
    );
    window.setFramerateLimit(60);

    // --- Sistema di boids ---
    constexpr std::size_t NUM_BOIDS = 300;
    BoidSystem boids(NUM_BOIDS);

    // --- Misura del tempo di simulazione ---
    UpdateMode mode = UpdateMode::Sequential;

    std::size_t frameCount = 0;
    double accumUpdateMs   = 0.0;
    double minUpdateMs     = std::numeric_limits<double>::max();
    double maxUpdateMs     = 0.0;

    // Per dt (tempo "fisico" della simulazione)
    sf::Clock frameClock;

    std::cout << "Comandi:\n"
              << "  S = modalità SEQUENZIALE\n"
              << "  P = modalità PARALLELA (OpenMP)\n"
              << "  R = azzera statistiche\n"
              << "  ESC / chiudi finestra = esci\n\n";

    while (window.isOpen()) {
        // --- Gestione eventi ---
        sf::Event event{};
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            } else if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Escape) {
                    window.close();
                } else if (event.key.code == sf::Keyboard::S) {
                    mode = UpdateMode::Sequential;
                    std::cout << "[MODE] Sequenziale\n";
                } else if (event.key.code == sf::Keyboard::P) {
                    mode = UpdateMode::Parallel;
                    std::cout << "[MODE] Parallelo (OpenMP)\n";
                } else if (event.key.code == sf::Keyboard::R) {
                    frameCount    = 0;
                    accumUpdateMs = 0.0;
                    minUpdateMs   = std::numeric_limits<double>::max();
                    maxUpdateMs   = 0.0;
                    std::cout << "[RESET] Statistiche azzerate\n";
                }
            }
        }

        // --- Delta time per la simulazione ---
        float dt = frameClock.restart().asSeconds();

        // --- Misura solo la parte di update ---
        auto t0 = std::chrono::high_resolution_clock::now();

        switch (mode) {
            case UpdateMode::Sequential:
                boids.updateSequential(dt);
                break;
            case UpdateMode::Parallel:
                boids.updateParallel(dt);
                break;
        }

        auto t1 = std::chrono::high_resolution_clock::now();
        double updateMs = std::chrono::duration<double, std::milli>(t1 - t0).count();

        // --- Aggiorna statistiche ---
        ++frameCount;
        accumUpdateMs += updateMs;
        if (updateMs < minUpdateMs) minUpdateMs = updateMs;
        if (updateMs > maxUpdateMs) maxUpdateMs = updateMs;

        // Ogni N frame stampa un riepilogo
        constexpr std::size_t PRINT_EVERY = 120; // ~2 secondi a 60 FPS
        if (frameCount % PRINT_EVERY == 0) {
            double avgMs = accumUpdateMs / static_cast<double>(frameCount);
            const char* modeStr = (mode == UpdateMode::Sequential) ? "SEQ" : "PAR";

            std::cout << "[STATS] mode=" << modeStr
                      << "  frames=" << frameCount
                      << "  avg_update=" << avgMs << " ms"
                      << "  min=" << minUpdateMs << " ms"
                      << "  max=" << maxUpdateMs << " ms\n";
        }

        // --- Rendering ---
        window.clear(sf::Color::Black);
        boids.draw(window);
        window.display();
    }

    return 0;
}
