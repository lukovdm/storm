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

   private:
    ConditionalAlgorithmSetting algorithm;
    storm::RationalNumber tolerance;
};

}  // namespace storm
