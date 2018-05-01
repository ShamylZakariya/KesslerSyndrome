/*
 *  PerlinNoise.cpp
 *  Terrain
 *
 *  Created by Shamyl Zakariya on 6/28/09.
 *  Copyright 2009 Shamyl Zakariya. All rights reserved.
 *
 */

#include "PerlinNoise.hpp"

namespace core {
    namespace util {

        namespace {

            const int permutation[] =
                    {
                            151, 160, 137, 91, 90, 15, 131, 13, 201, 95, 96, 53, 194, 233, 7,
                            225, 140, 36, 103, 30, 69, 142, 8, 99, 37, 240, 21, 10, 23, 190, 6,
                            148, 247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117, 35,
                            11, 32, 57, 177, 33, 88, 237, 149, 56, 87, 174, 20, 125, 136, 171,
                            168, 68, 175, 74, 165, 71, 134, 139, 48, 27, 166, 77, 146, 158,
                            231, 83, 111, 229, 122, 60, 211, 133, 230, 220, 105, 92, 41, 55,
                            46, 245, 40, 244, 102, 143, 54, 65, 25, 63, 161, 1, 216, 80, 73, 209,
                            76, 132, 187, 208, 89, 18, 169, 200, 196, 135, 130, 116, 188, 159,
                            86, 164, 100, 109, 198, 173, 186, 3, 64, 52, 217, 226, 250, 124, 123,
                            5, 202, 38, 147, 118, 126, 255, 82, 85, 212, 207, 206, 59, 227, 47,
                            16, 58, 17, 182, 189, 28, 42, 223, 183, 170, 213, 119, 248, 152, 2,
                            44, 154, 163, 70, 221, 153, 101, 155, 167, 43, 172, 9, 129, 22, 39,
                            253, 19, 98, 108, 110, 79, 113, 224, 232, 178, 185, 112, 104, 218,
                            246, 97, 228, 251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162,
                            241, 81, 51, 145, 235, 249, 14, 239, 107, 49, 192, 214, 31, 181, 199,
                            106, 157, 184, 84, 204, 176, 115, 121, 50, 45, 127, 4, 150, 254, 138,
                            236, 205, 93, 222, 114, 67, 29, 24, 72, 243, 141, 128, 195, 78, 66,
                            215, 61, 156, 180, 151, 160, 137, 91, 90, 15, 131, 13, 201, 95, 96,
                            53, 194, 233, 7, 225, 140, 36, 103, 30, 69, 142, 8, 99, 37, 240, 21,
                            10, 23, 190, 6, 148, 247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219,
                            203, 117, 35, 11, 32, 57, 177, 33, 88, 237, 149, 56, 87, 174, 20, 125,
                            136, 171, 168, 68, 175, 74, 165, 71, 134, 139, 48, 27, 166, 77, 146,
                            158, 231, 83, 111, 229, 122, 60, 211, 133, 230, 220, 105, 92, 41, 55,
                            46, 245, 40, 244, 102, 143, 54, 65, 25, 63, 161, 1, 216, 80, 73, 209,
                            76, 132, 187, 208, 89, 18, 169, 200, 196, 135, 130, 116, 188, 159,
                            86, 164, 100, 109, 198, 173, 186, 3, 64, 52, 217, 226, 250, 124, 123,
                            5, 202, 38, 147, 118, 126, 255, 82, 85, 212, 207, 206, 59, 227, 47, 16,
                            58, 17, 182, 189, 28, 42, 223, 183, 170, 213, 119, 248, 152, 2, 44, 154,
                            163, 70, 221, 153, 101, 155, 167, 43, 172, 9, 129, 22, 39, 253, 19, 98,
                            108, 110, 79, 113, 224, 232, 178, 185, 112, 104, 218, 246, 97, 228, 251,
                            34, 242, 193, 238, 210, 144, 12, 191, 179, 162, 241, 81, 51, 145, 235,
                            249, 14, 239, 107, 49, 192, 214, 31, 181, 199, 106, 157, 184, 84, 204,
                            176, 115, 121, 50, 45, 127, 4, 150, 254, 138, 236, 205, 93, 222, 114,
                            67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180
                    };

        }

