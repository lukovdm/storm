#include "ApproximatePOMDPModelchecker.h"

#include <tuple>

#include <boost/algorithm/string.hpp>

#include "storm-pomdp/analysis/FormulaInformation.h"

#include "storm/utility/ConstantsComparator.h"
#include "storm/utility/NumberTraits.h"
#include "storm/utility/graph.h"
#include "storm/logic/Formulas.h"

#include "storm/models/sparse/Dtmc.h"
#include "storm/models/sparse/StandardRewardModel.h"
#include "storm/modelchecker/prctl/SparseDtmcPrctlModelChecker.h"
#include "storm/utility/vector.h"
#include "storm/api/properties.h"
#include "storm/api/export.h"
#include "storm-pomdp/builder/BeliefMdpExplorer.h"
#include "storm-pomdp/modelchecker/TrivialPomdpValueBoundsModelChecker.h"

#include "storm/utility/macros.h"
#include "storm/utility/SignalHandler.h"
#include "storm/exceptions/NotSupportedException.h"

namespace storm {
    namespace pomdp {
        namespace modelchecker {
            
            template<typename PomdpModelType, typename BeliefValueType>
            ApproximatePOMDPModelchecker<PomdpModelType, BeliefValueType>::Result::Result(ValueType lower, ValueType upper) : lowerBound(lower), upperBound(upper) {
                // Intentionally left empty
            }
            
            template<typename PomdpModelType, typename BeliefValueType>
            typename ApproximatePOMDPModelchecker<PomdpModelType, BeliefValueType>::ValueType
            ApproximatePOMDPModelchecker<PomdpModelType, BeliefValueType>::Result::diff(bool relative) const {
                ValueType diff = upperBound - lowerBound;
                if (diff < storm::utility::zero<ValueType>()) {
                    STORM_LOG_WARN_COND(diff >= 1e-6, "Upper bound '" << upperBound << "' is smaller than lower bound '" << lowerBound << "': Difference is " << diff << ".");
                    diff = storm::utility::zero<ValueType >();
                }
                if (relative && !storm::utility::isZero(upperBound)) {
                    diff /= upperBound;
                }
                return diff;
            }
            
            template<typename PomdpModelType, typename BeliefValueType>
            bool ApproximatePOMDPModelchecker<PomdpModelType, BeliefValueType>::Result::updateLowerBound(ValueType const& value) {
                if (value > lowerBound) {
                    lowerBound = value;
                    return true;
                }
                return false;
            }
            
            template<typename PomdpModelType, typename BeliefValueType>
            bool ApproximatePOMDPModelchecker<PomdpModelType, BeliefValueType>::Result::updateUpperBound(ValueType const& value) {
                if (value < upperBound) {
                    upperBound = value;
                    return true;
                }
                return false;
            }
            
            template<typename PomdpModelType, typename BeliefValueType>
            ApproximatePOMDPModelchecker<PomdpModelType, BeliefValueType>::Statistics::Statistics() :  overApproximationBuildAborted(false), underApproximationBuildAborted(false), aborted(false) {
                // intentionally left empty;
            }
            
            template<typename PomdpModelType, typename BeliefValueType>
            ApproximatePOMDPModelchecker<PomdpModelType, BeliefValueType>::ApproximatePOMDPModelchecker(PomdpModelType const& pomdp, Options options) : pomdp(pomdp), options(options) {
                cc = storm::utility::ConstantsComparator<ValueType>(storm::utility::convertNumber<ValueType>(this->options.numericPrecision), false);
            }

            template<typename PomdpModelType, typename BeliefValueType>
            typename ApproximatePOMDPModelchecker<PomdpModelType, BeliefValueType>::Result ApproximatePOMDPModelchecker<PomdpModelType, BeliefValueType>::check(storm::logic::Formula const& formula) {
                STORM_LOG_ASSERT(options.unfold || options.discretize, "Invoked belief exploration but no task (unfold or discretize) given.");
                // Reset all collected statistics
                statistics = Statistics();
                statistics.totalTime.start();
                // Extract the relevant information from the formula
                auto formulaInfo = storm::pomdp::analysis::getFormulaInformation(pomdp, formula);
                
                // Compute some initial bounds on the values for each state of the pomdp
                auto initialPomdpValueBounds = TrivialPomdpValueBoundsModelChecker<storm::models::sparse::Pomdp<ValueType>>(pomdp).getValueBounds(formula, formulaInfo);
                Result result(initialPomdpValueBounds.lower[pomdp.getInitialStates().getNextSetIndex(0)], initialPomdpValueBounds.upper[pomdp.getInitialStates().getNextSetIndex(0)]);
                STORM_PRINT_AND_LOG("Initial value bounds are [" << result.lowerBound << ", " <<  result.upperBound << "]" << std::endl);

                boost::optional<std::string> rewardModelName;
                if (formulaInfo.isNonNestedReachabilityProbability() || formulaInfo.isNonNestedExpectedRewardFormula()) {
                    // FIXME: Instead of giving up, introduce a new observation for target states and make sink states absorbing.
                    STORM_LOG_THROW(formulaInfo.getTargetStates().observationClosed, storm::exceptions::NotSupportedException, "There are non-target states with the same observation as a target state. This is currently not supported");
                    if (formulaInfo.isNonNestedReachabilityProbability()) {
                        if (!formulaInfo.getSinkStates().empty()) {
                            auto reachableFromSinkStates = storm::utility::graph::getReachableStates(pomdp.getTransitionMatrix(), formulaInfo.getSinkStates().states, formulaInfo.getSinkStates().states, ~formulaInfo.getSinkStates().states);
                            reachableFromSinkStates &= ~formulaInfo.getSinkStates().states;
                            STORM_LOG_THROW(reachableFromSinkStates.empty(), storm::exceptions::NotSupportedException, "There are sink states that can reach non-sink states. This is currently not supported");
                        }
                    } else {
                        // Expected reward formula!
                        rewardModelName = formulaInfo.getRewardModelName();
                    }
                } else {
                    STORM_LOG_THROW(false, storm::exceptions::NotSupportedException, "Unsupported formula '" << formula << "'.");
                }
                
                if (options.refine) {
                    refineReachability(formulaInfo.getTargetStates().observations, formulaInfo.minimize(), rewardModelName, initialPomdpValueBounds.lower, initialPomdpValueBounds.upper, result);
                } else {
                    computeReachabilityOTF(formulaInfo.getTargetStates().observations, formulaInfo.minimize(), rewardModelName, initialPomdpValueBounds.lower, initialPomdpValueBounds.upper, result);
                }
                // "clear" results in case they were actually not requested (this will make the output a bit more clear)
                if ((formulaInfo.minimize() && !options.discretize) || (formulaInfo.maximize() && !options.unfold)) {
                    result.lowerBound = -storm::utility::infinity<ValueType>();
                }
                if ((formulaInfo.maximize() && !options.discretize) || (formulaInfo.minimize() && !options.unfold)) {
                    result.upperBound = storm::utility::infinity<ValueType>();
                }
                
                if (storm::utility::resources::isTerminate()) {
                    statistics.aborted = true;
                }
                statistics.totalTime.stop();
                return result;
            }
            
