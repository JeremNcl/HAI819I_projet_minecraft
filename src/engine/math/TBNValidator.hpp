#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/epsilon.hpp>
#include <cassert>
#include <cmath>
#include <iostream>

struct TBNValidator {
    static constexpr float EPSILON = 1e-3f;

    static bool isNormalized(const glm::vec3& v) {
        float len = glm::length(v);
        return glm::abs(len - 1.0f) < EPSILON;
    }

    static bool isOrthogonal(const glm::vec3& a, const glm::vec3& b) {
        float dot = glm::dot(a, b);
        return glm::abs(dot) < EPSILON;
    }

    static bool validateTBN(const glm::vec3& T, const glm::vec3& B, const glm::vec3& N) {
        bool valid = true;

        if (!isNormalized(T)) {
            std::cerr << "ERROR: Tangent not normalized (length=" << glm::length(T) << ")\n";
            valid = false;
        }
        if (!isNormalized(B)) {
            std::cerr << "ERROR: Bitangent not normalized (length=" << glm::length(B) << ")\n";
            valid = false;
        }
        if (!isNormalized(N)) {
            std::cerr << "ERROR: Normal not normalized (length=" << glm::length(N) << ")\n";
            valid = false;
        }

        if (!isOrthogonal(T, B)) {
            std::cerr << "ERROR: Tangent and Bitangent not orthogonal (dot=" << glm::dot(T, B) << ")\n";
            valid = false;
        }
        if (!isOrthogonal(T, N)) {
            std::cerr << "ERROR: Tangent and Normal not orthogonal (dot=" << glm::dot(T, N) << ")\n";
            valid = false;
        }
        if (!isOrthogonal(B, N)) {
            std::cerr << "ERROR: Bitangent and Normal not orthogonal (dot=" << glm::dot(B, N) << ")\n";
            valid = false;
        }

        glm::mat3 tbn(T, B, N);
        float det = glm::determinant(tbn);
        if (det < 0.5f) {
            std::cerr << "WARNING: TBN determinant < 0.5 (det=" << det << ") - might be left-handed\n";
            valid = false;
        }

        return valid;
    }
};
