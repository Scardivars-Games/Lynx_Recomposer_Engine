#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <random>
#include <string_view>
#include <unordered_set>
#include <vector>

#include <ilen/maths/AABB.h>
#include <ilen/physics/broad_phase_2d.h>

namespace {
    using Clock = std::chrono::steady_clock;
    using ilen::maths::aabb::AABB2D;
    using ilen::physics::BroadPhase2D;
    using ilen::physics::ColliderProxy2D;

    struct Scenario {
        std::string_view name;
        std::size_t dynamicCount;
        std::size_t staticCount;
        std::size_t iterations;
        float worldHalfExtent;
    };

    volatile std::uint64_t benchmarkSink = 0;

    std::uint64_t makePairKey(std::uint32_t entityA, std::uint32_t entityB) {
        const std::uint32_t minimum = std::min(entityA, entityB);
        const std::uint32_t maximum = std::max(entityA, entityB);
        return (static_cast<std::uint64_t>(minimum) << 32) |
            static_cast<std::uint64_t>(maximum);
    }

    ColliderProxy2D makeProxy(
        std::uint32_t entity,
        float x,
        float y,
        float halfExtent,
        bool isStatic
    ) {
        ColliderProxy2D proxy;
        proxy.entity = entity;
        proxy.bounds = AABB2D::fromCenterHalf(
            ilen::maths::vector::vec2{x, y},
            ilen::maths::vector::vec2{halfExtent, halfExtent}
        );
        proxy.isStatic = isStatic;
        return proxy;
    }

    void createScene(
        const Scenario& scenario,
        std::vector<ColliderProxy2D>& dynamicProxies,
        std::vector<ColliderProxy2D>& staticProxies
    ) {
        std::mt19937 generator(0x1EAFBEEFu);
        std::uniform_real_distribution<float> position(
            -scenario.worldHalfExtent,
            scenario.worldHalfExtent
        );
        std::uniform_real_distribution<float> halfExtent(0.25f, 0.85f);

        dynamicProxies.clear();
        staticProxies.clear();
        dynamicProxies.reserve(scenario.dynamicCount);
        staticProxies.reserve(scenario.staticCount);

        for(std::size_t index = 0; index < scenario.staticCount; ++index) {
            staticProxies.push_back(makeProxy(
                static_cast<std::uint32_t>(scenario.dynamicCount + index),
                position(generator),
                position(generator),
                halfExtent(generator),
                true
            ));
        }

        for(std::size_t index = 0; index < scenario.dynamicCount; ++index) {
            dynamicProxies.push_back(makeProxy(
                static_cast<std::uint32_t>(index),
                position(generator),
                position(generator),
                halfExtent(generator),
                false
            ));
        }

        // Garantiza algunos contactos conocidos sin convertir la escena en una masa densa.
        const std::size_t forcedContacts = std::min(
            dynamicProxies.size(),
            staticProxies.size()
        );
        for(std::size_t index = 0; index < forcedContacts; ++index) {
            if(index % 8 == 0) {
                dynamicProxies[index].bounds.center = staticProxies[index].bounds.center;
            }
        }
    }

    std::unordered_set<std::uint64_t> bruteForceContacts(
        const std::vector<ColliderProxy2D>& dynamicProxies,
        const std::vector<ColliderProxy2D>& staticProxies
    ) {
        std::unordered_set<std::uint64_t> contacts;

        for(std::size_t first = 0; first < dynamicProxies.size(); ++first) {
            for(std::size_t second = first + 1; second < dynamicProxies.size(); ++second) {
                if(ilen::maths::aabb::overlapAABB2D(
                    dynamicProxies[first].bounds,
                    dynamicProxies[second].bounds
                )) {
                    contacts.insert(makePairKey(
                        dynamicProxies[first].entity,
                        dynamicProxies[second].entity
                    ));
                }
            }

            for(const ColliderProxy2D& staticProxy : staticProxies) {
                if(ilen::maths::aabb::overlapAABB2D(
                    dynamicProxies[first].bounds,
                    staticProxy.bounds
                )) {
                    contacts.insert(makePairKey(
                        dynamicProxies[first].entity,
                        staticProxy.entity
                    ));
                }
            }
        }

        return contacts;
    }

