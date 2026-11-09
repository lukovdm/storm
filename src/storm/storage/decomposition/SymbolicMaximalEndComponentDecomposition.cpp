#include "storm/storage/decomposition/SymbolicMaximalEndComponentDecomposition.h"

#include <algorithm>
#include <array>
#include <stack>

#include "storm/adapters/RationalFunctionAdapter.h"
#include "storm/adapters/RationalNumberAdapter.h"
#include "storm/dd/DdManager.h"
#include "storm/models/symbolic/NondeterministicModel.h"
#include "storm/utility/macros.h"

namespace storm::storage {

namespace {

using VariableSet = std::set<storm::expressions::Variable>;
using VariablePairs = std::vector<std::pair<storm::expressions::Variable, storm::expressions::Variable>>;

/*!
 * Bundles the (fixed) variable sets so that the helper functions below have short signatures.
 */
struct Variables {
    VariableSet const& row;
    VariableSet const& column;
    VariableSet const& nondeterminism;
    VariablePairs const& rowColumnPairs;
};

/// Picks a single state out of the given (non-empty) set.
template<storm::dd::DdType Type>
storm::dd::Bdd<Type> pick(storm::dd::Bdd<Type> const& states, Variables const& vars) {
    STORM_LOG_ASSERT(!states.isZero(), "Cannot pick a state from the empty set.");
    return states.existsAbstractRepresentative(vars.row);
}

/// The successors of the given states within the given set.
template<storm::dd::DdType Type>
storm::dd::Bdd<Type> post(storm::dd::Bdd<Type> const& states, storm::dd::Bdd<Type> const& within, storm::dd::Bdd<Type> const& stateTransitions,
                          Variables const& vars) {
    return within && states.relationalProduct(stateTransitions, vars.row, vars.column);
}

/// The predecessors of the given states within the given set.
template<storm::dd::DdType Type>
storm::dd::Bdd<Type> pre(storm::dd::Bdd<Type> const& states, storm::dd::Bdd<Type> const& within, storm::dd::Bdd<Type> const& stateTransitions,
                         Variables const& vars) {
    return within && states.inverseRelationalProduct(stateTransitions, vars.row, vars.column);
}

/// Whether the given SCC is a single state without a self loop (and thus not an end component).
template<storm::dd::DdType Type>
bool isTrivialSccWithoutSelfLoop(storm::dd::Bdd<Type> const& scc, storm::dd::Bdd<Type> const& transitions, Variables const& vars) {
    if (scc.getNonZeroCount() != 1) {
        return false;
    }
    return (transitions && scc && scc.swapVariables(vars.rowColumnPairs)).isZero();
}

/// All choices (over row and nondeterminism variables) of the given states that leave the given states with positive probability.
template<storm::dd::DdType Type>
storm::dd::Bdd<Type> leavingChoices(storm::dd::Bdd<Type> const& states, storm::dd::Bdd<Type> const& transitions, Variables const& vars) {
    return (states && transitions && !states.swapVariables(vars.rowColumnPairs)).existsAbstract(vars.column);
}

template<storm::dd::DdType Type>
struct Attractor {
    storm::dd::Bdd<Type> states;
    storm::dd::Bdd<Type> choices;
};

/*!
 * Computes the random attractor of the given choices within the given states: the smallest set of states and choices such that
 * (1) the given choices are included, (2) a state is included if all its choices are included and
 * (3) a choice is included if it reaches an included state with positive probability.
 */
template<storm::dd::DdType Type>
Attractor<Type> randomAttractor(storm::dd::Bdd<Type> const& choices, storm::dd::Bdd<Type> const& states, storm::dd::Bdd<Type> const& transitions,
                                Variables const& vars) {
    storm::dd::Bdd<Type> const allChoices = transitions.existsAbstract(vars.column);
    Attractor<Type> next{states.getDdManager().getBddZero(), choices};
    Attractor<Type> current;
    do {
        current = next;
        // States all of whose choices are in the attractor.
        storm::dd::Bdd<Type> const choicesOutside = allChoices && !current.choices;
        storm::dd::Bdd<Type> const newStates = current.choices.existsAbstract(vars.nondeterminism) && !choicesOutside.existsAbstract(vars.nondeterminism);
        next.states = (current.states || newStates) && states;
        // Choices of the given states that can reach an attractor state.
        storm::dd::Bdd<Type> const choicesIntoAttractor = (states && transitions && next.states.swapVariables(vars.rowColumnPairs)).existsAbstract(vars.column);
        next.choices = current.choices || choicesIntoAttractor;
    } while (current.states != next.states);
    return current;
}

/*!
 * Computes the SCCs of the given state graph within the given states.
 * Follows Gentilini, Piazza and Policriti, "Computing strongly connected components in a linear number of symbolic steps" (SODA 2003),
 * using an explicit work stack instead of recursion.
 */
template<storm::dd::DdType Type>
std::vector<storm::dd::Bdd<Type>> sccDecomposition(storm::dd::Bdd<Type> const& allStates, storm::dd::Bdd<Type> const& stateTransitions, Variables const& vars) {
    struct Task {
        storm::dd::Bdd<Type> states;
        storm::dd::Bdd<Type> skeleton;
        storm::dd::Bdd<Type> node;
    };

    std::vector<storm::dd::Bdd<Type>> result;
    if (allStates.isZero()) {
        return result;
    }
    auto const zero = allStates.getDdManager().getBddZero();
    std::stack<Task> tasks;
    tasks.push({allStates, zero, zero});
    while (!tasks.empty()) {
        Task task = tasks.top();
        tasks.pop();
        if (task.skeleton.isZero()) {
            task.node = pick(task.states, vars);
        }
        STORM_LOG_ASSERT(!task.node.isZero(), "SCC search has no start node.");

        // Forward set of the node, together with a skeleton (a path through the onion rings).
        storm::dd::Bdd<Type> forward = zero;
        std::stack<storm::dd::Bdd<Type>> rings;
        for (storm::dd::Bdd<Type> level = task.node; !level.isZero(); level = !forward && post(level, task.states, stateTransitions, vars)) {
            rings.push(level);
            forward |= level;
        }
        storm::dd::Bdd<Type> const newNode = pick(rings.top(), vars);
        storm::dd::Bdd<Type> newSkeleton = newNode;
        rings.pop();
        while (!rings.empty()) {
            newSkeleton |= pick(rings.top() && pre(newSkeleton, task.states, stateTransitions, vars), vars);
            rings.pop();
        }

        // The SCC of the node: states in the forward set that can reach it.
        storm::dd::Bdd<Type> scc = task.node;
        for (storm::dd::Bdd<Type> added = scc; !added.isZero();) {
            storm::dd::Bdd<Type> const predecessors = pre(added, forward, stateTransitions, vars);
            added = predecessors && !scc;
            scc |= added;
        }
        result.push_back(scc);

        // Remaining states outside the forward set.
        storm::dd::Bdd<Type> const remainingSkeleton = task.skeleton && !scc;
        Task outside{task.states && !forward, remainingSkeleton, remainingSkeleton && pre(scc && task.skeleton, task.states, stateTransitions, vars)};
        if (!outside.states.isZero()) {
            tasks.push(std::move(outside));
        }
        // Remaining states inside the forward set.
        Task inside{forward && !scc, newSkeleton && !scc, newNode && !scc};
        if (!inside.states.isZero()) {
            tasks.push(std::move(inside));
        }
    }
    return result;
}

template<storm::dd::DdType Type>
std::vector<storm::dd::Bdd<Type>> mecDecompositionNaive(storm::dd::Bdd<Type> const& allStates, storm::dd::Bdd<Type> transitions, Variables const& vars) {
    std::vector<storm::dd::Bdd<Type>> result;
    std::stack<storm::dd::Bdd<Type>> candidates;
    for (auto& scc : sccDecomposition(allStates, transitions.existsAbstract(vars.nondeterminism), vars)) {
        candidates.push(std::move(scc));
    }
    while (!candidates.empty()) {
        storm::dd::Bdd<Type> scc = candidates.top();
        candidates.pop();
        if (isTrivialSccWithoutSelfLoop(scc, transitions, vars)) {
            continue;
        }
        storm::dd::Bdd<Type> const leaving = leavingChoices(scc, transitions, vars);
        if (leaving.isZero()) {
            result.push_back(scc);
        } else {
            auto const attractor = randomAttractor(leaving, scc, transitions, vars);
            transitions &= !attractor.choices;
            for (auto& subScc : sccDecomposition(scc && !attractor.states, transitions.existsAbstract(vars.nondeterminism), vars)) {
                candidates.push(std::move(subScc));
            }
        }
    }
    return result;
}

/*!
 * The INTERLEAVE algorithm of Bansal and Singh, "Symbolic Algorithms for Computing Maximal End Components" (CAV 2025, arXiv:2505.20748).
 * Ported from the implementation accompanying Ramneet Singh's master's thesis.
 *
 * Each task holds a set of states V that is closed under the remaining choices (no remaining choice leaves V) and optionally a start state.
 * From the start state we compute the forward set FW and the SCC of the start state, which splits V into
 * V1 = SCC, V2 = FW \ SCC and V3 = V \ FW. Choices leaving V1 or V3 are removed together with their random attractor. V2 needs no pruning:
 * its states cannot reach V1, so all their successors in V lie in V2. A state in the last layer of FW is a good start state for V2.
 */
template<storm::dd::DdType Type>
std::vector<storm::dd::Bdd<Type>> mecDecompositionInterleave(storm::dd::Bdd<Type> const& allStates, storm::dd::Bdd<Type> transitions, Variables const& vars) {
    struct Task {
        storm::dd::Bdd<Type> states;
        storm::dd::Bdd<Type> startState;
    };

    std::vector<storm::dd::Bdd<Type>> result;
    if (allStates.isZero()) {
        return result;
    }
    auto const zero = allStates.getDdManager().getBddZero();
    std::stack<Task> tasks;
    tasks.push({allStates, zero});
    while (!tasks.empty()) {
        Task task = tasks.top();
        tasks.pop();
        if (task.startState.isZero()) {
            task.startState = pick(task.states, vars);
        }

        // Forward set and SCC of the start state.
        storm::dd::Bdd<Type> const stateTransitions = transitions.existsAbstract(vars.nondeterminism);
        storm::dd::Bdd<Type> forward = zero;
        storm::dd::Bdd<Type> lastLevel = zero;
        for (storm::dd::Bdd<Type> level = task.startState; !level.isZero(); level = post(level, task.states && !forward, stateTransitions, vars)) {
            forward |= level;
            lastLevel = level;
        }
        storm::dd::Bdd<Type> const newStartState = pick(lastLevel, vars);
        storm::dd::Bdd<Type> v1 = zero;
        for (storm::dd::Bdd<Type> level = task.startState; !level.isZero(); level = pre(level, task.states && forward && !v1, stateTransitions, vars)) {
            v1 |= level;
        }
        storm::dd::Bdd<Type> const v2 = forward && !v1;
        storm::dd::Bdd<Type> v3 = task.states && !forward;

        // V1 is an MEC if no choice leaves it (unless it is a single state without self loop). Otherwise, prune.
        storm::dd::Bdd<Type> const leavingV1 = leavingChoices(v1, transitions, vars);
        if (leavingV1.isZero()) {
            if (!isTrivialSccWithoutSelfLoop(v1, transitions, vars)) {
                result.push_back(v1);
            }
            v1 = zero;
        } else {
            auto const attractor = randomAttractor(leavingV1, v1, transitions, vars);
            transitions &= !attractor.choices;
            v1 &= !attractor.states;
        }
        // V3 may have choices into FW: prune them.
        storm::dd::Bdd<Type> const leavingV3 = leavingChoices(v3, transitions, vars);
        if (!leavingV3.isZero()) {
            auto const attractor = randomAttractor(leavingV3, v3, transitions, vars);
            transitions &= !attractor.choices;
            v3 &= !attractor.states;
        }

        // Push the non-empty subproblems such that the smallest one is processed first.
        std::array<std::pair<uint64_t, Task>, 3> subTasks{
            {{v1.getNonZeroCount(), Task{v1, zero}}, {v2.getNonZeroCount(), Task{v2, newStartState && v2}}, {v3.getNonZeroCount(), Task{v3, zero}}}};
        std::stable_sort(subTasks.begin(), subTasks.end(), [](auto const& a, auto const& b) { return a.first > b.first; });
        for (auto& [size, subTask] : subTasks) {
            if (size > 0) {
                tasks.push(std::move(subTask));
            }
        }
    }
    return result;
}

}  // namespace

template<storm::dd::DdType Type>
SymbolicMaximalEndComponentDecomposition<Type>::SymbolicMaximalEndComponentDecomposition(
    storm::dd::Bdd<Type> const& states, storm::dd::Bdd<Type> const& transitions, std::set<storm::expressions::Variable> const& rowVariables,
    std::set<storm::expressions::Variable> const& columnVariables, std::set<storm::expressions::Variable> const& nondeterminismVariables,
    std::vector<std::pair<storm::expressions::Variable, storm::expressions::Variable>> const& rowColumnPairs, SymbolicMecAlgorithm algorithm)
    : transitions(transitions && states), columnVariables(columnVariables), rowColumnPairs(rowColumnPairs) {
    Variables const vars{rowVariables, columnVariables, nondeterminismVariables, rowColumnPairs};
    switch (algorithm) {
        case SymbolicMecAlgorithm::Interleave:
            mecStates = mecDecompositionInterleave(states, this->transitions, vars);
            break;
        case SymbolicMecAlgorithm::Naive:
            mecStates = mecDecompositionNaive(states, this->transitions, vars);
            break;
    }
}

template<storm::dd::DdType Type>
template<typename ValueType>
SymbolicMaximalEndComponentDecomposition<Type>::SymbolicMaximalEndComponentDecomposition(
    storm::models::symbolic::NondeterministicModel<Type, ValueType> const& model, SymbolicMecAlgorithm algorithm)
    : SymbolicMaximalEndComponentDecomposition(model, model.getReachableStates(), algorithm) {
    // Intentionally left empty.
}

template<storm::dd::DdType Type>
template<typename ValueType>
SymbolicMaximalEndComponentDecomposition<Type>::SymbolicMaximalEndComponentDecomposition(
    storm::models::symbolic::NondeterministicModel<Type, ValueType> const& model, storm::dd::Bdd<Type> const& states, SymbolicMecAlgorithm algorithm)
    : SymbolicMaximalEndComponentDecomposition(states, model.getTransitionMatrix().notZero(), model.getRowVariables(), model.getColumnVariables(),
                                               model.getNondeterminismVariables(), model.getRowColumnMetaVariablePairs(), algorithm) {
    // Intentionally left empty.
}

template<storm::dd::DdType Type>
uint64_t SymbolicMaximalEndComponentDecomposition<Type>::size() const {
    return mecStates.size();
}

template<storm::dd::DdType Type>
bool SymbolicMaximalEndComponentDecomposition<Type>::empty() const {
    return mecStates.empty();
}

template<storm::dd::DdType Type>
std::vector<storm::dd::Bdd<Type>> const& SymbolicMaximalEndComponentDecomposition<Type>::getMecStates() const {
    return mecStates;
}

template<storm::dd::DdType Type>
storm::dd::Bdd<Type> const& SymbolicMaximalEndComponentDecomposition<Type>::getMecStates(uint64_t index) const {
    return mecStates.at(index);
}

template<storm::dd::DdType Type>
storm::dd::Bdd<Type> SymbolicMaximalEndComponentDecomposition<Type>::getAllMecStates() const {
    storm::dd::Bdd<Type> result = transitions.getDdManager().getBddZero();
    for (auto const& mec : mecStates) {
        result |= mec;
    }
    return result;
}

template<storm::dd::DdType Type>
storm::dd::Bdd<Type> SymbolicMaximalEndComponentDecomposition<Type>::getMecChoices(uint64_t index) const {
    storm::dd::Bdd<Type> const& mec = mecStates.at(index);
    storm::dd::Bdd<Type> const mecTransitions = transitions && mec;
    return mecTransitions.existsAbstract(columnVariables) && !(mecTransitions && !mec.swapVariables(rowColumnPairs)).existsAbstract(columnVariables);
}

template<storm::dd::DdType Type>
storm::dd::Bdd<Type> SymbolicMaximalEndComponentDecomposition<Type>::getAllMecChoices() const {
    storm::dd::Bdd<Type> result = transitions.getDdManager().getBddZero();
    for (uint64_t index = 0; index < mecStates.size(); ++index) {
        result |= getMecChoices(index);
    }
    return result;
}

template class SymbolicMaximalEndComponentDecomposition<storm::dd::DdType::CUDD>;
template class SymbolicMaximalEndComponentDecomposition<storm::dd::DdType::Sylvan>;

template SymbolicMaximalEndComponentDecomposition<storm::dd::DdType::CUDD>::SymbolicMaximalEndComponentDecomposition(
    storm::models::symbolic::NondeterministicModel<storm::dd::DdType::CUDD, double> const&, SymbolicMecAlgorithm);
template SymbolicMaximalEndComponentDecomposition<storm::dd::DdType::CUDD>::SymbolicMaximalEndComponentDecomposition(
    storm::models::symbolic::NondeterministicModel<storm::dd::DdType::CUDD, double> const&, storm::dd::Bdd<storm::dd::DdType::CUDD> const&,
    SymbolicMecAlgorithm);
template SymbolicMaximalEndComponentDecomposition<storm::dd::DdType::Sylvan>::SymbolicMaximalEndComponentDecomposition(
    storm::models::symbolic::NondeterministicModel<storm::dd::DdType::Sylvan, double> const&, SymbolicMecAlgorithm);
template SymbolicMaximalEndComponentDecomposition<storm::dd::DdType::Sylvan>::SymbolicMaximalEndComponentDecomposition(
    storm::models::symbolic::NondeterministicModel<storm::dd::DdType::Sylvan, double> const&, storm::dd::Bdd<storm::dd::DdType::Sylvan> const&,
    SymbolicMecAlgorithm);
template SymbolicMaximalEndComponentDecomposition<storm::dd::DdType::Sylvan>::SymbolicMaximalEndComponentDecomposition(
    storm::models::symbolic::NondeterministicModel<storm::dd::DdType::Sylvan, storm::RationalNumber> const&, SymbolicMecAlgorithm);
template SymbolicMaximalEndComponentDecomposition<storm::dd::DdType::Sylvan>::SymbolicMaximalEndComponentDecomposition(
    storm::models::symbolic::NondeterministicModel<storm::dd::DdType::Sylvan, storm::RationalNumber> const&, storm::dd::Bdd<storm::dd::DdType::Sylvan> const&,
    SymbolicMecAlgorithm);
template SymbolicMaximalEndComponentDecomposition<storm::dd::DdType::Sylvan>::SymbolicMaximalEndComponentDecomposition(
    storm::models::symbolic::NondeterministicModel<storm::dd::DdType::Sylvan, storm::RationalFunction> const&, SymbolicMecAlgorithm);
template SymbolicMaximalEndComponentDecomposition<storm::dd::DdType::Sylvan>::SymbolicMaximalEndComponentDecomposition(
    storm::models::symbolic::NondeterministicModel<storm::dd::DdType::Sylvan, storm::RationalFunction> const&, storm::dd::Bdd<storm::dd::DdType::Sylvan> const&,
    SymbolicMecAlgorithm);

}  // namespace storm::storage