            template<typename PomdpModelType, typename BeliefValueType>
            void ApproximatePOMDPModelchecker<PomdpModelType, BeliefValueType>::printStatisticsToStream(std::ostream& stream) const {
                stream << "##### Grid Approximation Statistics ######" << std::endl;
                stream << "# Input model: " << std::endl;
                pomdp.printModelInformationToStream(stream);
                stream << "# Max. Number of states with same observation: " << pomdp.getMaxNrStatesWithSameObservation() << std::endl;
                
                if (statistics.aborted) {
                    stream << "# Computation aborted early" << std::endl;
                }
                
                stream << "# Total check time: " << statistics.totalTime << std::endl;
                
                // Refinement information:
                if (statistics.refinementSteps) {
                    stream << "# Number of refinement steps: " << statistics.refinementSteps.get() << std::endl;
                }
                
                // The overapproximation MDP:
                if (statistics.overApproximationStates) {
                    stream << "# Number of states in the ";
                    if (options.refine) {
                        stream << "final ";
                    }
                    stream << "grid MDP for the over-approximation: ";
                    if (statistics.overApproximationBuildAborted) {
                        stream << ">=";
                    }
                    stream << statistics.overApproximationStates.get() << std::endl;
                    stream << "# Maximal resolution for over-approximation: " << statistics.overApproximationMaxResolution.get() << std::endl;
                    stream << "# Time spend for building the over-approx grid MDP(s): " << statistics.overApproximationBuildTime << std::endl;
                    stream << "# Time spend for checking the over-approx grid MDP(s): " << statistics.overApproximationCheckTime << std::endl;
                }
                
                // The underapproximation MDP:
                if (statistics.underApproximationStates) {
                    stream << "# Number of states in the ";
                    if (options.refine) {
                        stream << "final ";
                    }
                    stream << "grid MDP for the under-approximation: ";
                    if (statistics.underApproximationBuildAborted) {
                        stream << ">=";
                    }
                    stream << statistics.underApproximationStates.get() << std::endl;
                    stream << "# Exploration state limit for under-approximation: " << statistics.underApproximationStateLimit.get() << std::endl;
                    stream << "# Time spend for building the under-approx grid MDP(s): " << statistics.underApproximationBuildTime << std::endl;
                    stream << "# Time spend for checking the under-approx grid MDP(s): " << statistics.underApproximationCheckTime << std::endl;
                }

                stream << "##########################################" << std::endl;
            }
            
            template<typename PomdpModelType, typename BeliefValueType>
            void ApproximatePOMDPModelchecker<PomdpModelType, BeliefValueType>::computeReachabilityOTF(std::set<uint32_t> const &targetObservations, bool min, boost::optional<std::string> rewardModelName, std::vector<ValueType> const& lowerPomdpValueBounds, std::vector<ValueType> const& upperPomdpValueBounds, Result& result) {
                
                if (options.discretize) {
                    std::vector<uint64_t> observationResolutionVector(pomdp.getNrObservations(), options.resolutionInit);
                    auto manager = std::make_shared<BeliefManagerType>(pomdp, options.numericPrecision, options.dynamicTriangulation ? BeliefManagerType::TriangulationMode::Dynamic : BeliefManagerType::TriangulationMode::Static);
                    if (rewardModelName) {
                        manager->setRewardModel(rewardModelName);
                    }
                    auto approx = std::make_shared<ExplorerType>(manager, lowerPomdpValueBounds, upperPomdpValueBounds);
                    HeuristicParameters heuristicParameters;
                    heuristicParameters.gapThreshold = options.gapThresholdInit;
                    heuristicParameters.observationThreshold = options.obsThresholdInit; // Actually not relevant without refinement
                    heuristicParameters.sizeThreshold = options.sizeThresholdInit == 0 ? std::numeric_limits<uint64_t>::max() : options.sizeThresholdInit;
                    heuristicParameters.optimalChoiceValueEpsilon = options.optimalChoiceValueThresholdInit;
                    
                    buildOverApproximation(targetObservations, min, rewardModelName.is_initialized(), false, heuristicParameters, observationResolutionVector, manager, approx);
                    if (approx->hasComputedValues()) {
                        STORM_PRINT_AND_LOG("Explored and checked Over-Approximation MDP:\n");
                        approx->getExploredMdp()->printModelInformationToStream(std::cout);
                        ValueType& resultValue = min ? result.lowerBound : result.upperBound;
                        resultValue = approx->getComputedValueAtInitialState();
                    }
                }
                if (options.unfold) { // Underapproximation (uses a fresh Belief manager)
                    auto manager = std::make_shared<BeliefManagerType>(pomdp, options.numericPrecision, options.dynamicTriangulation ? BeliefManagerType::TriangulationMode::Dynamic : BeliefManagerType::TriangulationMode::Static);
                    if (rewardModelName) {
                        manager->setRewardModel(rewardModelName);
                    }
                    auto approx = std::make_shared<ExplorerType>(manager, lowerPomdpValueBounds, upperPomdpValueBounds);
                    HeuristicParameters heuristicParameters;
                    heuristicParameters.gapThreshold = options.gapThresholdInit;
                    heuristicParameters.optimalChoiceValueEpsilon = options.optimalChoiceValueThresholdInit;
                    heuristicParameters.sizeThreshold = options.sizeThresholdInit;
                    if (heuristicParameters.sizeThreshold == 0) {
                        if (options.explorationTimeLimit) {
                            heuristicParameters.sizeThreshold = std::numeric_limits<uint64_t>::max();
                        } else {
                            heuristicParameters.sizeThreshold = pomdp.getNumberOfStates() * pomdp.getMaxNrStatesWithSameObservation();
                            STORM_PRINT_AND_LOG("Heuristically selected an under-approximation mdp size threshold of " << heuristicParameters.sizeThreshold << "." << std::endl);
                        }
                    }
                    buildUnderApproximation(targetObservations, min, rewardModelName.is_initialized(), false, heuristicParameters, manager, approx);
                    if (approx->hasComputedValues()) {
                        STORM_PRINT_AND_LOG("Explored and checked Under-Approximation MDP:\n");
                        approx->getExploredMdp()->printModelInformationToStream(std::cout);
                        ValueType& resultValue = min ? result.upperBound : result.lowerBound;
                        resultValue = approx->getComputedValueAtInitialState();
                    }
                }
            }
            
