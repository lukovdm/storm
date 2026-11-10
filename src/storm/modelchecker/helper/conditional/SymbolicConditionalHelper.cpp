#include "storm/modelchecker/helper/conditional/SymbolicConditionalHelper.h"

#include <set>
#include <string>

#include "storm/adapters/RationalNumberAdapter.h"
#include "storm/dd/Add.h"
#include "storm/dd/Bdd.h"
#include "storm/dd/DdManager.h"
#include "storm/environment/Environment.h"
#include "storm/environment/modelchecker/ConditionalModelCheckerEnvironment.h"
#include "storm/environment/modelchecker/ModelCheckerEnvironment.h"
#include "storm/environment/solver/MinMaxSolverEnvironment.h"
#include "storm/environment/solver/SolverEnvironment.h"
#include "storm/exceptions/InvalidPropertyException.h"
#include "storm/exceptions/NotSupportedException.h"
#include "storm/exceptions/UncheckedRequirementException.h"
#include "storm/modelchecker/prctl/helper/SymbolicMdpPrctlHelper.h"
#include "storm/modelchecker/results/SymbolicQuantitativeCheckResult.h"
#include "storm/models/symbolic/NondeterministicModel.h"
#include "storm/solver/SymbolicMinMaxLinearEquationSolver.h"
#include "storm/utility/Extremum.h"
#include "storm/utility/NumberTraits.h"
#include "storm/utility/RationalApproximation.h"
#include "storm/utility/SignalHandler.h"
#include "storm/utility/constants.h"
#include "storm/utility/graph.h"
#include "storm/utility/macros.h"

