#pragma once

#include <memory>
#include <optional>

#include "storm/dd/DdType.h"
#include "storm/solver/OptimizationDirection.h"

namespace storm {
class Environment;

namespace dd {
template<storm::dd::DdType Type>
class Bdd;
}

namespace models::symbolic {
template<storm::dd::DdType Type, typename ValueType>
class NondeterministicModel;
}

namespace modelchecker {
class CheckResult;

/*!
 * Computes the optimal conditional probability Pmax/Pmin( F target | F condition ) for the (single) initial state of a symbolic MDP using the
 * bisection method. Mirrors the sparse implementation in ConditionalHelper but works on BDDs/ADDs throughout.
 *
 * @param threshold If given, the bisection stops as soon as the value is known to be above or below the threshold.
 */
template<storm::dd::DdType Type, typename ValueType>
std::unique_ptr<CheckResult> computeSymbolicConditionalProbabilities(Environment const& env, storm::solver::OptimizationDirection dir,
                                                                     storm::models::symbolic::NondeterministicModel<Type, ValueType> const& model,
                                                                     storm::dd::Bdd<Type> const& targetStates, storm::dd::Bdd<Type> const& conditionStates,
                                                                     std::optional<ValueType> const& threshold = std::nullopt);

}  // namespace modelchecker
}  // namespace storm