            template<typename PomdpModelType, typename BeliefValueType>
            void ApproximatePOMDPModelchecker<PomdpModelType, BeliefValueType>::refineReachability(std::set<uint32_t> const &targetObservations, bool min, boost::optional<std::string> rewardModelName, std::vector<ValueType> const& lowerPomdpValueBounds, std::vector<ValueType> const& upperPomdpValueBounds, Result& result) {
                statistics.refinementSteps = 0;

                // Set up exploration data
                std::vector<uint64_t> observationResolutionVector;
                std::shared_ptr<BeliefManagerType> overApproxBeliefManager;
                std::shared_ptr<ExplorerType> overApproximation;
                HeuristicParameters overApproxHeuristicPar;
                if (options.discretize) { // Setup and build first OverApproximation
                    observationResolutionVector = std::vector<uint64_t>(pomdp.getNrObservations(), options.resolutionInit);
                    overApproxBeliefManager = std::make_shared<BeliefManagerType>(pomdp, options.numericPrecision, options.dynamicTriangulation ? BeliefManagerType::TriangulationMode::Dynamic : BeliefManagerType::TriangulationMode::Static);
                    if (rewardModelName) {
                        overApproxBeliefManager->setRewardModel(rewardModelName);
                    }
                    overApproximation = std::make_shared<ExplorerType>(overApproxBeliefManager, lowerPomdpValueBounds, upperPomdpValueBounds);
                    overApproxHeuristicPar.gapThreshold = options.gapThresholdInit;
                    overApproxHeuristicPar.observationThreshold = options.obsThresholdInit;
                    overApproxHeuristicPar.sizeThreshold = options.sizeThresholdInit == 0 ? std::numeric_limits<uint64_t>::max() : options.sizeThresholdInit;
                    overApproxHeuristicPar.optimalChoiceValueEpsilon = options.optimalChoiceValueThresholdInit;
                    buildOverApproximation(targetObservations, min, rewardModelName.is_initialized(), false, overApproxHeuristicPar, observationResolutionVector, overApproxBeliefManager, overApproximation);
                    if (!overApproximation->hasComputedValues() || storm::utility::resources::isTerminate()) {
                        return;
                    }
                    ValueType const& newValue = overApproximation->getComputedValueAtInitialState();
                    bool betterBound = min ? result.updateLowerBound(newValue) : result.updateUpperBound(newValue);
                    if (betterBound) {
                        STORM_PRINT_AND_LOG("Over-approx result for refinement improved after " << statistics.totalTime << " seconds in refinement step #" << statistics.refinementSteps.get() << ". New value is '" << newValue << "'." << std::endl);
                    }
                }
                
                std::shared_ptr<BeliefManagerType> underApproxBeliefManager;
                std::shared_ptr<ExplorerType> underApproximation;
                HeuristicParameters underApproxHeuristicPar;
                if (options.unfold) { // Setup and build first UnderApproximation
                    underApproxBeliefManager = std::make_shared<BeliefManagerType>(pomdp, options.numericPrecision, options.dynamicTriangulation ? BeliefManagerType::TriangulationMode::Dynamic : BeliefManagerType::TriangulationMode::Static);
                    if (rewardModelName) {
                        underApproxBeliefManager->setRewardModel(rewardModelName);
                    }
                    underApproximation = std::make_shared<ExplorerType>(underApproxBeliefManager, lowerPomdpValueBounds, upperPomdpValueBounds);
                    underApproxHeuristicPar.gapThreshold = options.gapThresholdInit;
                    underApproxHeuristicPar.optimalChoiceValueEpsilon = options.optimalChoiceValueThresholdInit;
                    underApproxHeuristicPar.sizeThreshold = options.sizeThresholdInit;
                    if (underApproxHeuristicPar.sizeThreshold == 0) {
                        // Select a decent value automatically
                        underApproxHeuristicPar.sizeThreshold = pomdp.getNumberOfStates() * pomdp.getMaxNrStatesWithSameObservation();
                    }
                    buildUnderApproximation(targetObservations, min, rewardModelName.is_initialized(), false, underApproxHeuristicPar, underApproxBeliefManager, underApproximation);
                    if (!underApproximation->hasComputedValues() || storm::utility::resources::isTerminate()) {
                        return;
                    }
                    ValueType const& newValue = underApproximation->getComputedValueAtInitialState();
                    bool betterBound = min ? result.updateUpperBound(newValue) : result.updateLowerBound(newValue);
                    if (betterBound) {
                        STORM_PRINT_AND_LOG("Under-approx result for refinement improved after " << statistics.totalTime << " seconds in refinement step #" << statistics.refinementSteps.get() << ". New value is '" << newValue << "'." << std::endl);
                    }
                }
                
                // Do some output
                STORM_PRINT_AND_LOG("Completed iteration #" << statistics.refinementSteps.get() << ". Current checktime is " << statistics.totalTime << ".");
                bool computingLowerBound = false;
                bool computingUpperBound = false;
                if (options.discretize) {
                    STORM_PRINT_AND_LOG(" Over-approx MDP has size " << overApproximation->getExploredMdp()->getNumberOfStates() << ".");
                    (min ? computingLowerBound : computingUpperBound) = true;
                }
                if (options.unfold) {
                    STORM_PRINT_AND_LOG(" Under-approx MDP has size " << underApproximation->getExploredMdp()->getNumberOfStates() << ".");
                    (min ? computingUpperBound : computingLowerBound) = true;
                }
                if (computingLowerBound && computingUpperBound) {
                    STORM_PRINT_AND_LOG(" Current result is [" << result.lowerBound << ", " << result.upperBound << "].");
                } else if (computingLowerBound) {
                    STORM_PRINT_AND_LOG(" Current result is ≥" << result.lowerBound << ".");
                } else if (computingUpperBound) {
                    STORM_PRINT_AND_LOG(" Current result is ≤" << result.upperBound << ".");
                }
                STORM_PRINT_AND_LOG(std::endl);
                
                // Start refinement
                STORM_LOG_WARN_COND(options.refineStepLimit.is_initialized() || !storm::utility::isZero(options.refinePrecision), "No termination criterion for refinement given. Consider to specify a steplimit, a non-zero precisionlimit, or a timeout");
                STORM_LOG_WARN_COND(storm::utility::isZero(options.refinePrecision) || (options.unfold && options.discretize), "Refinement goal precision is given, but only one bound is going to be refined.");
                while ((!options.refineStepLimit.is_initialized() || statistics.refinementSteps.get() < options.refineStepLimit.get()) && result.diff() > options.refinePrecision) {
                    bool overApproxFixPoint = true;
                    bool underApproxFixPoint = true;
                    if (options.discretize) {
                        // Refine over-approximation
                        if (min) {
                            overApproximation->takeCurrentValuesAsLowerBounds();
                        } else {
                            overApproximation->takeCurrentValuesAsUpperBounds();
                        }
                        overApproxHeuristicPar.gapThreshold *= options.gapThresholdFactor;
                        overApproxHeuristicPar.sizeThreshold = storm::utility::convertNumber<uint64_t, ValueType>(storm::utility::convertNumber<ValueType, uint64_t>(overApproximation->getExploredMdp()->getNumberOfStates()) * options.sizeThresholdFactor);
                        overApproxHeuristicPar.observationThreshold += options.obsThresholdIncrementFactor * (storm::utility::one<ValueType>() - overApproxHeuristicPar.observationThreshold);
                        overApproxHeuristicPar.optimalChoiceValueEpsilon *= options.optimalChoiceValueThresholdFactor;
                        overApproxFixPoint = buildOverApproximation(targetObservations, min, rewardModelName.is_initialized(), true, overApproxHeuristicPar, observationResolutionVector, overApproxBeliefManager, overApproximation);
                        if (overApproximation->hasComputedValues() && !storm::utility::resources::isTerminate()) {
                            ValueType const& newValue = overApproximation->getComputedValueAtInitialState();
                            bool betterBound = min ? result.updateLowerBound(newValue) : result.updateUpperBound(newValue);
                            if (betterBound) {
                                STORM_PRINT_AND_LOG("Over-approx result for refinement improved after " << statistics.totalTime << " in refinement step #" << (statistics.refinementSteps.get() + 1) << ". New value is '" << newValue << "'." << std::endl);
                            }
                        } else {
                            break;
                        }
                    }
                    
                    if (options.unfold && result.diff() > options.refinePrecision) {
                        // Refine under-approximation
                        underApproxHeuristicPar.gapThreshold *= options.gapThresholdFactor;
                        underApproxHeuristicPar.sizeThreshold = storm::utility::convertNumber<uint64_t, ValueType>(storm::utility::convertNumber<ValueType, uint64_t>(underApproximation->getExploredMdp()->getNumberOfStates()) * options.sizeThresholdFactor);
                        underApproxHeuristicPar.optimalChoiceValueEpsilon *= options.optimalChoiceValueThresholdFactor;
                        underApproxFixPoint = buildUnderApproximation(targetObservations, min, rewardModelName.is_initialized(), true, underApproxHeuristicPar, underApproxBeliefManager, underApproximation);
                        if (underApproximation->hasComputedValues() && !storm::utility::resources::isTerminate()) {
                            ValueType const& newValue = underApproximation->getComputedValueAtInitialState();
                            bool betterBound = min ? result.updateUpperBound(newValue) : result.updateLowerBound(newValue);
                            if (betterBound) {
                                STORM_PRINT_AND_LOG("Under-approx result for refinement improved after " << statistics.totalTime << " in refinement step #" << (statistics.refinementSteps.get() + 1) << ". New value is '" << newValue << "'." << std::endl);
                            }
                        } else {
                            break;
                        }
                    }
                    
                    if (storm::utility::resources::isTerminate()) {
                        break;
                    } else {
                        ++statistics.refinementSteps.get();
                        // Don't make too many outputs (to avoid logfile clutter)
                        if (statistics.refinementSteps.get() <= 1000) {
                            STORM_PRINT_AND_LOG("Completed iteration #" << statistics.refinementSteps.get() << ". Current checktime is " << statistics.totalTime << ".");
                            bool computingLowerBound = false;
                            bool computingUpperBound = false;
                            if (options.discretize) {
                                STORM_PRINT_AND_LOG(" Over-approx MDP has size " << overApproximation->getExploredMdp()->getNumberOfStates() << ".");
                                (min ? computingLowerBound : computingUpperBound) = true;
                            }
                            if (options.unfold) {
                                STORM_PRINT_AND_LOG(" Under-approx MDP has size " << underApproximation->getExploredMdp()->getNumberOfStates() << ".");
                                (min ? computingUpperBound : computingLowerBound) = true;
                            }
                            if (computingLowerBound && computingUpperBound) {
                                STORM_PRINT_AND_LOG(" Current result is [" << result.lowerBound << ", " << result.upperBound << "].");
                            } else if (computingLowerBound) {
                                STORM_PRINT_AND_LOG(" Current result is ≥" << result.lowerBound << ".");
                            } else if (computingUpperBound) {
                                STORM_PRINT_AND_LOG(" Current result is ≤" << result.upperBound << ".");
                            }
                            STORM_PRINT_AND_LOG(std::endl);
                            STORM_LOG_WARN_COND(statistics.refinementSteps.get() < 1000, "Refinement requires  more than 1000 iterations.");
                        }
                    }
                    if (overApproxFixPoint && underApproxFixPoint) {
                        STORM_PRINT_AND_LOG("Refinement fixpoint reached after " << statistics.refinementSteps.get() << " iterations." << std::endl);
                        break;
                    }
                }
            }