namespace storm::modelchecker {

namespace {

using VariableSet = std::set<storm::expressions::Variable>;
using VariablePairs = std::vector<std::pair<storm::expressions::Variable, storm::expressions::Variable>>;

/*!
 * The normal form of the conditional query (see the sparse ConditionalHelper for a detailed description).
 */
template<storm::dd::DdType Type, typename ValueType>
struct SymbolicNormalForm {
    storm::dd::Bdd<Type> maybeStates;                          // non-terminal states reachable from the initial state without visiting terminal states
    storm::dd::Bdd<Type> terminalStates;                       // states whose target and condition values are known
    storm::dd::Bdd<Type> conditionStates;                      // states where the condition holds almost surely under all schedulers
    storm::dd::Bdd<Type> universalObservationFailureStates;    // states that can not reach the condition
    storm::dd::Bdd<Type> existentialObservationFailureStates;  // states from which some scheduler avoids the condition (and acts optimally afterwards)
    storm::dd::Add<Type, ValueType> targetValues;              // the target value of terminal states (zero elsewhere)
};

/// Returns a fresh name for a meta variable.
template<storm::dd::DdType Type>
std::string freshName(storm::dd::DdManager<Type> const& manager, std::string const& base) {
    std::string name = base;
    for (uint64_t index = 0; manager.hasMetaVariable(name); ++index) {
        name = base + "_" + std::to_string(index);
    }
    return name;
}

/// The successors of the given states (as row states).
template<storm::dd::DdType Type, typename ValueType>
storm::dd::Bdd<Type> successors(storm::models::symbolic::NondeterministicModel<Type, ValueType> const& model, storm::dd::Bdd<Type> const& states,
                                storm::dd::Bdd<Type> const& transitions) {
    return states.relationalProduct(transitions, model.getRowVariables(), model.getColumnVariables());
}

template<storm::dd::DdType Type, typename ValueType>
SymbolicNormalForm<Type, ValueType> obtainNormalForm(Environment const& env, storm::solver::OptimizationDirection dir,
                                                     storm::models::symbolic::NondeterministicModel<Type, ValueType> const& model,
                                                     storm::dd::Bdd<Type> const& targetStates, storm::dd::Bdd<Type> const& conditionStates) {
    using Helper = storm::modelchecker::helper::SymbolicMdpPrctlHelper<Type, ValueType>;
    auto const& reachable = model.getReachableStates();
    auto const transitions = model.getTransitionMatrix().notZero();
    // Prob1A for the condition and the target, Prob0A for the extended condition
    auto const extendedConditionStates = storm::utility::graph::performProb01Min(model, reachable, conditionStates).second;
    auto const universalObservationFailureStates = storm::utility::graph::performProb01Max(model, reachable, extendedConditionStates).first;
    auto const extendedTargetStates = storm::utility::graph::performProb01Min(model, reachable, targetStates).second;
    auto const targetAndNotCondFailStates = extendedTargetStates && !(extendedConditionStates || universalObservationFailureStates);

    // The optimal probabilities to reach the target (after the condition) and the condition (after the target).
    auto reachabilityEnv = env;
    if constexpr (!storm::NumberTraits<ValueType>::IsExact) {
        // For exact computations, the (zero) conditional precision is meaningless for the solver (and breaks rational search).
        reachabilityEnv.solver().minMax().setPrecision(env.modelchecker().conditional().getPrecision());
        reachabilityEnv.solver().minMax().setRelativeTerminationCriterion(env.modelchecker().conditional().isRelativePrecision());
    }
    auto zero = model.getManager().template getAddZero<ValueType>();
    storm::dd::Add<Type, ValueType> targetValues = zero;
    if (!extendedConditionStates.isZero()) {
        auto result = Helper::computeUntilProbabilities(reachabilityEnv, dir, model, model.getTransitionMatrix(), reachable, extendedTargetStates, false);
        targetValues += extendedConditionStates.ite(result->template asSymbolicQuantitativeCheckResult<Type, ValueType>().getValueVector(), zero);
    }
    if (!targetAndNotCondFailStates.isZero()) {
        auto result = Helper::computeUntilProbabilities(reachabilityEnv, dir, model, model.getTransitionMatrix(), reachable, extendedConditionStates, false);
        targetValues += targetAndNotCondFailStates.ite(result->template asSymbolicQuantitativeCheckResult<Type, ValueType>().getValueVector(), zero);
    }
    auto const terminalStatesThatReachCondition = extendedConditionStates || (targetAndNotCondFailStates && targetValues.notZero());

    auto terminalStates = extendedConditionStates || extendedTargetStates || universalObservationFailureStates;
    if (storm::solver::minimize(dir)) {
        // From states that can only reach observation failure (or target states with almost sure observation failure), the optimal policy enforces
        // observation failure. They are terminal with target value zero.
        terminalStates |= storm::utility::graph::performProb0A(model, transitions, reachable && !terminalStates, terminalStatesThatReachCondition);
    }
    auto const nonTerminalStates = reachable && !terminalStates;
    auto const existentialObservationFailureStates =
        storm::utility::graph::performProb0E(model, transitions, nonTerminalStates, terminalStatesThatReachCondition);

    // Restrict to the non-terminal states that can be reached from the initial state without visiting a terminal state.
    auto const stateTransitions = transitions.existsAbstract(model.getNondeterminismVariables());
    auto maybeStates = model.getInitialStates() && nonTerminalStates;
    for (auto frontier = maybeStates; !frontier.isZero();) {
        frontier = successors(model, frontier, stateTransitions) && nonTerminalStates && !maybeStates;
        maybeStates |= frontier;
    }
    return {maybeStates,
            terminalStates,
            extendedConditionStates,
            universalObservationFailureStates,
            existentialObservationFailureStates,
            terminalStates.ite(targetValues, zero)};
}

/*!
 * The weighted reachability problem max/min (w_t * P(target) + w_c * P(condition)) on the reduced model.
 *
 * As in the sparse implementation, the initial component (the states reachable from the initial state via choices that allow observation failure)
 * is collapsed into the initial state, which gets the exiting choices of all initial-component states. The source state of such a choice is encoded
 * in a copy of the row variables that is added to the nondeterminism variables.
 *
 * End components are not eliminated, since collapsing them would require a copy of the state variables for all states (which Sylvan can only place
 * below all other variables). Instead, each state that may stay in an end component forever gets an additional "stop" choice with value zero.
 * We always minimize: maximization is reduced to minimization by negating the weights, and the symbolic solver only masks illegal choices when
 * minimizing. With the stop choices, the desired solution is the greatest fixpoint of the Bellman operator: Every fixpoint is at most zero on
 * states that can stay forever and is thus bounded by the value of every scheduler. Value iteration reaches it when started from an upper bound and
 * policy iteration when started from the scheduler that stops wherever possible (a strict improvement never closes an end component).
 */
template<storm::dd::DdType Type, typename ValueType>
class SymbolicWeightedReachabilityHelper {
   public:
    SymbolicWeightedReachabilityHelper(storm::models::symbolic::NondeterministicModel<Type, ValueType> const& model,
                                       SymbolicNormalForm<Type, ValueType> const& normalForm)
        : rowVariables(model.getRowVariables()),
          columnVariables(model.getColumnVariables()),
          choiceVariables(model.getNondeterminismVariables()),
          pairs(model.getRowColumnMetaVariablePairs()) {
        auto& manager = model.getManager();
        auto const& T = model.getTransitionMatrix();
        auto const transitions = T.notZero();
        auto const init = model.getInitialStates();
        initAdd = init.template toAdd<ValueType>();
        auto const legalChoices = transitions.existsAbstract(columnVariables);
        auto toColumns = [this](storm::dd::Bdd<Type> const& states) { return states.swapVariables(pairs); };

        // Identify the initial component.
        auto const eofColumns = toColumns(normalForm.existentialObservationFailureStates);
        auto const eofChoices = legalChoices && !(transitions && !eofColumns).existsAbstract(columnVariables);
        auto const eofTransitions = (transitions && eofChoices).existsAbstract(choiceVariables);
        storm::dd::Bdd<Type> initialComponent = init;
        for (auto frontier = init; !frontier.isZero();) {
            frontier = successors(model, frontier, eofTransitions) && normalForm.maybeStates && !initialComponent;
            initialComponent |= frontier;
        }
        auto const initialComponentExits = legalChoices && initialComponent && !eofChoices;
        STORM_LOG_INFO("Initial component has " << initialComponent.getNonZeroCount() << " states.");

        // The states of the reduced model.
        auto const otherStates = normalForm.maybeStates && !initialComponent;
        states = otherStates || init;
        auto const statesAsColumns = toColumns(states).template toAdd<ValueType>();

        // Row values: the (weighted) probability mass that moves to terminal states.
        auto const terminalColumns = toColumns(normalForm.terminalStates);
        auto const targetValueColumns = normalForm.targetValues.swapVariables(pairs);
        auto const conditionValueColumns =
            toColumns(normalForm.conditionStates).template toAdd<ValueType>() +
            (terminalColumns && !toColumns(normalForm.conditionStates)).ite(targetValueColumns, manager.template getAddZero<ValueType>());
        auto const targetRowValues = (T * targetValueColumns).sumAbstract(columnVariables);
        auto const conditionRowValues = (T * conditionValueColumns).sumAbstract(columnVariables);
        // Transitions within the reduced model. Mass into terminal states is covered by the row values, mass into the initial component (except
        // for the initial state itself) is dropped (it yields value zero for both target and condition).
        auto const reducedT = T * statesAsColumns;

        // Choices of the states outside the initial component are kept as they are.
        auto const otherAdd = otherStates.template toAdd<ValueType>();
        matrix = reducedT * otherAdd;
        targetValues = targetRowValues * otherAdd;
        conditionValues = conditionRowValues * otherAdd;
        legal = legalChoices && otherStates;

        // Choices of the initial state are all exits of the initial component.
        if (initialComponent == init) {
            auto const exitsAdd = initialComponentExits.template toAdd<ValueType>();
            matrix += reducedT * exitsAdd;
            targetValues += targetRowValues * exitsAdd;
            conditionValues += conditionRowValues * exitsAdd;
            legal |= initialComponentExits;
        } else {
            // Encode the source state of each exit in a copy of the row variables.
            VariablePairs rowToSource;
            for (auto const& rowVariable : rowVariables) {
                auto const& ddVariable = manager.getMetaVariable(rowVariable);
                std::string const name = freshName(manager, "conditional_source_" + rowVariable.getName());
                std::vector<storm::expressions::Variable> copy;
                // We create two layers (and only use the first) since Sylvan expects DD variables to come in (unprimed, primed) pairs.
                if (ddVariable.getType() == storm::dd::MetaVariableType::Bool) {
                    copy = manager.addMetaVariable(name, 2);
                } else if (ddVariable.getType() == storm::dd::MetaVariableType::Int) {
                    copy = manager.addMetaVariable(name, ddVariable.getLow(), ddVariable.getHigh(), 2);
                } else {
                    copy = manager.addBitVectorMetaVariable(name, ddVariable.getNumberOfDdVariables(), 2);
                }
                rowToSource.emplace_back(rowVariable, copy.front());
                choiceVariables.insert(copy.front());
            }
            auto lift = [&](storm::dd::Add<Type, ValueType> const& add) { return add.swapVariables(rowToSource) * initAdd; };
            auto const exitsAdd = initialComponentExits.template toAdd<ValueType>();
            matrix += lift(reducedT * exitsAdd);
            targetValues += lift(targetRowValues * exitsAdd);
            conditionValues += lift(conditionRowValues * exitsAdd);
            legal |= init && initialComponentExits.swapVariables(rowToSource);
        }

        // States that can stay in an end component forever (the initial state can not, since it can not yield observation failure).
        storm::dd::Bdd<Type> const otherTransitions = transitions && otherStates;
        // The states from which some scheduler stays in the remaining states forever (with probability one). These are the states of end components
        // and the states that can reach one without leaving. Giving all of them a stop choice is sound since staying forever is worth zero.
        stopStates = otherStates;
        for (storm::dd::Bdd<Type> previous = manager.getBddZero(); previous != stopStates;) {
            previous = stopStates;
            auto const leaving = (otherTransitions && !toColumns(stopStates)).existsAbstract(columnVariables);
            stopStates = (legalChoices && stopStates && !leaving).existsAbstract(model.getNondeterminismVariables());
        }
        STORM_LOG_INFO("Found " << stopStates.getNonZeroCount() << " states that can stay in an end component.");

        if (!stopStates.isZero()) {
            // Add a stop choice to these states. It is encoded by a fresh nondeterminism variable.
            // Again, two layers keep the DD variables in (unprimed, primed) pairs as required by Sylvan.
            auto const stopVariable = manager.addMetaVariable(freshName(manager, "conditional_stop"), 2).front();
            auto const stop = manager.getEncoding(stopVariable, 1);
            auto const noStopAdd = (!stop).template toAdd<ValueType>();
            matrix *= noStopAdd;
            targetValues *= noStopAdd;
            conditionValues *= noStopAdd;
            storm::dd::Bdd<Type> stopChoice = stop && stopStates;
            for (auto const& variable : model.getNondeterminismVariables()) {
                stopChoice &= manager.getEncoding(variable, 0);
            }
            legal = (legal && !stop) || stopChoice;
            choiceVariables.insert(stopVariable);
            // The initial scheduler stops wherever possible.
            initialScheduler = (stopChoice || (legal && !stopStates)).existsAbstractRepresentative(choiceVariables);
        }
        illegal = states && !legal;
        STORM_LOG_INFO("Reduced model has " << states.getNonZeroCount() << " states and " << matrix.getNodeCount() << " matrix nodes.");
    }