        PerlinNoise::config PerlinNoise::config::parse(xml::XmlMultiTree perlinConfigNode) {
            config c;
            c.octaves = xml::readNumericAttribute<size_t>(perlinConfigNode, "octaves", c.octaves);
            c.falloff = xml::readNumericAttribute<double>(perlinConfigNode, "falloff", c.falloff);
            c.scale = xml::readNumericAttribute<double>(perlinConfigNode, "scale", c.scale);
            c.seed = xml::readNumericAttribute<int>(perlinConfigNode, "seed", c.seed);
            return c;
        }


        /*
         size_t _octaves;
         int _seed;
         rng _rng;
         double _persistence, _persistenceMax, _scale;
         double *_octFrequency, *_octPersistence;
         const int *_p;
         int _offsetX, _offsetY, _offsetZ;
         bool _initialized;
         */

        PerlinNoise::PerlinNoise(size_t octaves, double falloff, double scale, int seed) :
                _octaves(octaves),
                _seed(seed),
                _persistence(saturate(falloff)),
                _scale(scale),
                _octFrequency(nullptr),
                _octPersistence(nullptr),
                _p(permutation),
                _initialized(false) {
            setSeed(_seed);
        }

        PerlinNoise::PerlinNoise(const config &c) :
                _octaves(c.octaves),
                _seed(c.seed),
                _persistence(saturate(c.falloff)),
                _scale(c.scale),
                _octFrequency(nullptr),
                _octPersistence(nullptr),
                _p(permutation),
                _initialized(false) {
            setSeed(_seed);
        }

        PerlinNoise::PerlinNoise(const PerlinNoise &copy) :
                _octaves(copy._octaves),
                _seed(copy._seed),
                _rng(copy._rng),
                _persistence(copy._persistence),
                _persistenceMax(copy._persistenceMax),
                _scale(copy._scale),
                _octFrequency(nullptr), // this will be initted as normal
                _octPersistence(nullptr), // ditto
                _p(permutation),
                _initialized(false) {
            setSeed(_seed);
        }

        PerlinNoise &
        PerlinNoise::operator=(const PerlinNoise &copy) {
            _octaves = copy._octaves;
            _seed = copy._seed;
            _rng = copy._rng;
            _persistence = copy._persistence;
            _persistenceMax = copy._persistenceMax;
            _scale = copy._scale;
            _initialized = false;

            delete[] _octFrequency;
            delete[] _octPersistence;
            _octFrequency = _octPersistence = nullptr;

            setSeed(_seed);

            return *this;
        }

#pragma mark -

        Channel32f fill(PerlinNoise &pn, const ivec2 &size, const ivec2 &offset, bool normalized) {
            Channel32f channel(size.x, size.y);
            fill(channel, pn, offset, normalized);
            return channel;
        }

        void fill(Channel32f &channel, PerlinNoise &pn, const ivec2 &offset, bool normalized) {
            Channel32f::Iter iterator = channel.getIter();

            const double
                    width = channel.getWidth(),
                    height = channel.getHeight(),
                    ox = offset.x,
                    oy = offset.y;

            while (iterator.line()) {
                while (iterator.pixel()) {
                    double
                            xf = ox + double(iterator.x()) / width,
                            yf = oy + double(iterator.y()) / height;

                    float v = float(normalized ? pn.noise(xf, yf, 1) : pn.noiseUnit(xf, yf, 1));
                    iterator.v() = v;
                }
            }

        }

        void fill(std::vector<double> &data, PerlinNoise &pn, const int offset, bool normalized) {
            double
                    x = 0,
                    width = data.size(),
                    xInc = 1 / width;

            for (std::vector<double>::iterator v(data.begin()), end(data.end()); v != end; ++v, x += xInc) {
                *v = normalized ? pn.noise(x) : pn.noiseUnit(x);
            }
        }


    }
} // end namespace core::util