            /*!
             * Heuristically rates the quality of the approximation described by the given successor observation info.
             * Here, 0 means a bad approximation and 1 means a good approximation.
             */
            template<typename PomdpModelType, typename BeliefValueType>
            typename ApproximatePOMDPModelchecker<PomdpModelType, BeliefValueType>::ValueType ApproximatePOMDPModelchecker<PomdpModelType, BeliefValueType>::rateObservation(typename ExplorerType::SuccessorObservationInformation const& info, uint64_t const& observationResolution, uint64_t const& maxResolution) {
                auto n = storm::utility::convertNumber<ValueType, uint64_t>(info.support.size());
                auto one = storm::utility::one<ValueType>();
                if (storm::utility::isOne(n)) {
                    // If the belief is Dirac, it has to be approximated precisely.
                    // In this case, we return the best possible rating
                    return one;
                } else {
                    // Create the rating for this observation at this choice from the given info
                    ValueType obsChoiceRating = info.maxProbabilityToSuccessorWithObs / info.observationProbability;
                    // At this point, obsRating is the largest triangulation weight (which ranges from 1/n to 1
                    // Normalize the rating so that it ranges from 0 to 1, where
                    // 0 means that the actual belief lies in the middle of the triangulating simplex (i.e. a "bad" approximation) and 1 means that the belief is precisely approximated.
                    obsChoiceRating = (obsChoiceRating * n - one) / (n - one);
                    // Scale the ratings with the resolutions, so that low resolutions get a lower rating (and are thus more likely to be refined)
                    obsChoiceRating *= storm::utility::convertNumber<ValueType>(observationResolution) / storm::utility::convertNumber<ValueType>(maxResolution);
                    return obsChoiceRating;
                }
            }
            