    /// Computes the optimal value of targetWeight * P(target) + conditionWeight * P(condition) at the initial state.
    ValueType computeWeightedDiff(Environment const& env, storm::solver::OptimizationDirection dir, ValueType const& targetWeight,
                                  ValueType const& conditionWeight) {
        auto& manager = matrix.getDdManager();
        auto const method = env.solver().minMax().getMethod();
        // The symbolic solver uses value iteration for all methods other than policy iteration and rational search.
        bool const valueIteration = method != storm::solver::MinMaxMethod::PolicyIteration && method != storm::solver::MinMaxMethod::RationalSearch;
        if (!solver) {
            solver = storm::solver::GeneralSymbolicMinMaxLinearEquationSolverFactory<Type, ValueType>().create(matrix, states, illegal, rowVariables,
                                                                                                               columnVariables, choiceVariables, pairs);
            solver->setBounds(-storm::utility::one<ValueType>(), storm::utility::one<ValueType>());
            // Value iteration starting from an upper bound converges to the greatest fixpoint, which is the desired solution (see above).
            // We always provide such a starting point. Policy iteration instead starts with the scheduler that stops wherever possible.
            if (valueIteration || stopStates.isZero()) {
                solver->setHasUniqueSolution(true);
            }
            auto requirements = solver->getRequirements(env, storm::solver::OptimizationDirection::Minimize);
            if (requirements.validInitialScheduler()) {
                solver->setInitialScheduler(*initialScheduler);
                requirements.clearValidInitialScheduler();
            }
            requirements.clearBounds();
            STORM_LOG_THROW(!requirements.hasEnabledCriticalRequirement(), storm::exceptions::UncheckedRequirementException,
                            "Solver requirements " + requirements.getEnabledRequirementsAsString() +
                                " not checked. The symbolic conditional computation supports value iteration and policy iteration.");
            solver->setRequirementsChecked();
        }
        // Maximization is reduced to minimization, see above.
        bool const negate = storm::solver::maximize(dir);
        ValueType const tw = negate ? -targetWeight : targetWeight;
        ValueType const cw = negate ? -conditionWeight : conditionWeight;
        auto const b = targetValues * manager.getConstant(tw) + conditionValues * manager.getConstant(cw);

        storm::dd::Add<Type, ValueType> start = manager.template getAddZero<ValueType>();
        if (valueIteration) {
            // Since target and condition probabilities are in [0,1], the solution is at most |tw| + |cw|. Moreover, changing the weights by
            // (dt, dc) changes the solution by at most |dt| + |dc|. We use this to warm-start from the previous solution.
            ValueType shift = storm::utility::abs(tw) + storm::utility::abs(cw);
            if (lastSolution) {
                shift = storm::utility::abs<ValueType>(tw - lastTargetWeight) + storm::utility::abs<ValueType>(cw - lastConditionWeight);
                start = *lastSolution;
            }
            start += states.template toAdd<ValueType>() * manager.getConstant(shift);
        }
        auto const x = solver->solveEquations(env, storm::solver::OptimizationDirection::Minimize, start, b);
        if (valueIteration) {
            lastSolution = x;
            lastTargetWeight = tw;
            lastConditionWeight = cw;
        }
        ValueType const result = (x * initAdd).sumAbstract(rowVariables).getMax();
        return negate ? -result : result;
    }