    std::unordered_set<std::uint64_t> broadPhaseContacts(
        BroadPhase2D& broadPhase,
        std::vector<ColliderProxy2D>& dynamicProxies
    ) {
        std::unordered_set<std::uint64_t> contacts;

        for(const auto& pair : broadPhase.BuildCandidatesPairs(dynamicProxies)) {
            if(ilen::maths::aabb::overlapAABB2D(
                pair.proxyA->bounds,
                pair.proxyB->bounds
            )) {
                contacts.insert(makePairKey(
                    pair.proxyA->entity,
                    pair.proxyB->entity
                ));
            }
        }

        return contacts;
    }

    std::uint64_t countBruteForceContacts(
        const std::vector<ColliderProxy2D>& dynamicProxies,
        const std::vector<ColliderProxy2D>& staticProxies
    ) {
        std::uint64_t contacts = 0;

        for(std::size_t first = 0; first < dynamicProxies.size(); ++first) {
            for(std::size_t second = first + 1; second < dynamicProxies.size(); ++second) {
                contacts += ilen::maths::aabb::overlapAABB2D(
                    dynamicProxies[first].bounds,
                    dynamicProxies[second].bounds
                );
            }

            for(const ColliderProxy2D& staticProxy : staticProxies) {
                contacts += ilen::maths::aabb::overlapAABB2D(
                    dynamicProxies[first].bounds,
                    staticProxy.bounds
                );
            }
        }

        return contacts;
    }

    std::uint64_t countBroadPhaseContacts(
        BroadPhase2D& broadPhase,
        std::vector<ColliderProxy2D>& dynamicProxies,
        std::size_t& candidateCount
    ) {
        std::uint64_t contacts = 0;
        const auto& pairs = broadPhase.BuildCandidatesPairs(dynamicProxies);
        candidateCount = pairs.size();

        for(const auto& pair : pairs) {
            contacts += ilen::maths::aabb::overlapAABB2D(
                pair.proxyA->bounds,
                pair.proxyB->bounds
            );
        }

        return contacts;
    }

    template<typename Callback>
    double measureMedianMicrosecondsPerFrame(
        std::size_t iterations,
        Callback&& callback
    ) {
        constexpr std::size_t repetitions = 7;
        std::vector<double> samples;
        samples.reserve(repetitions);

        for(std::size_t repetition = 0; repetition < repetitions; ++repetition) {
            const auto begin = Clock::now();
            for(std::size_t iteration = 0; iteration < iterations; ++iteration) {
                benchmarkSink = benchmarkSink ^ callback(iteration);
            }
            const auto end = Clock::now();

            const double totalMicroseconds =
                std::chrono::duration<double, std::micro>(end - begin).count();
            samples.push_back(totalMicroseconds / static_cast<double>(iterations));
        }

        std::sort(samples.begin(), samples.end());
        return samples[samples.size() / 2];
    }