            template<typename PomdpModelType, typename BeliefValueType>
            std::vector<typename ApproximatePOMDPModelchecker<PomdpModelType, BeliefValueType>::ValueType> ApproximatePOMDPModelchecker<PomdpModelType, BeliefValueType>::getObservationRatings(std::shared_ptr<ExplorerType> const& overApproximation, std::vector<uint64_t> const& observationResolutionVector, uint64_t const& maxResolution) {
                uint64_t numMdpStates = overApproximation->getExploredMdp()->getNumberOfStates();
                auto const& choiceIndices = overApproximation->getExploredMdp()->getNondeterministicChoiceIndices();

                std::vector<ValueType> resultingRatings(pomdp.getNrObservations(), storm::utility::one<ValueType>());
                
                std::map<uint32_t, typename ExplorerType::SuccessorObservationInformation> gatheredSuccessorObservations; // Declare here to avoid reallocations
                for (uint64_t mdpState = 0; mdpState < numMdpStates; ++mdpState) {
                    // Check whether this state is reached under an optimal scheduler.
                    // The heuristic assumes that the remaining states are not relevant for the observation score.
                    if (overApproximation->stateIsOptimalSchedulerReachable(mdpState)) {
                        for (uint64_t mdpChoice = choiceIndices[mdpState]; mdpChoice < choiceIndices[mdpState + 1]; ++mdpChoice) {
                            // Similarly, only optimal actions are relevant
                            if (overApproximation->actionIsOptimal(mdpChoice)) {
                                // score the observations for this choice
                                gatheredSuccessorObservations.clear();
                                overApproximation->gatherSuccessorObservationInformationAtMdpChoice(mdpChoice, gatheredSuccessorObservations);
                                for (auto const& obsInfo : gatheredSuccessorObservations) {
                                    auto const& obs = obsInfo.first;
                                    ValueType obsChoiceRating = rateObservation(obsInfo.second, observationResolutionVector[obs], maxResolution);
             
                                    // The rating of the observation will be the minimum over all choice-based observation ratings
                                    resultingRatings[obs] = std::min(resultingRatings[obs], obsChoiceRating);
                                }
                            }
                        }
                    }
                }
                return resultingRatings;
            }
            
