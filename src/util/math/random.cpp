
#include "util/pch.h"

#include "random.h"

namespace AT::util {

    
    glm::vec3 random::get_vec3(f32 min, f32 max) { return glm::vec3(get<f32>(min, max), get<f32>(min, max), get<f32>(min, max)); }


    bool random::get_percent(f32 percentage) { return get<f32>(0.f, 1.f) < percentage; }


    std::string random::get_string(const size_t length) {

        const std::string charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        std::uniform_int_distribution<size_t> dist(0, charset.size() - 1);

        std::string result;
        result.resize(length);
        std::generate(result.begin(), result.end(), [&]() { return charset[dist(engine)]; });
        return result;
    }

}
