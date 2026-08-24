#pragma once

#include "storm/modelchecker/results/CheckResult.h"
#include "storm/utility/ExtendedNumber.h"

namespace storm {
namespace modelchecker {
template<typename ValueType>
class QuantitativeCheckResult : public CheckResult {
   public:
    /// The type of the values held by this result. Rewards and expected times can be infinite, so this is ValueType
    /// itself only for the value types that can express that; the others are extended with the infinities.
    typedef storm::utility::ExtendedValueType<ValueType> extended_value_type;

    virtual ~QuantitativeCheckResult() = default;

    virtual std::unique_ptr<CheckResult> compareAgainstBound(storm::logic::ComparisonType comparisonType, ValueType const& bound) const;

    virtual void oneMinus() = 0;

    virtual extended_value_type getMin() const = 0;
    virtual extended_value_type getMax() const = 0;

    virtual extended_value_type average() const = 0;
    virtual extended_value_type sum() const = 0;

    virtual bool isQuantitative() const override;
};
}  // namespace modelchecker
}  // namespace storm