            template<typename PomdpModelType, typename BeliefValueType>
            bool ApproximatePOMDPModelchecker<PomdpModelType, BeliefValueType>::buildOverApproximation(std::set<uint32_t> const &targetObservations, bool min, bool computeRewards, bool refine, HeuristicParameters const& heuristicParameters, std::vector<uint64_t>& observationResolutionVector, std::shared_ptr<BeliefManagerType>& beliefManager, std::shared_ptr<ExplorerType>& overApproximation) {
                
                // Detect whether the refinement reached a fixpoint.
                bool fixPoint = true;
                
                // current maximal resolution (needed for refinement heuristic)
                uint64_t oldMaxResolution = *std::max_element(observationResolutionVector.begin(), observationResolutionVector.end());

                statistics.overApproximationBuildTime.start();
                storm::storage::BitVector refinedObservations;
                if (!refine) {
                    // If we build the model from scratch, we first have to setup the explorer for the overApproximation.
                    if (computeRewards) {
                        overApproximation->startNewExploration(storm::utility::zero<ValueType>());
                    } else {
                        overApproximation->startNewExploration(storm::utility::one<ValueType>(), storm::utility::zero<ValueType>());
                    }
                } else {
                    // If we refine the existing overApproximation, our heuristic also wants to know which states are reachable under an optimal policy
                    overApproximation->computeOptimalChoicesAndReachableMdpStates(heuristicParameters.optimalChoiceValueEpsilon, true);
                    // We also need to find out which observation resolutions needs refinement.
                    auto obsRatings = getObservationRatings(overApproximation, observationResolutionVector, oldMaxResolution);
                    // If there is a score < 1, we have not reached a fixpoint, yet
                    if (std::any_of(obsRatings.begin(), obsRatings.end(), [](ValueType const& value){return value < storm::utility::one<ValueType>();})) {
                        fixPoint = false;
                    }
                    refinedObservations = storm::utility::vector::filter<ValueType>(obsRatings, [&heuristicParameters](ValueType const& r) { return r <= heuristicParameters.observationThreshold;});
                    STORM_LOG_DEBUG("Refining the resolution of " << refinedObservations.getNumberOfSetBits() << "/" << refinedObservations.size() << " observations.");
                    for (auto const& obs : refinedObservations) {
                        // Increment the resolution at the refined observations.
                        // Detect overflows properly.
                        storm::RationalNumber newObsResolutionAsRational = storm::utility::convertNumber<storm::RationalNumber>(observationResolutionVector[obs]) * storm::utility::convertNumber<storm::RationalNumber>(options.resolutionFactor);
                        if (newObsResolutionAsRational > storm::utility::convertNumber<storm::RationalNumber>(std::numeric_limits<uint64_t>::max())) {
                            observationResolutionVector[obs] = std::numeric_limits<uint64_t>::max();
                        } else {
                            observationResolutionVector[obs] = storm::utility::convertNumber<uint64_t>(newObsResolutionAsRational);
                        }
                    }
                    overApproximation->restartExploration();
                }
                statistics.overApproximationMaxResolution = *std::max_element(observationResolutionVector.begin(), observationResolutionVector.end());
                
                // Start exploration
                storm::utility::Stopwatch explorationTime;
                if (options.explorationTimeLimit) {
                    explorationTime.start();
                }
                bool timeLimitExceeded = false;
                std::map<uint32_t, typename ExplorerType::SuccessorObservationInformation> gatheredSuccessorObservations; // Declare here to avoid reallocations
                uint64_t numRewiredOrExploredStates = 0;
                while (overApproximation->hasUnexploredState()) {
                    if (!timeLimitExceeded && options.explorationTimeLimit && static_cast<uint64_t>(explorationTime.getTimeInSeconds()) > options.explorationTimeLimit.get()) {
                        STORM_LOG_INFO("Exploration time limit exceeded.");
                        timeLimitExceeded = true;
                        fixPoint = false;
                    }

                    uint64_t currId = overApproximation->exploreNextState();
                    bool hasOldBehavior = refine && overApproximation->currentStateHasOldBehavior();
                    if (!hasOldBehavior) {
                        fixPoint = false; // Exploring a new state!
                    }
                    uint32_t currObservation = beliefManager->getBeliefObservation(currId);
                    if (targetObservations.count(currObservation) != 0) {
                        overApproximation->setCurrentStateIsTarget();
                        overApproximation->addSelfloopTransition();
                    } else {
                        // We need to decide how to treat this state (and each individual enabled action). There are the following cases:
                        // 1 The state has no old behavior and
                        //   1.1 we explore all actions or
                        //   1.2 we truncate all actions
                        // 2 The state has old behavior and was truncated in the last iteration and
                        //   2.1 we explore all actions or
                        //   2.2 we truncate all actions (essentially restoring old behavior, but we do the truncation step again to benefit from updated bounds)
                        // 3 The state has old behavior and was not truncated in the last iteration and the current action
                        //   3.1 should be rewired or
                        //   3.2 should get the old behavior but either
                        //       3.2.1 none of the successor observation has been refined since the last rewiring or exploration of this action
                        //       3.2.2 rewiring is only delayed as it could still have an effect in a later refinement step
                        
                        // Find out in which case we are
                        bool exploreAllActions = false;
                        bool truncateAllActions = false;
                        bool restoreAllActions = false;
                        bool checkRewireForAllActions = false;
                        ValueType gap = storm::utility::abs<ValueType>(overApproximation->getUpperValueBoundAtCurrentState() - overApproximation->getLowerValueBoundAtCurrentState());
                        // Get the relative gap
                        gap = gap * storm::utility::convertNumber<ValueType, uint64_t>(2) / (storm::utility::abs<ValueType>(overApproximation->getLowerValueBoundAtCurrentState()) + storm::utility::abs<ValueType>(overApproximation->getUpperValueBoundAtCurrentState()));
                        if (!hasOldBehavior) {
                            // Case 1
                            // If we explore this state and if it has no old behavior, it is clear that an "old" optimal scheduler can be extended to a scheduler that reaches this state
                            if (!timeLimitExceeded && gap > heuristicParameters.gapThreshold && numRewiredOrExploredStates < heuristicParameters.sizeThreshold) {
                                exploreAllActions = true; // Case 1.1
                            } else {
                                truncateAllActions = true; // Case 1.2
                                overApproximation->setCurrentStateIsTruncated();
                            }
                        } else {
                            if (overApproximation->getCurrentStateWasTruncated()) {
                                // Case 2
                                if (!timeLimitExceeded && overApproximation->currentStateIsOptimalSchedulerReachable() && gap > heuristicParameters.gapThreshold && numRewiredOrExploredStates < heuristicParameters.sizeThreshold) {
                                    exploreAllActions = true; // Case 2.1
                                    fixPoint = false;
                                } else {
                                    truncateAllActions = true; // Case 2.2
                                    overApproximation->setCurrentStateIsTruncated();
                                    if (fixPoint) {
                                        // Properly check whether this can still be a fixpoint
                                        if (overApproximation->currentStateIsOptimalSchedulerReachable() && !storm::utility::isZero(gap)) {
                                            fixPoint = false;
                                        }
                                        //} else {
                                            // In this case we truncated a state that is not reachable under optimal schedulers.
                                            // If no other state is explored (i.e. fixPoint remaints true), these states should still not be reachable in subsequent iterations
                                    }
                                }
                            } else {
                                // Case 3
                                // The decision for rewiring also depends on the corresponding action, but we have some criteria that lead to case 3.2 (independent of the action)
                                if (!timeLimitExceeded && overApproximation->currentStateIsOptimalSchedulerReachable() && gap > heuristicParameters.gapThreshold && numRewiredOrExploredStates < heuristicParameters.sizeThreshold) {
                                    checkRewireForAllActions = true; // Case 3.1 or Case 3.2
                                } else {
                                    restoreAllActions = true; // Definitely Case 3.2
                                    // We still need to check for each action whether rewiring makes sense later
                                    checkRewireForAllActions = true;
                                }
                            }
                        }
                        bool expandedAtLeastOneAction = false;
                        for (uint64 action = 0, numActions = beliefManager->getBeliefNumberOfChoices(currId); action < numActions; ++action) {
                            bool expandCurrentAction = exploreAllActions || truncateAllActions;
                            if (checkRewireForAllActions) {
                                assert(refine);
                                // In this case, we still need to check whether this action needs to be expanded
                                assert(!expandCurrentAction);
                                // Check the action dependent conditions for rewiring
                                // First, check whether this action has been rewired since the last refinement of one of the successor observations (i.e. whether rewiring would actually change the successor states)
                                assert(overApproximation->currentStateHasOldBehavior());
                                if (overApproximation->getCurrentStateActionExplorationWasDelayed(action) || overApproximation->currentStateHasSuccessorObservationInObservationSet(action, refinedObservations)) {
                                    // Then, check whether the other criteria for rewiring are satisfied
                                    if (!restoreAllActions && overApproximation->actionAtCurrentStateWasOptimal(action)) {
                                        // Do the rewiring now! (Case 3.1)
                                        expandCurrentAction = true;
                                        fixPoint = false;
                                    } else {
                                        // Delay the rewiring (Case 3.2.2)
                                        overApproximation->setCurrentChoiceIsDelayed(action);
                                        if (fixPoint) {
                                            // Check whether this delay means that a fixpoint has not been reached
                                            if (!overApproximation->getCurrentStateActionExplorationWasDelayed(action) || (overApproximation->currentStateIsOptimalSchedulerReachable() && overApproximation->actionAtCurrentStateWasOptimal(action) && !storm::utility::isZero(gap))) {
                                                fixPoint = false;
                                            }
                                        }
                                    }
                                } // else { Case 3.2.1 }
                            }
                            
                            if (expandCurrentAction) {
                                expandedAtLeastOneAction = true;
                                if (!truncateAllActions) {
                                    // Cases 1.1, 2.1, or 3.1
                                    auto successorGridPoints = beliefManager->expandAndTriangulate(currId, action, observationResolutionVector);
                                    for (auto const& successor : successorGridPoints) {
                                        overApproximation->addTransitionToBelief(action, successor.first, successor.second, false);
                                    }
                                    if (computeRewards) {
                                        overApproximation->computeRewardAtCurrentState(action);
                                    }
                                } else {
                                    // Cases 1.2 or 2.2
                                    ValueType truncationProbability = storm::utility::zero<ValueType>();
                                    ValueType truncationValueBound = storm::utility::zero<ValueType>();
                                    auto successorGridPoints = beliefManager->expandAndTriangulate(currId, action, observationResolutionVector);
                                    for (auto const& successor : successorGridPoints) {
                                        bool added = overApproximation->addTransitionToBelief(action, successor.first, successor.second, true);
                                        if (!added) {
                                            // We did not explore this successor state. Get a bound on the "missing" value
                                            truncationProbability += successor.second;
                                            truncationValueBound += successor.second * (min ? overApproximation->computeLowerValueBoundAtBelief(successor.first) : overApproximation->computeUpperValueBoundAtBelief(successor.first));
                                        }
                                    }
                                    if (computeRewards) {
                                        // The truncationValueBound will be added on top of the reward introduced by the current belief state.
                                        overApproximation->addTransitionsToExtraStates(action, truncationProbability);
                                        overApproximation->computeRewardAtCurrentState(action, truncationValueBound);
                                    } else {
                                        overApproximation->addTransitionsToExtraStates(action, truncationValueBound, truncationProbability - truncationValueBound);
                                    }
                                }
                            } else {
                                // Case 3.2
                                overApproximation->restoreOldBehaviorAtCurrentState(action);
                            }
                        }
                        if (expandedAtLeastOneAction) {
                            ++numRewiredOrExploredStates;
                        }
                    }
                    
                    if (storm::utility::resources::isTerminate()) {
                        break;
                    }
                }
                
                if (storm::utility::resources::isTerminate()) {
                    // don't overwrite statistics of a previous, successful computation
                    if (!statistics.overApproximationStates) {
                        statistics.overApproximationBuildAborted = true;
                        statistics.overApproximationStates = overApproximation->getCurrentNumberOfMdpStates();
                    }
                    statistics.overApproximationBuildTime.stop();
                    return false;
                }
                
                overApproximation->finishExploration();
                statistics.overApproximationBuildTime.stop();
                
                statistics.overApproximationCheckTime.start();
                overApproximation->computeValuesOfExploredMdp(min ? storm::solver::OptimizationDirection::Minimize : storm::solver::OptimizationDirection::Maximize);
                statistics.overApproximationCheckTime.stop();
                
                // don't overwrite statistics of a previous, successful computation
                if (!storm::utility::resources::isTerminate() || !statistics.overApproximationStates) {
                    statistics.overApproximationStates = overApproximation->getExploredMdp()->getNumberOfStates();
                }
                return fixPoint;
            }

