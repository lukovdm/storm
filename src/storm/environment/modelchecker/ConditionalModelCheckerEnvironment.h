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

   private:
    ConditionalAlgorithmSetting algorithm;
};

}  // namespace storm
