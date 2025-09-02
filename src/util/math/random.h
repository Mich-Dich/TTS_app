#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/random.hpp>

namespace AT::util {
    
    // @brief A class that provides methods for generating random numbers of various types.
    //        It utilizes the Mersenne Twister pseudo-random number generator for high-quality randomness.
    //        The class can generate floating-point numbers and unsigned integers within specified ranges.
    class random {
    public:

        // Constructs a random number generator and seeds its internal Mersenne Twister engine.
        // @param seed The seed value used to initialize the RNG engine. By default this is supplied by std::random_device.
        // @return None.
        random(u32 seed = std::random_device{}())
            : engine(seed) {}

            
        // Generates a random value of type T within the inclusive range [min, max].
        // If T is a floating-point type, a std::uniform_real_distribution<T> is used.
        // If T is an integral type, a std::uniform_int_distribution<T> is used.
        // @tparam T The numeric type to generate (floating-point or integral).
        // @param min The lower bound of the range.
        // @param max The upper bound of the range.
        // @return A randomly generated value of type T between min and max.
        template<typename T>
        T get(T min, T max) {

            if constexpr (std::is_floating_point_v<T>) {

                std::uniform_real_distribution<T> dist(min, max);
                return dist(engine);

            } else if constexpr (std::is_integral_v<T>) {

                std::uniform_int_distribution<T> dist(min, max);
                return dist(engine);
            }
        }

        // Generates a glm::vec3 where each component is a random float in [min, max].
        // Internally uses get<f32>(min, max) to produce each component.
        // @param min The minimum value for each component (default: -1.0f).
        // @param max The maximum value for each component (default:  1.0f).
        // @return A glm::vec3 with randomly generated components.
        glm::vec3 get_vec3(f32 min = -1.0f, f32 max = 1.0f);

        // Generates a random floating-point number in the range [0.f, 1.f] and checks if it's less than the percentage.
        // @param percentage The probability threshold (should be between 0.f and 1.f). Defaults to 0.0f.
        // @return True if the generated random float is less than percentage; false otherwise.
        bool get_percent(f32 percentage = 0.0f);

        // Generates a random alphanumeric string of the requested length using a predefined charset.
        // @param length The number of characters to generate.
        // @return A std::string containing randomly selected characters from the charset.
        std::string get_string(const size_t length);

    private:
        std::mt19937                            engine; // Mersenne Twister pseudo-random generator
    };

}
