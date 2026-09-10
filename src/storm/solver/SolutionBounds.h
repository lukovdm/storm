#pragma once

#include <optional>
#include <vector>

namespace storm::solver {

/*!
 * Sound lower and upper bounds on a solution vector. Both have the same size as the solution. They are tracked
 * independently, as an algorithm may establish only one of them; an unset bound means that side is not known.
 */
template<typename ValueType>
struct SolutionBounds {
    std::optional<std::vector<ValueType>> lower;
    std::optional<std::vector<ValueType>> upper;

    bool hasLower() const {
        return lower.has_value();
    }

    bool hasUpper() const {
        return upper.has_value();
    }

    bool hasAny() const {
        return hasLower() || hasUpper();
    }

    void clear() {
        lower = std::nullopt;
        upper = std::nullopt;
    }
};

}  // namespace storm::solver
