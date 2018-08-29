#pragma once

#include <unordered_map>
#include <boost/optional.hpp>

#include "storm/logic/CloneVisitor.h"
#include "storm/logic/RewardAccumulation.h"
#include "storm/storage/jani/Model.h"
#include "storm/storage/jani/Property.h"

namespace storm {
    namespace logic {
        
        class RewardAccumulationEliminationVisitor : public CloneVisitor {
        public:
            RewardAccumulationEliminationVisitor(storm::jani::Model const& model);
            
            /*!
             * Eliminates any reward accumulations of the formula, where the presence of the reward accumulation does not change the result of the formula
             */
            std::shared_ptr<Formula> eliminateRewardAccumulations(Formula const& f) const;
            
            void eliminateRewardAccumulations(std::vector<storm::jani::Property>& properties) const;
           
            virtual boost::any visit(BoundedUntilFormula const& f, boost::any const& data) const override;
            virtual boost::any visit(CumulativeRewardFormula const& f, boost::any const& data) const override;
            virtual boost::any visit(EventuallyFormula const& f, boost::any const& data) const override;
            virtual boost::any visit(RewardOperatorFormula const& f, boost::any const& data) const override;
            virtual boost::any visit(TotalRewardFormula const& f, boost::any const& data) const override;

            
        private:
            bool canEliminate(storm::logic::RewardAccumulation const& accumulation, boost::optional<std::string> rewardModelName) const;
            
            storm::jani::Model const& model;
        };
        
    }
}
