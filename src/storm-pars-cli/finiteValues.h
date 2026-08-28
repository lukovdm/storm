#pragma once

#include <vector>

#include "storm/exceptions/NotSupportedException.h"
#include "storm/logic/Formula.h"
#include "storm/modelchecker/results/ExplicitQuantitativeCheckResult.h"
#include "storm/utility/ExtendedNumber.h"
#include "storm/utility/macros.h"

namespace storm::pars {

/*!
 * Narrows the values of a quantitative check result back to the plain value type, naming the state and the property
 * if one of them is infinite.
 *
 * @param result a check result holding one value per state
 * @param formula the property the result belongs to, for the error message
 * @return the values of the result, one per state
 */
template<typename ValueType>
std::vector<ValueType> getFiniteValues(storm::modelchecker::ExplicitQuantitativeCheckResult<ValueType> const& result, storm::logic::Formula const& formula) {
    auto const& values = result.getValueVector();
    for (uint64_t state = 0; state < values.size(); ++state) {
        STORM_LOG_THROW(storm::utility::isFinite(values[state]), storm::exceptions::NotSupportedException,
                        "Property '" << formula << "' has the value " << values[state] << " in state " << state
                                     << ". This analysis requires a finite value for every state.");
    }
    return result.getFiniteValueVector();
}

}  // namespace storm::pars
