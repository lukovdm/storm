#pragma once

#include "storm/adapters/RationalNumberAdapter.h"
#include "storm/modelchecker/helper/conditional/ConditionalAlgorithmSetting.h"

namespace storm {

class ConditionalModelCheckerEnvironment {
   public:
    ConditionalModelCheckerEnvironment();
    ~ConditionalModelCheckerEnvironment();

    ConditionalAlgorithmSetting getAlgorithm() const;
    void setAlgorithm(ConditionalAlgorithmSetting value);

    storm::RationalNumber getTolerance() const;
    void setTolerance(storm::RationalNumber const& value);

    bool isAllowOptimizationForBoundedPropertiesSet() const;
    void setAllowOptimizationForBoundedProperties(bool value);

   private:
    ConditionalAlgorithmSetting algorithm;
    storm::RationalNumber tolerance;
    bool allowOptimizationForBoundedProperties;
};

}  // namespace storm
