#include "storm/modelchecker/prctl/SymbolicMdpPrctlModelChecker.h"

#include "storm/exceptions/InvalidArgumentException.h"
#include "storm/exceptions/InvalidPropertyException.h"
#include "storm/exceptions/InvalidStateException.h"
#include "storm/logic/FragmentSpecification.h"
#include "storm/modelchecker/helper/conditional/SymbolicConditionalHelper.h"
#include "storm/modelchecker/prctl/helper/SymbolicMdpPrctlHelper.h"
#include "storm/modelchecker/results/SymbolicQualitativeCheckResult.h"
#include "storm/modelchecker/results/SymbolicQuantitativeCheckResult.h"
#include "storm/models/symbolic/StandardRewardModel.h"
#include "storm/utility/FilteredRewardModel.h"
#include "storm/utility/graph.h"
#include "storm/utility/macros.h"

namespace storm {
namespace modelchecker {

template<typename ModelType>
SymbolicMdpPrctlModelChecker<ModelType>::SymbolicMdpPrctlModelChecker(ModelType const& model) : SymbolicPropositionalModelChecker<ModelType>(model) {
    // Intentionally left empty.
}

template<typename ModelType>
bool SymbolicMdpPrctlModelChecker<ModelType>::canHandleStatic(CheckTask<storm::logic::Formula, ValueType> const& checkTask) {
    storm::logic::Formula const& formula = checkTask.getFormula();
    return formula.isInFragment(storm::logic::prctl()
                                    .setLongRunAverageRewardFormulasAllowed(false)
                                    .setConditionalProbabilityFormulasAllowed(true)
                                    .setOnlyEventuallyFormuluasInConditionalFormulasAllowed(true)
                                    .setTimeOperatorsAllowed(true)
                                    .setReachbilityTimeFormulasAllowed(true)
                                    .setRewardAccumulationAllowed(true));
}

template<typename ModelType>
bool SymbolicMdpPrctlModelChecker<ModelType>::canHandle(CheckTask<storm::logic::Formula, ValueType> const& checkTask) const {
    return canHandleStatic(checkTask);
}

template<typename ModelType>
std::unique_ptr<CheckResult> SymbolicMdpPrctlModelChecker<ModelType>::computeUntilProbabilities(
    Environment const& env, CheckTask<storm::logic::UntilFormula, ValueType> const& checkTask) {
    storm::logic::UntilFormula const& pathFormula = checkTask.getFormula();
    STORM_LOG_THROW(checkTask.isOptimizationDirectionSet(), storm::exceptions::InvalidPropertyException,
                    "Formula needs to specify whether minimal or maximal values are to be computed on nondeterministic model.");
    std::unique_ptr<CheckResult> leftResultPointer = this->check(env, pathFormula.getLeftSubformula());
    std::unique_ptr<CheckResult> rightResultPointer = this->check(env, pathFormula.getRightSubformula());
    SymbolicQualitativeCheckResult<DdType> const& leftResult = leftResultPointer->asSymbolicQualitativeCheckResult<DdType>();
    SymbolicQualitativeCheckResult<DdType> const& rightResult = rightResultPointer->asSymbolicQualitativeCheckResult<DdType>();
    return storm::modelchecker::helper::SymbolicMdpPrctlHelper<DdType, ValueType>::computeUntilProbabilities(
        env, checkTask.getOptimizationDirection(), this->getModel(), this->getModel().getTransitionMatrix(), leftResult.getTruthValuesVector(),
        rightResult.getTruthValuesVector(), checkTask.isQualitativeSet());
}

template<typename ModelType>
std::unique_ptr<CheckResult> SymbolicMdpPrctlModelChecker<ModelType>::computeConditionalProbabilities(
    Environment const& env, CheckTask<storm::logic::ConditionalFormula, ValueType> const& checkTask) {
    storm::logic::ConditionalFormula const& conditionalFormula = checkTask.getFormula();
    STORM_LOG_THROW(checkTask.isOptimizationDirectionSet(), storm::exceptions::InvalidPropertyException,
                    "Formula needs to specify whether minimal or maximal values are to be computed on nondeterministic model.");
    STORM_LOG_THROW(this->getModel().getInitialStates().getNonZeroCount() == 1, storm::exceptions::InvalidPropertyException,
                    "Cannot compute conditional probabilities on MDPs with more than one initial state.");
    STORM_LOG_THROW(checkTask.isOnlyInitialStatesRelevantSet(), storm::exceptions::InvalidPropertyException,
                    "Conditional probabilities can only be computed for the initial states of the model.");
    STORM_LOG_THROW(conditionalFormula.getSubformula().isEventuallyFormula() && conditionalFormula.getConditionFormula().isEventuallyFormula(),
                    storm::exceptions::InvalidPropertyException, "Illegal conditional probability formula.");

    std::unique_ptr<CheckResult> targetResult = this->check(env, conditionalFormula.getSubformula().asEventuallyFormula().getSubformula());
    std::unique_ptr<CheckResult> conditionResult = this->check(env, conditionalFormula.getConditionFormula().asEventuallyFormula().getSubformula());
    std::optional<ValueType> threshold;
    if (checkTask.isBoundSet()) {
        threshold = checkTask.getBoundThreshold();
    }
    return storm::modelchecker::computeSymbolicConditionalProbabilities<DdType, ValueType>(
        env, checkTask.getOptimizationDirection(), this->getModel(), targetResult->asSymbolicQualitativeCheckResult<DdType>().getTruthValuesVector(),
        conditionResult->asSymbolicQualitativeCheckResult<DdType>().getTruthValuesVector(), threshold);
}

template<typename ModelType>
std::unique_ptr<CheckResult> SymbolicMdpPrctlModelChecker<ModelType>::computeGloballyProbabilities(
    Environment const& env, CheckTask<storm::logic::GloballyFormula, ValueType> const& checkTask) {
    storm::logic::GloballyFormula const& pathFormula = checkTask.getFormula();
    STORM_LOG_THROW(checkTask.isOptimizationDirectionSet(), storm::exceptions::InvalidArgumentException,
                    "Formula needs to specify whether minimal or maximal values are to be computed on nondeterministic model.");
    std::unique_ptr<CheckResult> subResultPointer = this->check(env, pathFormula.getSubformula());
    SymbolicQualitativeCheckResult<DdType> const& subResult = subResultPointer->asSymbolicQualitativeCheckResult<DdType>();
    return storm::modelchecker::helper::SymbolicMdpPrctlHelper<DdType, ValueType>::computeGloballyProbabilities(
        env, checkTask.getOptimizationDirection(), this->getModel(), this->getModel().getTransitionMatrix(), subResult.getTruthValuesVector(),
        checkTask.isQualitativeSet());
}

template<typename ModelType>
std::unique_ptr<CheckResult> SymbolicMdpPrctlModelChecker<ModelType>::computeNextProbabilities(
    Environment const& env, CheckTask<storm::logic::NextFormula, ValueType> const& checkTask) {
    storm::logic::NextFormula const& pathFormula = checkTask.getFormula();
    STORM_LOG_THROW(checkTask.isOptimizationDirectionSet(), storm::exceptions::InvalidPropertyException,
                    "Formula needs to specify whether minimal or maximal values are to be computed on nondeterministic model.");
    std::unique_ptr<CheckResult> subResultPointer = this->check(env, pathFormula.getSubformula());
    SymbolicQualitativeCheckResult<DdType> const& subResult = subResultPointer->asSymbolicQualitativeCheckResult<DdType>();
    return storm::modelchecker::helper::SymbolicMdpPrctlHelper<DdType, ValueType>::computeNextProbabilities(
        env, checkTask.getOptimizationDirection(), this->getModel(), this->getModel().getTransitionMatrix(), subResult.getTruthValuesVector());
}

template<typename ModelType>
std::unique_ptr<CheckResult> SymbolicMdpPrctlModelChecker<ModelType>::computeBoundedUntilProbabilities(
    Environment const& env, CheckTask<storm::logic::BoundedUntilFormula, ValueType> const& checkTask) {
    storm::logic::BoundedUntilFormula const& pathFormula = checkTask.getFormula();
    STORM_LOG_THROW(checkTask.isOptimizationDirectionSet(), storm::exceptions::InvalidArgumentException,
                    "Formula needs to specify whether minimal or maximal values are to be computed on nondeterministic model.");
    STORM_LOG_THROW(!pathFormula.hasLowerBound() && pathFormula.hasUpperBound(), storm::exceptions::InvalidPropertyException,
                    "Formula needs to have (a single) upper time bound, and no lower bound.");
    STORM_LOG_THROW(pathFormula.hasIntegerUpperBound(), storm::exceptions::InvalidPropertyException, "Formula needs to have discrete upper time bound.");
    std::unique_ptr<CheckResult> leftResultPointer = this->check(env, pathFormula.getLeftSubformula());
    std::unique_ptr<CheckResult> rightResultPointer = this->check(env, pathFormula.getRightSubformula());
    SymbolicQualitativeCheckResult<DdType> const& leftResult = leftResultPointer->asSymbolicQualitativeCheckResult<DdType>();
    SymbolicQualitativeCheckResult<DdType> const& rightResult = rightResultPointer->asSymbolicQualitativeCheckResult<DdType>();
    return storm::modelchecker::helper::SymbolicMdpPrctlHelper<DdType, ValueType>::computeBoundedUntilProbabilities(
        env, checkTask.getOptimizationDirection(), this->getModel(), this->getModel().getTransitionMatrix(), leftResult.getTruthValuesVector(),
        rightResult.getTruthValuesVector(), pathFormula.getNonStrictUpperBound<uint64_t>());
}

template<typename ModelType>
std::unique_ptr<CheckResult> SymbolicMdpPrctlModelChecker<ModelType>::computeCumulativeRewards(
    Environment const& env, CheckTask<storm::logic::CumulativeRewardFormula, ValueType> const& checkTask) {
    storm::logic::CumulativeRewardFormula const& rewardPathFormula = checkTask.getFormula();
    STORM_LOG_THROW(checkTask.isOptimizationDirectionSet(), storm::exceptions::InvalidArgumentException,
                    "Formula needs to specify whether minimal or maximal values are to be computed on nondeterministic model.");
    STORM_LOG_THROW(rewardPathFormula.hasIntegerBound(), storm::exceptions::InvalidPropertyException, "Formula needs to have a discrete time bound.");
    auto rewardModel = storm::utility::createFilteredRewardModel(this->getModel(), checkTask);
    return storm::modelchecker::helper::SymbolicMdpPrctlHelper<DdType, ValueType>::computeCumulativeRewards(
        env, checkTask.getOptimizationDirection(), this->getModel(), this->getModel().getTransitionMatrix(), rewardModel.get(),
        rewardPathFormula.getNonStrictBound<uint64_t>());
}

template<typename ModelType>
std::unique_ptr<CheckResult> SymbolicMdpPrctlModelChecker<ModelType>::computeInstantaneousRewards(
    Environment const& env, CheckTask<storm::logic::InstantaneousRewardFormula, ValueType> const& checkTask) {
    storm::logic::InstantaneousRewardFormula const& rewardPathFormula = checkTask.getFormula();
    STORM_LOG_THROW(checkTask.isOptimizationDirectionSet(), storm::exceptions::InvalidArgumentException,
                    "Formula needs to specify whether minimal or maximal values are to be computed on nondeterministic model.");
    STORM_LOG_THROW(rewardPathFormula.hasIntegerBound(), storm::exceptions::InvalidPropertyException, "Formula needs to have a discrete time bound.");
    return storm::modelchecker::helper::SymbolicMdpPrctlHelper<DdType, ValueType>::computeInstantaneousRewards(
        env, checkTask.getOptimizationDirection(), this->getModel(), this->getModel().getTransitionMatrix(),
        checkTask.isRewardModelSet() ? this->getModel().getRewardModel(checkTask.getRewardModel()) : this->getModel().getRewardModel(""),
        rewardPathFormula.getBound<uint64_t>());
}

template<typename ModelType>
std::unique_ptr<CheckResult> SymbolicMdpPrctlModelChecker<ModelType>::computeReachabilityRewards(
    Environment const& env, CheckTask<storm::logic::EventuallyFormula, ValueType> const& checkTask) {
    storm::logic::EventuallyFormula const& eventuallyFormula = checkTask.getFormula();
    STORM_LOG_THROW(checkTask.isOptimizationDirectionSet(), storm::exceptions::InvalidPropertyException,
                    "Formula needs to specify whether minimal or maximal values are to be computed on nondeterministic model.");
    std::unique_ptr<CheckResult> subResultPointer = this->check(env, eventuallyFormula.getSubformula());
    SymbolicQualitativeCheckResult<DdType> const& subResult = subResultPointer->asSymbolicQualitativeCheckResult<DdType>();
    auto rewardModel = storm::utility::createFilteredRewardModel(this->getModel(), checkTask);
    return storm::modelchecker::helper::SymbolicMdpPrctlHelper<DdType, ValueType>::computeReachabilityRewards(
        env, checkTask.getOptimizationDirection(), this->getModel(), this->getModel().getTransitionMatrix(), rewardModel.get(),
        subResult.getTruthValuesVector());
}

template<typename ModelType>
std::unique_ptr<CheckResult> SymbolicMdpPrctlModelChecker<ModelType>::computeReachabilityTimes(
    Environment const& env, CheckTask<storm::logic::EventuallyFormula, ValueType> const& checkTask) {
    storm::logic::EventuallyFormula const& eventuallyFormula = checkTask.getFormula();
    STORM_LOG_THROW(checkTask.isOptimizationDirectionSet(), storm::exceptions::InvalidPropertyException,
                    "Formula needs to specify whether minimal or maximal values are to be computed on nondeterministic model.");
    std::unique_ptr<CheckResult> subResultPointer = this->check(env, eventuallyFormula.getSubformula());
    SymbolicQualitativeCheckResult<DdType> const& subResult = subResultPointer->asSymbolicQualitativeCheckResult<DdType>();
    return storm::modelchecker::helper::SymbolicMdpPrctlHelper<DdType, ValueType>::computeReachabilityTimes(
        env, checkTask.getOptimizationDirection(), this->getModel(), this->getModel().getTransitionMatrix(), subResult.getTruthValuesVector());
}

template class SymbolicMdpPrctlModelChecker<storm::models::symbolic::Mdp<storm::dd::DdType::CUDD, double>>;
template class SymbolicMdpPrctlModelChecker<storm::models::symbolic::Mdp<storm::dd::DdType::Sylvan, double>>;

template class SymbolicMdpPrctlModelChecker<storm::models::symbolic::Mdp<storm::dd::DdType::Sylvan, storm::RationalNumber>>;
}  // namespace modelchecker
}  // namespace storm
