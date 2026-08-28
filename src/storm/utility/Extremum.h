#pragma once

#include <limits>
#include <optional>
#include <type_traits>

#include "storm/solver/OptimizationDirection.h"
#include "storm/utility/ExtendedNumber.h"
#include "storm/utility/NumberTraits.h"

namespace storm::utility {

/*!
 * Stores and manages an extremal (maximal or minimal) value
 */
template<storm::OptimizationDirection Dir, typename ValueType>
class Extremum {
   public:
    Extremum() = default;
    Extremum(ValueType const& value);
    Extremum(ValueType&& value);
    Extremum(Extremum const&) = default;
    Extremum(Extremum&&) = default;
    Extremum& operator=(Extremum const&) = default;
    Extremum& operator=(Extremum&&) = default;
    ~Extremum() = default;

    /*!
     * Sets the extremum to the given value
     * @return a reference to this
     */
    Extremum& operator=(ValueType const& value);

    /*!
     * Sets the extremum to the given value
     * @return a reference to this
     */
    Extremum& operator=(ValueType&& value);

    /*!
     * @param value
     * @return True if the provided value is strictly better (larger if we maximize; smaller if we minimize) than the stored value
     */
    bool better(ValueType const& value) const;

    /*!
     * Updates the stored value, if the given extremal value is better.
     * @param other
     * @return true if the extremum value of this changed
     */
    bool operator&=(Extremum const& other);

    /*!
     * Updates the stored value, if the given extremal value is better.
     * @param other
     * @return true if the extremum value of this changed
     */
    bool operator&=(Extremum&& other);

    /*!
     * Updates the stored value, if the given value is better.
     * @param other
     * @return true if the extremum value of this changed
     */
    bool operator&=(ValueType const& value);

    /*!
     * Updates the stored value, if the given value is better.
     * @param other
     * @return true if the extremum value of this changed
     */
    bool operator&=(ValueType&& value);

    /*!
     * @return true if nothing better than the base value has been contributed. Note that this is not a statement about
     * having a value: the extremum over an empty set has one, namely the infinity this reports on. A value type that
     * brings its own infinity has always worked this way; every other one now does too.
     */
    bool empty() const;

    /*!
     * @pre the extremal value is finite, which an extremum over an empty set is not
     * @return the stored extremal value. Use getExtendedValue instead where the value may be infinite: a value type
     * that has no infinity of its own has nothing for this to return in that case.
     */
    ValueType const& operator*() const;

    /*!
     * @pre the extremal value is finite, which an extremum over an empty set is not
     * @return the stored extremal value
     */
    ValueType& operator*();

    /*!
     * @return the stored extremal value as an optional. Returns std::nullopt if this is empty
     */
    std::optional<ValueType> getOptionalValue() const;

    /*!
     * @return the stored extremal value, including an infinite one. This is total: the extremum over an empty set is
     * +infinity if we minimize and -infinity if we maximize, which is what an extremum over an empty set is, so there
     * is no case for a caller to special-case. Prefer this over operator* and getOptionalValue wherever the value may
     * legitimately be infinite.
     */
    storm::utility::ExtendedValueType<ValueType> const& getExtendedValue() const;

    /*!
     * Updates the stored value, if the given value is better. This is the counterpart of operator&= for the callers
     * that hold an extended value, i.e. one that may be infinite. It only exists where that is a distinct type.
     * @return true if the extremum value of this changed
     */
    template<typename ExtendedType>
        requires(std::is_same_v<ExtendedType, storm::utility::ExtendedValueType<ValueType>> &&
                 !std::is_same_v<storm::utility::ExtendedValueType<ValueType>, ValueType>)
    bool operator&=(ExtendedType const& value) {
        if (betterThanStored(value)) {
            data.value = value;
            return true;
        }
        return false;
    }

    /*!
     * Forgets the extremal value so that this represents the extremum over an empty set.
     */
    void reset();

   private:
    /// The extremum is stored in the type that extends ValueType with the two infinities, which for a value type that
    /// brings its own infinity is ValueType itself. The extremum over an empty set is then not a special case but a
    /// value like any other: it is +infinity if we minimize and -infinity if we maximize, which is exactly what the
    /// extremum over an empty set is. Nothing has to remember whether anything was contributed.
    using StorageType = storm::utility::ExtendedValueType<ValueType>;

    /// True if ValueType brings its own infinity, in which case the storage is ValueType itself and this costs nothing.
    static bool const StoresPlainValues = std::is_same_v<StorageType, ValueType>;
    static_assert(!StoresPlainValues || std::numeric_limits<ValueType>::has_infinity, "NumberTraits claims an infinity that numeric_limits cannot provide.");

    /// @return the value an extremum over an empty set has.
    static StorageType baseValue() {
        if constexpr (StoresPlainValues) {
            // Taken from numeric_limits rather than from storm::utility so that it stays a compile time constant.
            if constexpr (storm::solver::minimize(Dir)) {
                return std::numeric_limits<ValueType>::infinity();
            } else {
                static_assert(storm::solver::maximize(Dir));
                return -std::numeric_limits<ValueType>::infinity();
            }
        } else {
            if constexpr (storm::solver::minimize(Dir)) {
                return StorageType::infinity();
            } else {
                static_assert(storm::solver::maximize(Dir));
                return StorageType::negativeInfinity();
            }
        }
    }

    /*!
     * @return true if the given stored value is strictly better than the one held here.
     */
    bool betterThanStored(StorageType const& value) const {
        if constexpr (storm::solver::minimize(Dir)) {
            return value < data.value;
        } else {
            static_assert(storm::solver::maximize(Dir));
            return value > data.value;
        }
    }

    struct Data {
        StorageType value{baseValue()};
    };

    /// Data
    Data data;
};

template<typename ValueType>
using Maximum = Extremum<storm::OptimizationDirection::Maximize, ValueType>;
template<typename ValueType>
using Minimum = Extremum<storm::OptimizationDirection::Minimize, ValueType>;

}  // namespace storm::utility