            template<typename PomdpModelType, typename BeliefValueType>
            bool ApproximatePOMDPModelchecker<PomdpModelType, BeliefValueType>::buildUnderApproximation(std::set<uint32_t> const &targetObservations, bool min, bool computeRewards, bool refine, HeuristicParameters const& heuristicParameters, std::shared_ptr<BeliefManagerType>& beliefManager, std::shared_ptr<ExplorerType>& underApproximation) {
                statistics.underApproximationBuildTime.start();
                bool fixPoint = true;
                if (heuristicParameters.sizeThreshold != std::numeric_limits<uint64_t>::max()) {
                    statistics.underApproximationStateLimit = heuristicParameters.sizeThreshold;
                }
                if (!refine) {
                    // Build a new under approximation
                    if (computeRewards) {
                        underApproximation->startNewExploration(storm::utility::zero<ValueType>());
                    } else {
                        underApproximation->startNewExploration(storm::utility::one<ValueType>(), storm::utility::zero<ValueType>());
                    }
                } else {
                    // Restart the building process
                    underApproximation->restartExploration();
                }
                
                // Expand the beliefs
                storm::utility::Stopwatch explorationTime;
                if (options.explorationTimeLimit) {
                    explorationTime.start();
                }
                bool timeLimitExceeded = false;
                while (underApproximation->hasUnexploredState()) {
                    if (!timeLimitExceeded && options.explorationTimeLimit && static_cast<uint64_t>(explorationTime.getTimeInSeconds()) > options.explorationTimeLimit.get()) {
                        STORM_LOG_INFO("Exploration time limit exceeded.");
                        timeLimitExceeded = true;
                    }
                    uint64_t currId = underApproximation->exploreNextState();
                    
                    uint32_t currObservation = beliefManager->getBeliefObservation(currId);
                    bool stateAlreadyExplored = refine && underApproximation->currentStateHasOldBehavior() && !underApproximation->getCurrentStateWasTruncated();
                    if (!stateAlreadyExplored || timeLimitExceeded) {
                        fixPoint = false;
                    }
                    if (targetObservations.count(currObservation) != 0) {
                        underApproximation->setCurrentStateIsTarget();
                        underApproximation->addSelfloopTransition();
                    } else {
                        bool stopExploration = false;
                        if (timeLimitExceeded) {
                            stopExploration = true;
                            underApproximation->setCurrentStateIsTruncated();
                        } else if (!stateAlreadyExplored) {
                            // Check whether we want to explore the state now!
                            ValueType gap = storm::utility::abs<ValueType>(underApproximation->getUpperValueBoundAtCurrentState() - underApproximation->getLowerValueBoundAtCurrentState());
                            // Get the relative gap
                            gap = gap * storm::utility::convertNumber<ValueType, uint64_t>(2) / (storm::utility::abs<ValueType>(underApproximation->getLowerValueBoundAtCurrentState()) + storm::utility::abs<ValueType>(underApproximation->getUpperValueBoundAtCurrentState()));
                            if (gap < heuristicParameters.gapThreshold) {
                                stopExploration = true;
                                underApproximation->setCurrentStateIsTruncated();
                            } else if (underApproximation->getCurrentNumberOfMdpStates() >= heuristicParameters.sizeThreshold) {
                                stopExploration = true;
                                underApproximation->setCurrentStateIsTruncated();
                            }
                        }
                        for (uint64 action = 0, numActions = beliefManager->getBeliefNumberOfChoices(currId); action < numActions; ++action) {
                            // Always restore old behavior if available
                            if (stateAlreadyExplored) {
                                underApproximation->restoreOldBehaviorAtCurrentState(action);
                            } else {
                                ValueType truncationProbability = storm::utility::zero<ValueType>();
                                ValueType truncationValueBound = storm::utility::zero<ValueType>();
                                auto successors = beliefManager->expand(currId, action);
                                for (auto const& successor : successors) {
                                    bool added = underApproximation->addTransitionToBelief(action, successor.first, successor.second, stopExploration);
                                    if (!added) {
                                        STORM_LOG_ASSERT(stopExploration, "Didn't add a transition although exploration shouldn't be stopped.");
                                        // We did not explore this successor state. Get a bound on the "missing" value
                                        truncationProbability += successor.second;
                                        // Some care has to be taken here: Essentially, we are triangulating a value for the under-approximation out of other
                                        // under-approximation values. In general, this does not yield a sound underapproximation anymore.
                                        // However, in our case this is still the case as the under-approximation values are based on a memoryless scheduler.
                                        truncationValueBound += successor.second * (min ? underApproximation->computeUpperValueBoundAtBelief(successor.first) : underApproximation->computeLowerValueBoundAtBelief(successor.first));
                                    }
                                }
                                if (stopExploration) {
                                    if (computeRewards) {
                                        underApproximation->addTransitionsToExtraStates(action, truncationProbability);
                                    } else {
                                        underApproximation->addTransitionsToExtraStates(action, truncationValueBound, truncationProbability - truncationValueBound);
                                    }
                                }
                                if (computeRewards) {
                                    // The truncationValueBound will be added on top of the reward introduced by the current belief state.
                                    underApproximation->computeRewardAtCurrentState(action, truncationValueBound);
                                }
                            }
                        }
                    }
                    if (storm::utility::resources::isTerminate()) {
                        break;
                    }
                }
                
                if (storm::utility::resources::isTerminate()) {
                    // don't overwrite statistics of a previous, successful computation
                    if (!statistics.underApproximationStates) {
                        statistics.underApproximationBuildAborted = true;
                        statistics.underApproximationStates = underApproximation->getCurrentNumberOfMdpStates();
                    }
                    statistics.underApproximationBuildTime.stop();
                    return false;
                }
                
                underApproximation->finishExploration();
                statistics.underApproximationBuildTime.stop();

                statistics.underApproximationCheckTime.start();
                underApproximation->computeValuesOfExploredMdp(min ? storm::solver::OptimizationDirection::Minimize : storm::solver::OptimizationDirection::Maximize);
                statistics.underApproximationCheckTime.stop();
                
                // don't overwrite statistics of a previous, successful computation
                if (!storm::utility::resources::isTerminate() || !statistics.underApproximationStates) {
                    statistics.underApproximationStates = underApproximation->getExploredMdp()->getNumberOfStates();
                }
                return fixPoint;

            }

            template class ApproximatePOMDPModelchecker<storm::models::sparse::Pomdp<double>>;
            template class ApproximatePOMDPModelchecker<storm::models::sparse::Pomdp<storm::RationalNumber>>;

        }
    }
}
