#pragma once

#include <set>
#include <utility>
#include <vector>

#include "storm/dd/Bdd.h"
#include "storm/dd/DdType.h"
#include "storm/expressions/Variable.h"

namespace storm {
namespace models::symbolic {
template<storm::dd::DdType Type, typename ValueType>
class NondeterministicModel;
}

namespace storage {

/*!
 * The algorithm used to compute a symbolic MEC decomposition.
 */
enum class SymbolicMecAlgorithm {
    /// The INTERLEAVE algorithm by Bansal and Singh (CAV 2025), which interleaves SCC search and attractor pruning. Usually the fastest.
    Interleave,
    /// The basic algorithm of Chatterjee et al. (arXiv:1804.00206): repeatedly decompose into SCCs and remove random attractors of leaving actions.
    Naive
};

/*!
 * Computes the maximal end components (MECs) of a symbolically represented MDP.
 *
 * Each MEC is given by its set of states (as a BDD over the row variables). The choices of a MEC are exactly those choices of its states whose
 * successors all lie in the MEC; they can be obtained via getMecChoices.
 */
template<storm::dd::DdType Type>
class SymbolicMaximalEndComponentDecomposition {
   public:
    /*!
     * Computes the MECs within the given set of states.
     *
     * @param states The states to consider. Choices that leave this set (with positive probability) are never part of an end component.
     * @param transitions The transition relation (row, nondeterminism and column variables) as a BDD.
     * @param rowVariables The row (source state) variables.
     * @param columnVariables The column (successor state) variables.
     * @param nondeterminismVariables The variables encoding the nondeterministic choices.
     * @param rowColumnPairs The pairs of row and column variables.
     * @param algorithm The algorithm to use.
     */
    SymbolicMaximalEndComponentDecomposition(storm::dd::Bdd<Type> const& states, storm::dd::Bdd<Type> const& transitions,
                                             std::set<storm::expressions::Variable> const& rowVariables,
                                             std::set<storm::expressions::Variable> const& columnVariables,
                                             std::set<storm::expressions::Variable> const& nondeterminismVariables,
                                             std::vector<std::pair<storm::expressions::Variable, storm::expressions::Variable>> const& rowColumnPairs,
                                             SymbolicMecAlgorithm algorithm = SymbolicMecAlgorithm::Interleave);

    /*!
     * Computes the MECs of the given model, restricted to the given states (all reachable states by default).
     */
    template<typename ValueType>
    SymbolicMaximalEndComponentDecomposition(storm::models::symbolic::NondeterministicModel<Type, ValueType> const& model,
                                             SymbolicMecAlgorithm algorithm = SymbolicMecAlgorithm::Interleave);
    template<typename ValueType>
    SymbolicMaximalEndComponentDecomposition(storm::models::symbolic::NondeterministicModel<Type, ValueType> const& model, storm::dd::Bdd<Type> const& states,
                                             SymbolicMecAlgorithm algorithm = SymbolicMecAlgorithm::Interleave);

    /// The number of MECs.
    uint64_t size() const;
    bool empty() const;

    /// The states of each MEC. The sets are pairwise disjoint.
    std::vector<storm::dd::Bdd<Type>> const& getMecStates() const;
    storm::dd::Bdd<Type> const& getMecStates(uint64_t index) const;

    /// The union of all MEC states.
    storm::dd::Bdd<Type> getAllMecStates() const;

    /// The choices (over row and nondeterminism variables) of the given MEC, i.e., the choices of MEC states that stay inside the MEC.
    storm::dd::Bdd<Type> getMecChoices(uint64_t index) const;

    /// The union of the choices of all MECs.
    storm::dd::Bdd<Type> getAllMecChoices() const;

   private:
    std::vector<storm::dd::Bdd<Type>> mecStates;
    storm::dd::Bdd<Type> transitions;
    std::set<storm::expressions::Variable> columnVariables;
    std::vector<std::pair<storm::expressions::Variable, storm::expressions::Variable>> rowColumnPairs;
};

}  // namespace storage
}  // namespace storm