   private:
    VariableSet rowVariables;
    VariableSet columnVariables;
    VariableSet choiceVariables;
    VariablePairs pairs;
    storm::dd::Bdd<Type> states;
    storm::dd::Bdd<Type> legal;
    storm::dd::Bdd<Type> illegal;
    storm::dd::Bdd<Type> stopStates;
    std::optional<storm::dd::Bdd<Type>> initialScheduler;
    storm::dd::Add<Type, ValueType> matrix;
    storm::dd::Add<Type, ValueType> targetValues;
    storm::dd::Add<Type, ValueType> conditionValues;
    storm::dd::Add<Type, ValueType> initAdd;
    std::unique_ptr<storm::solver::SymbolicMinMaxLinearEquationSolver<Type, ValueType>> solver;
    std::optional<storm::dd::Add<Type, ValueType>> lastSolution;
    ValueType lastTargetWeight;
    ValueType lastConditionWeight;
};

template<storm::dd::DdType Type, typename ValueType>
ValueType valueAtInitialState(storm::models::symbolic::NondeterministicModel<Type, ValueType> const& model, storm::dd::Add<Type, ValueType> const& values) {
    return (values * model.getInitialStates().template toAdd<ValueType>()).sumAbstract(model.getRowVariables()).getMax();
}

/// The value if it can be determined without solving the weighted reachability problem (see the sparse implementation).
template<storm::dd::DdType Type, typename ValueType>
std::optional<ValueType> handleTrivialCases(storm::models::symbolic::NondeterministicModel<Type, ValueType> const& model,
                                            SymbolicNormalForm<Type, ValueType> const& normalForm) {
    auto const& init = model.getInitialStates();
    if (!(init && normalForm.terminalStates).isZero()) {
        if (!(init && normalForm.conditionStates).isZero()) {
            return valueAtInitialState(model, normalForm.targetValues);
        }
        STORM_LOG_THROW((init && normalForm.universalObservationFailureStates).isZero(), storm::exceptions::InvalidPropertyException,
                        "Trying to compute undefined conditional probability: the condition has probability 0 under all policies.");
        return storm::utility::one<ValueType>();
    }
    if (normalForm.targetValues.isZero()) {
        return storm::utility::zero<ValueType>();
    }
    return std::nullopt;
}

template<storm::dd::DdType Type, typename ValueType>
ValueType computeViaBisection(Environment const& env, bool useAdvancedBounds, storm::solver::OptimizationDirection dir,
                              SymbolicWeightedReachabilityHelper<Type, ValueType>& wrh, std::optional<ValueType> const& threshold) {
    bool const relative = env.modelchecker().conditional().isRelativePrecision();
    auto const precision = storm::utility::convertNumber<ValueType>(env.modelchecker().conditional().getPrecision());
    auto const zero = storm::utility::zero<ValueType>();
    auto const one = storm::utility::one<ValueType>();

    ValueType pMin = zero, pMax = one;
    if (useAdvancedBounds) {
        pMin = wrh.computeWeightedDiff(env, storm::OptimizationDirection::Minimize, zero, one);
        pMax = wrh.computeWeightedDiff(env, storm::OptimizationDirection::Maximize, zero, one);
        STORM_LOG_TRACE("Conditioning event bounds: [" << storm::utility::convertNumber<double>(pMin) << ", " << storm::utility::convertNumber<double>(pMax)
                                                       << "].");
    }
    storm::utility::Maximum<ValueType> lowerBound = zero;
    storm::utility::Minimum<ValueType> upperBound = one;

    ValueType middle = threshold ? *threshold : (*lowerBound + *upperBound) / 2;
    [[maybe_unused]] ValueType rationalCandidate = middle;
    [[maybe_unused]] uint64_t rationalCandidateCount = 0;
    std::set<ValueType> checkedMiddleValues;
    for (uint64_t iterationCount = 1; true; ++iterationCount) {
        ValueType const middleValue = wrh.computeWeightedDiff(env, dir, one, -middle);
        checkedMiddleValues.insert(middle);
        if (!useAdvancedBounds) {
            if (middleValue >= zero) {
                lowerBound &= middle;
            }
            if (middleValue <= zero) {
                upperBound &= middle;
            }
            middle = (*lowerBound + *upperBound) / 2;
        } else {
            if (middleValue >= zero) {
                lowerBound &= middle + (middleValue / pMax);
                upperBound &= middle + (middleValue / pMin);
            }
            if (middleValue <= zero) {
                lowerBound &= middle + (middleValue / pMin);
                upperBound &= middle + (middleValue / pMax);
            }
            middle = *lowerBound + (one + middleValue) * (*upperBound - *lowerBound) / 2;
            if (!storm::NumberTraits<ValueType>::IsExact && storm::utility::isAlmostZero(*upperBound - *lowerBound)) {
                if (*lowerBound > *upperBound) {
                    std::swap(*lowerBound, *upperBound);
                }
            } else {
                STORM_LOG_ASSERT(middle >= *lowerBound && middle <= *upperBound, "Bisection method bounds are inconsistent.");
            }
        }

        ValueType const boundDiff = *upperBound - *lowerBound;
        STORM_LOG_TRACE("Iteration #" << iterationCount << ": bounds [" << *lowerBound << ", " << *upperBound << "], middle value " << middleValue << ".");
        if (boundDiff <= (relative ? (precision * *lowerBound) : precision)) {
            STORM_LOG_INFO("Bisection method converged after " << iterationCount << " iterations. Difference is "
                                                               << storm::utility::convertNumber<double>(boundDiff) << ".");
            break;
        }
        if (threshold && (*upperBound <= *threshold || *lowerBound >= *threshold)) {
            STORM_LOG_INFO("Bisection method determined result after " << iterationCount << " iterations.");
            break;
        }
        if (!storm::NumberTraits<ValueType>::IsExact && storm::utility::isAlmostZero(boundDiff)) {
            STORM_LOG_WARN("Precision of non-exact type exceeded: Bisection method has not terminated, but the difference between upper and lower bound is "
                           << boundDiff << ".");
        }
        if (storm::utility::resources::isTerminate()) {
            STORM_LOG_WARN("Bisection solver aborted after " << iterationCount << " iterations.");
            break;
        }
        // For exact computations, prefer rationals with a short representation (see the sparse implementation).
        if constexpr (storm::NumberTraits<ValueType>::IsExact) {
            if (rationalCandidateCount >= 4 && rationalCandidate >= *lowerBound && rationalCandidate <= *upperBound &&
                !checkedMiddleValues.contains(rationalCandidate)) {
                middle = rationalCandidate;
                rationalCandidateCount = 0;
            } else {
                bool const includeLower = !checkedMiddleValues.contains(*lowerBound);
                bool const includeUpper = !checkedMiddleValues.contains(*upperBound);
                auto newRationalCandidate = storm::utility::findRational(*lowerBound, includeLower, *upperBound, includeUpper);
                if (rationalCandidate == newRationalCandidate) {
                    ++rationalCandidateCount;
                } else {
                    rationalCandidate = newRationalCandidate;
                    rationalCandidateCount = 0;
                }
                ValueType delta = std::min<ValueType>(*upperBound - middle, middle - *lowerBound) / storm::utility::convertNumber<ValueType, uint64_t>(16);
                middle = storm::utility::findRational(middle - delta, true, middle + delta, true);
            }
        }
        if (iterationCount == 8) {
            if (storm::utility::isZero(*lowerBound) && !checkedMiddleValues.contains(zero)) {
                middle = zero;
            } else if (storm::utility::isOne(*upperBound) && !checkedMiddleValues.contains(one)) {
                middle = one;
            }
        }
    }
    return (*lowerBound + *upperBound) / 2;
}

/// Decides whether the conditional probability is above, below or equal to the threshold with a single weighted reachability query.
template<storm::dd::DdType Type, typename ValueType>
ValueType decideThreshold(Environment const& env, storm::solver::OptimizationDirection dir, SymbolicWeightedReachabilityHelper<Type, ValueType>& wrh,
                          ValueType const& threshold) {
    ValueType const value = wrh.computeWeightedDiff(env, dir, storm::utility::one<ValueType>(), -threshold);
    if (value > storm::utility::zero<ValueType>()) {
        return storm::utility::one<ValueType>();
    } else if (value < storm::utility::zero<ValueType>()) {
        return storm::utility::zero<ValueType>();
    }
    return threshold;
}

}  // namespace

template<storm::dd::DdType Type, typename ValueType>
std::unique_ptr<CheckResult> computeSymbolicConditionalProbabilities(Environment const& env, storm::solver::OptimizationDirection dir,
                                                                     storm::models::symbolic::NondeterministicModel<Type, ValueType> const& model,
                                                                     storm::dd::Bdd<Type> const& targetStates, storm::dd::Bdd<Type> const& conditionStates,
                                                                     std::optional<ValueType> const& threshold) {
    STORM_LOG_THROW(model.getInitialStates().getNonZeroCount() == 1, storm::exceptions::NotSupportedException,
                    "Conditional probabilities can only be computed for models with a single initial state.");
    auto analysisEnv = env;
    if (storm::NumberTraits<ValueType>::IsExact && env.modelchecker().conditional().isPrecisionSetFromDefault()) {
        STORM_LOG_INFO("Setting the conditional precision to 0 since the value type is exact and the precision was not explicitly set by the user.");
        analysisEnv.modelchecker().conditional().setPrecision(storm::utility::zero<storm::RationalNumber>(), false);
    }
    if (storm::NumberTraits<ValueType>::IsExact && env.solver().minMax().isMethodSetFromDefault()) {
        // Rational search (the default for exact symbolic solving) requires a unique solution, which we do not have in the presence of end components.
        analysisEnv.solver().minMax().setMethod(storm::solver::MinMaxMethod::PolicyIteration, false);
    }
    STORM_LOG_WARN_COND(!env.solver().isForceSoundness(),
                        "Bisection method does not adequately handle propagation of errors. Result is not necessarily sound.");

    auto const normalForm = obtainNormalForm(analysisEnv, dir, model, targetStates, conditionStates);
    ValueType value;
    if (auto trivialValue = handleTrivialCases(model, normalForm)) {
        value = *trivialValue;
        STORM_LOG_DEBUG("Initial state has trivial value " << value);
    } else {
        STORM_LOG_INFO("Analyzing normal form with " << normalForm.maybeStates.getNonZeroCount() << " maybe states.");
        auto alg = analysisEnv.modelchecker().conditional().getAlgorithm();
        bool useAdvancedBounds;
        switch (alg) {
            case ConditionalAlgorithmSetting::Default:
            case ConditionalAlgorithmSetting::Bisection:
            case ConditionalAlgorithmSetting::BisectionPolicyTracking:
                useAdvancedBounds = false;
                break;
            case ConditionalAlgorithmSetting::BisectionAdvanced:
            case ConditionalAlgorithmSetting::BisectionAdvancedPolicyTracking:
                useAdvancedBounds = true;
                break;
            default:
                STORM_LOG_THROW(false, storm::exceptions::NotSupportedException,
                                "The conditional algorithm '" << alg << "' is not supported by the symbolic engine. Use a bisection variant.");
        }
        STORM_LOG_INFO_COND(alg != ConditionalAlgorithmSetting::BisectionPolicyTracking && alg != ConditionalAlgorithmSetting::BisectionAdvancedPolicyTracking,
                            "Policy tracking is not supported by the symbolic engine, using plain bisection.");
        SymbolicWeightedReachabilityHelper<Type, ValueType> wrh(model, normalForm);
        if (threshold) {
            value = decideThreshold(analysisEnv, dir, wrh, *threshold);
        } else {
            value = computeViaBisection(analysisEnv, useAdvancedBounds, dir, wrh, threshold);
        }
    }
    auto const& init = model.getInitialStates();
    return std::make_unique<SymbolicQuantitativeCheckResult<Type, ValueType>>(model.getReachableStates(), init,
                                                                              init.template toAdd<ValueType>() * model.getManager().getConstant(value));
}

template std::unique_ptr<CheckResult> computeSymbolicConditionalProbabilities(
    Environment const&, storm::solver::OptimizationDirection, storm::models::symbolic::NondeterministicModel<storm::dd::DdType::CUDD, double> const&,
    storm::dd::Bdd<storm::dd::DdType::CUDD> const&, storm::dd::Bdd<storm::dd::DdType::CUDD> const&, std::optional<double> const&);
template std::unique_ptr<CheckResult> computeSymbolicConditionalProbabilities(
    Environment const&, storm::solver::OptimizationDirection, storm::models::symbolic::NondeterministicModel<storm::dd::DdType::Sylvan, double> const&,
    storm::dd::Bdd<storm::dd::DdType::Sylvan> const&, storm::dd::Bdd<storm::dd::DdType::Sylvan> const&, std::optional<double> const&);
template std::unique_ptr<CheckResult> computeSymbolicConditionalProbabilities(
    Environment const&, storm::solver::OptimizationDirection,
    storm::models::symbolic::NondeterministicModel<storm::dd::DdType::Sylvan, storm::RationalNumber> const&, storm::dd::Bdd<storm::dd::DdType::Sylvan> const&,
    storm::dd::Bdd<storm::dd::DdType::Sylvan> const&, std::optional<storm::RationalNumber> const&);

}  // namespace storm::modelchecker
