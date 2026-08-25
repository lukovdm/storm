#pragma once

#include <memory>
#include <vector>
#include "storm/storage/Scheduler.h"

namespace storm {
namespace storage {
class BitVector;
}

namespace modelchecker {
namespace helper {
/*!
 * @tparam ValueType the value type of the model, and hence of any scheduler that is produced.
 * @tparam ValuesType the type of the computed values. A reward or a time can be infinite while the model's value type
 *         has no representation for that, so the two are not always the same.
 */
template<typename ValueType, typename ValuesType = ValueType>
struct MDPSparseModelCheckingHelperReturnType {
    MDPSparseModelCheckingHelperReturnType(MDPSparseModelCheckingHelperReturnType const&) = delete;
    MDPSparseModelCheckingHelperReturnType(MDPSparseModelCheckingHelperReturnType&&) = default;

    MDPSparseModelCheckingHelperReturnType(std::vector<ValuesType>&& values, std::unique_ptr<storm::storage::Scheduler<ValueType>>&& scheduler = nullptr)
        : values(std::move(values)), scheduler(std::move(scheduler)) {
        // Intentionally left empty.
    }

    virtual ~MDPSparseModelCheckingHelperReturnType() {
        // Intentionally left empty.
    }

    // The values computed for the states.
    std::vector<ValuesType> values;

    // A scheduler, if it was computed.
    std::unique_ptr<storm::storage::Scheduler<ValueType>> scheduler;
};
}  // namespace helper

}  // namespace modelchecker
}  // namespace storm