    bool runScenario(const Scenario& scenario) {
        std::vector<ColliderProxy2D> initialDynamicProxies;
        std::vector<ColliderProxy2D> staticProxies;
        createScene(scenario, initialDynamicProxies, staticProxies);

        BroadPhase2D correctnessBroadPhase(4.0f);
        correctnessBroadPhase.RebuildStaticGrid(staticProxies);

        auto correctnessDynamicProxies = initialDynamicProxies;
        const auto expectedContacts = bruteForceContacts(
            correctnessDynamicProxies,
            staticProxies
        );
        const auto actualContacts = broadPhaseContacts(
            correctnessBroadPhase,
            correctnessDynamicProxies
        );

        if(expectedContacts != actualContacts) {
            std::cerr << "[ERROR] " << scenario.name
                      << ": fuerza bruta encontro " << expectedContacts.size()
                      << " contactos y broadphase encontro " << actualContacts.size()
                      << ".\n";
            return false;
        }

        auto bruteDynamicProxies = initialDynamicProxies;
        const double bruteMicroseconds = measureMedianMicrosecondsPerFrame(
            scenario.iterations,
            [&](std::size_t iteration) {
                // Movimiento minimo para representar proxies dinamicos sin cambiar
                // sustancialmente la distribucion de la escena.
                bruteDynamicProxies.front().bounds.center.x +=
                    (iteration & 1U) == 0U ? 0.0001f : -0.0001f;
                return countBruteForceContacts(bruteDynamicProxies, staticProxies);
            }
        );

        auto broadDynamicProxies = initialDynamicProxies;
        BroadPhase2D measuredBroadPhase(4.0f);
        const auto rebuildBegin = Clock::now();
        measuredBroadPhase.RebuildStaticGrid(staticProxies);
        const auto rebuildEnd = Clock::now();
        const double rebuildMicroseconds =
            std::chrono::duration<double, std::micro>(rebuildEnd - rebuildBegin).count();

        std::size_t candidateCount = 0;
        const double broadMicroseconds = measureMedianMicrosecondsPerFrame(
            scenario.iterations,
            [&](std::size_t iteration) {
                broadDynamicProxies.front().bounds.center.x +=
                    (iteration & 1U) == 0U ? 0.0001f : -0.0001f;
                return countBroadPhaseContacts(
                    measuredBroadPhase,
                    broadDynamicProxies,
                    candidateCount
                );
            }
        );

        const std::uint64_t brutePairs =
            (scenario.dynamicCount * (scenario.dynamicCount - 1U)) / 2U +
            scenario.dynamicCount * scenario.staticCount;
        const double reduction = brutePairs == 0
            ? 0.0
            : 100.0 * (1.0 - static_cast<double>(candidateCount) /
                static_cast<double>(brutePairs));
        const double speedup = broadMicroseconds == 0.0
            ? 0.0
            : bruteMicroseconds / broadMicroseconds;

        std::cout << scenario.name << '\n'
                  << "  dinamicos=" << scenario.dynamicCount
                  << " estaticos=" << scenario.staticCount
                  << " pares_brutos=" << brutePairs << '\n'
                  << "  candidatos=" << candidateCount
                  << " reduccion=" << std::fixed << std::setprecision(2)
                  << reduction << "% contactos=" << expectedContacts.size() << '\n'
                  << "  fuerza_bruta=" << std::setprecision(3)
                  << bruteMicroseconds << " us/frame\n"
                  << "  broadphase=" << broadMicroseconds << " us/frame\n"
                  << "  aceleracion=" << std::setprecision(2) << speedup << "x\n"
                  << "  reconstruccion_estaticos=" << std::setprecision(3)
                  << rebuildMicroseconds << " us (no incluida por frame)\n"
                  << "  exactitud=OK\n\n";

        return true;
    }
}

int main() {
    const std::vector<Scenario> scenarios{
        {"Escena actual aproximada", 1, 199, 20000, 45.0f},
        {"Escena mediana dispersa", 64, 2048, 600, 180.0f},
        {"Escena grande dispersa", 256, 8192, 60, 360.0f}
    };

    std::cout << "Benchmark BroadPhase2D (mediana de 7 muestras)\n"
              << "La reconstruccion estatica se mide aparte porque los estaticos no cambian.\n\n";

    bool allCorrect = true;
    for(const Scenario& scenario : scenarios) {
        allCorrect = runScenario(scenario) && allCorrect;
    }

    if(benchmarkSink == 0xFFFFFFFFFFFFFFFFULL) {
        std::cerr << "sink=" << benchmarkSink << '\n';
    }

    return allCorrect ? 0 : 1;
}
