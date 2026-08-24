#pragma once

#include <optional>
#include <vector>

namespace storm::solver {

/*!
 * Sound lower and upper bounds on a solution vector, as computed by e.g. interval iteration.
 * Both bounds have the same size as the solution. They are tracked independently because an algorithm may
 * well establish only one of them: optimistic value iteration, for instance, keeps a sound lower bound in
 * every iteration but only verifies its upper bound once it converges. An unset bound means that the
 * algorithm in question did not provide that side.
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

    /*!
     * Retrieves whether at least one of the two bounds is known.
     */
    bool hasAny() const {
        return hasLower() || hasUpper();
    }

    void clear() {
        lower = std::nullopt;
        upper = std::nullopt;
    }
};

}  // namespace storm::solver
