//
//  CloudLayerParticleSystem.hpp
//  Kessler Syndrome
//
//  Created by Shamyl Zakariya on 10/18/17.
//

#ifndef CloudLayerParticleSystem_hpp
#define CloudLayerParticleSystem_hpp

#include <cinder/Perlin.h>

#include "elements/ParticleSystem/ParticleSystem.hpp"
#include "game/KesslerSyndrome/GameConstants.hpp"

namespace game {

    SMART_PTR(CloudLayerParticleSimulation);

    SMART_PTR(CloudLayerParticleSystemDrawComponent);

    SMART_PTR(CloudLayerParticleSystem);

    class CloudLayerParticleSimulation : public elements::BaseParticleSimulation {
    public:

        struct particle_config {
            double minRadius;
            double maxRadius;
            double minRadiusNoiseValue;
            ColorA color;

            particle_config() :
                    minRadius(0),
                    maxRadius(100),
                    minRadiusNoiseValue(0.5),
                    color(1, 1, 1, 1) {
            }

            static particle_config parse(const XmlTree &node);

        };
        
        struct noise_config {
            int octaves;
            int seed;
            
            noise_config():
                    octaves(4),
                    seed(12345)
            {}
            
            static noise_config parse(const XmlTree &node);
        };


        struct config {
            noise_config noise;
            particle_config particle;
            dvec2 origin;
            double radius;
            // the lower the period, the more rapidly cloud formations change
            core::seconds_t period;
            // the lower the turbulence, the smoother the cloud chape, the higher, the rougher
            double turbulence;
            size_t count;
            double displacementForce;
            double returnForce;

            config() :
                    origin(0, 0),
                    radius(500),
                    period(1),
                    turbulence(4),
                    count(100),
                    displacementForce(10),
                    returnForce(32) {
            }

            static config parse(const XmlTree &node);
        };


    public:

        CloudLayerParticleSimulation(const config &c);

        void onReady(core::ObjectRef parent, core::StageRef stage) override;

        void update(const core::time_state &timeState) override;

        void setParticleCount(size_t count) override;

        size_t getFirstActive() const override {
            return 0;
        };

        size_t getActiveCount() const override {
            return _state.size();
        }

        cpBB getBB() const override {
            return _bb;
        }

        //CloudLayerParticleSimulation

        // adds a gravity to take into account when simulation cloud position. the gravity
        // will be automatically discarded when it is finished
        void addGravityDisplacement(const core::RadialGravitationCalculatorRef &gravity);

    protected:

        virtual void simulate(const core::time_state &timeState);

        void pruneDisplacements();

        void applyGravityDisplacements(const core::time_state &timeState);

    private:

        struct particle_physics {
            dvec2 home;
            dvec2 position;
            dvec2 previous_position;
            dvec2 velocity;
            double radius;
            double damping;
        };

        config _config;
        ci::Perlin _noise;
        core::seconds_t _time;
        cpBB _bb;
        vector<core::RadialGravitationCalculatorRef> _displacements;
        vector<particle_physics> _physics;
    };

    class CloudLayerParticleSystem : public elements::BaseParticleSystem {
    public:

        struct config {
            elements::ParticleSystemDrawComponent::config drawConfig;
            CloudLayerParticleSimulation::config simulationConfig;

            static config parse(const XmlTree &node);
        };

        static CloudLayerParticleSystemRef create(const config &c);

    public:

        CloudLayerParticleSystem(std::string name);

    };


}

#endif /* CloudLayerParticleSystem_hpp */
