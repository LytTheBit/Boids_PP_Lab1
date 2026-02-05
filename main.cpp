#include <SFML/Graphics.hpp>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <omp.h>
#include <sstream>
#include <string>
#include <vector>

#include "config.hpp"
#include "boid.hpp"

enum class UpdateMode {
    Sequential,
    Parallel
};

// Restituisce timestamp corrente in formato YYYYMMDD_HHMMSS per nomi di file
static std::string nowTimestampForFilename() {
    std::time_t t = std::time(nullptr);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y%m%d_%H%M%S");
    return oss.str();
}

static const char* modeToString(UpdateMode m) {
    return (m == UpdateMode::Sequential) ? "SEQ" : "PAR";
}

// Esegue benchmark headless e salva CSV (dati grezzi: una riga per run misurata)
static void runBenchmarks(sf::RenderWindow& window) {
    // Parametri base
    const std::vector<std::size_t> boidCounts = {1000, 5000, 10000};
    const std::size_t steps        = 1000;   // passi di simulazione per run
    const std::size_t warmupRuns   = 1;     // run di warm-up (non misurati)
    const std::size_t measuredRuns = 10;   // run misurati per configurazione. Ho messo 10 perchè altrimenti diventa troppo lungo

    // dt fisso per rendere più stabile la misura (evita dt variabile del frameClock)
    const float fixedDt = 1.0f / 60.0f;

    // OpenMP: evita variazioni dinamiche del numero di thread
    omp_set_dynamic(0);

    const std::string outName = std::string("benchmark_") + nowTimestampForFilename() + ".csv";
    std::ofstream out(outName);
    if (!out) {
        std::cerr << "[BENCH] ERRORE: impossibile aprire file output '" << outName << "'\n";
        return;
    }

    out << "mode,num_boids,steps,run_id,total_ms,ms_per_step,omp_max_threads\n";

    std::cout << "[BENCH] Avvio benchmark (rendering disattivato)\n";
    std::cout << "[BENCH] Output CSV: " << outName << "\n";
    std::cout << "[BENCH] steps=" << steps
              << " warmupRuns=" << warmupRuns
              << " measuredRuns=" << measuredRuns << "\n";
    std::cout << "[BENCH] omp_get_max_threads()=" << omp_get_max_threads() << "\n\n";

    auto pumpEvents = [&window]() {
        sf::Event ev{};
        while (window.pollEvent(ev)) {
            if (ev.type == sf::Event::Closed) window.close();
            if (ev.type == sf::Event::KeyPressed && ev.key.code == sf::Keyboard::Escape) window.close();
        }
    };

    std::size_t totalConfigs = boidCounts.size() * 2;
    std::size_t cfgIndex = 0;

    for (UpdateMode m : {UpdateMode::Sequential, UpdateMode::Parallel}) {
        for (std::size_t nBoids : boidCounts) {
            ++cfgIndex;
            if (!window.isOpen()) return;

            std::cout << "[BENCH] Config " << cfgIndex << "/" << totalConfigs
                      << "  mode=" << modeToString(m)
                      << "  N=" << nBoids << "\n";

            // Warm-up (non misurato)
            for (std::size_t w = 0; w < warmupRuns; ++w) {
                pumpEvents();
                if (!window.isOpen()) return;

                BoidSystem warmSys(nBoids);
                for (std::size_t s = 0; s < steps; ++s) {
                    if (m == UpdateMode::Sequential) warmSys.updateSequential(fixedDt);
                    else warmSys.updateParallel(fixedDt);
                }

                std::cout << "  - warmup " << (w + 1) << "/" << warmupRuns << " completato\n";
            }

            // Run misurati
            for (std::size_t r = 0; r < measuredRuns; ++r) {
                pumpEvents();
                if (!window.isOpen()) return;

                BoidSystem runSys(nBoids);

                auto t0 = std::chrono::high_resolution_clock::now();
                for (std::size_t s = 0; s < steps; ++s) {
                    if (m == UpdateMode::Sequential) runSys.updateSequential(fixedDt);
                    else runSys.updateParallel(fixedDt);
                }
                auto t1 = std::chrono::high_resolution_clock::now();

                double totalMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
                double msPerStep = totalMs / static_cast<double>(steps);

                out << modeToString(m) << ','
                    << nBoids << ','
                    << steps << ','
                    << r << ','
                    << std::fixed << std::setprecision(6) << totalMs << ','
                    << std::fixed << std::setprecision(9) << msPerStep << ','
                    << omp_get_max_threads() << "\n";

                if ((r + 1) % 2 == 0) { // log ogni tot run
                    std::cout << "  - run " << (r + 1) << "/" << measuredRuns
                              << "  total_ms=" << std::fixed << std::setprecision(3) << totalMs
                              << "  ms/step=" << std::fixed << std::setprecision(6) << msPerStep
                              << "\n";
                }
            }

            std::cout << "[BENCH] Config completata.\n\n";
        }
    }

    std::cout << "[BENCH] Benchmark completato. File: " << outName << "\n";
}

int main() {
    // --- Finestra SFML ---
    sf::RenderWindow window(
        sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT),
        "Boids Simulation - Perf Test"
    );
    window.setFramerateLimit(60);

    // --- Sistema di boids ---
    constexpr std::size_t NUM_BOIDS = 3000;
    BoidSystem boids(NUM_BOIDS);

    std::cout << "OpenMP diagnostics:\n";
    std::cout << "  omp_get_num_procs()      = " << omp_get_num_procs() << "\n";
    std::cout << "  omp_get_max_threads()    = " << omp_get_max_threads() << "\n";

    #pragma omp parallel
    {
        #pragma omp single
        {
            std::cout << "  threads_in_parallel_region = "
                      << omp_get_num_threads() << "\n";
        }
    }

    std::cout << std::endl;

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
              << "  T = avvia TEST automatici (benchmark headless + CSV)\n"
              << "  ESC / chiudi finestra = esci (anche durante il benchmark)\n\n";

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
                } else if (event.key.code == sf::Keyboard::T) {
                    runBenchmarks(window);

                    // Reset clock/statistiche per evitare dt anomali al rientro
                    frameClock.restart();
                    frameCount    = 0;
                    accumUpdateMs = 0.0;
                    minUpdateMs   = std::numeric_limits<double>::max();
                    maxUpdateMs   = 0.0;

                    std::cout << "[BENCH] Ritorno alla simulazione interattiva.\n";
                }
            }
        }

        if (!window.isOpen()) break;

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