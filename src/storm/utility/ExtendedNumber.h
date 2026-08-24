#pragma once

#include <functional>
#include <limits>
#include <ostream>
#include <type_traits>
#include <utility>

#include "storm/exceptions/InvalidOperationException.h"
#include "storm/utility/NumberTraits.h"
#include "storm/utility/constants.h"
#include "storm/utility/macros.h"

namespace storm::utility {

/*!
 * A value of ValueType extended with the two infinities -infinity and +infinity.
 *
 * This is intended for value types that cannot represent infinity themselves, i.e., those for which
 * NumberTraits<ValueType>::HasInfinity is false. Types that do have their own infinity (such as double) should be used
 * directly instead; the ExtendedValueType alias below picks the right one.
 *
 * The arithmetic follows the usual conventions for the extended reals. The forms that are not defined there
 * (infinity - infinity, 0 * infinity, and infinity / infinity) throw an InvalidOperationException rather than yielding
 * a NaN, since the exact value types have no NaN to yield.
 *
 * ExtendedNumber is meant for the boundaries of a computation: check results, bounds that are handed around as scalars,
 * and values that are converted between value types. It is deliberately not meant to be used as the value type of a
 * transition matrix or of a solver, where the tag would be paid for in the innermost loops.
 *
 * The operators are hidden friends so that a finite ValueType converts implicitly on either side of them, which lets
 * code that mixes extended and plain values read the way it did before.
 */
template<typename ValueType>
class ExtendedNumber {
   public:
    enum class Kind { NegativeInfinity, Finite, PositiveInfinity };

    /*!
     * Creates a finite value that is zero.
     */
    ExtendedNumber() : kind(Kind::Finite), value(storm::utility::zero<ValueType>()) {
        // Intentionally left empty
    }

    /*!
     * Creates a finite value. This conversion is deliberately implicit so that finite values can be used where an
     * extended value is expected.
     */
    ExtendedNumber(ValueType const& value) : kind(Kind::Finite), value(value) {
        // Intentionally left empty
    }

    ExtendedNumber(ValueType&& value) : kind(Kind::Finite), value(std::move(value)) {
        // Intentionally left empty
    }

    ExtendedNumber(ExtendedNumber const&) = default;
    ExtendedNumber(ExtendedNumber&&) = default;
    ExtendedNumber& operator=(ExtendedNumber const&) = default;
    ExtendedNumber& operator=(ExtendedNumber&&) = default;
    ~ExtendedNumber() = default;

    /*!
     * @return +infinity
     */
    static ExtendedNumber infinity() {
        return ExtendedNumber(Kind::PositiveInfinity);
    }

    /*!
     * @return -infinity
     */
    static ExtendedNumber negativeInfinity() {
        return ExtendedNumber(Kind::NegativeInfinity);
    }

    /*!
     * @return true if this is neither +infinity nor -infinity
     */
    bool isFinite() const {
        return kind == Kind::Finite;
    }

    /*!
     * @return true if this is +infinity or -infinity
     */
    bool isInfinite() const {
        return kind != Kind::Finite;
    }

    /*!
     * @return true if this is +infinity
     */
    bool isPositiveInfinity() const {
        return kind == Kind::PositiveInfinity;
    }

    /*!
     * @return true if this is -infinity
     */
    bool isNegativeInfinity() const {
        return kind == Kind::NegativeInfinity;
    }

    /*!
     * @pre this is finite
     * @return the finite value
     */
    ValueType const& getFinite() const {
        STORM_LOG_ASSERT(isFinite(), "Tried to get the finite value of " << *this << ".");
        return value;
    }

    /*!
     * @pre this is finite
     * @return the finite value
     */
    ValueType& getFinite() {
        STORM_LOG_ASSERT(isFinite(), "Tried to get the finite value of " << *this << ".");
        return value;
    }

    friend bool operator==(ExtendedNumber const& first, ExtendedNumber const& second) {
        if (first.kind != second.kind) {
            return false;
        }
        return !first.isFinite() || first.value == second.value;
    }

    friend bool operator!=(ExtendedNumber const& first, ExtendedNumber const& second) {
        return !(first == second);
    }

    friend bool operator<(ExtendedNumber const& first, ExtendedNumber const& second) {
        if (first.kind != second.kind) {
            return first.kind < second.kind;
        }
        return first.isFinite() && first.value < second.value;
    }

    friend bool operator<=(ExtendedNumber const& first, ExtendedNumber const& second) {
        return !(second < first);
    }

    friend bool operator>(ExtendedNumber const& first, ExtendedNumber const& second) {
        return second < first;
    }

    friend bool operator>=(ExtendedNumber const& first, ExtendedNumber const& second) {
        return !(first < second);
    }

    ExtendedNumber operator-() const {
        switch (kind) {
            case Kind::PositiveInfinity:
                return negativeInfinity();
            case Kind::NegativeInfinity:
                return infinity();
            default:
                return ExtendedNumber(-value);
        }
    }

    ExtendedNumber const& operator+() const {
        return *this;
    }

    friend ExtendedNumber operator+(ExtendedNumber const& first, ExtendedNumber const& second) {
        if (first.isFinite() && second.isFinite()) {
            return ExtendedNumber(first.value + second.value);
        }
        if (first.isInfinite() && second.isInfinite()) {
            STORM_LOG_THROW(first.kind == second.kind, storm::exceptions::InvalidOperationException, "Tried to compute " << first << " + " << second << ".");
            return first;
        }
        return first.isInfinite() ? first : second;
    }

    friend ExtendedNumber operator-(ExtendedNumber const& first, ExtendedNumber const& second) {
        return first + (-second);
    }

    friend ExtendedNumber operator*(ExtendedNumber const& first, ExtendedNumber const& second) {
        if (first.isFinite() && second.isFinite()) {
            return ExtendedNumber(first.value * second.value);
        }
        // At least one operand is infinite, so the result is determined by the signs unless the other operand is zero.
        int const signs = first.sign() * second.sign();
        STORM_LOG_THROW(signs != 0, storm::exceptions::InvalidOperationException, "Tried to compute " << first << " * " << second << ".");
        return signs > 0 ? infinity() : negativeInfinity();
    }

    friend ExtendedNumber operator/(ExtendedNumber const& first, ExtendedNumber const& second) {
        STORM_LOG_THROW(!(first.isInfinite() && second.isInfinite()), storm::exceptions::InvalidOperationException,
                        "Tried to compute " << first << " / " << second << ".");
        if (second.isInfinite()) {
            // A finite value divided by an infinite one.
            return ExtendedNumber(storm::utility::zero<ValueType>());
        }
        int const divisorSign = second.sign();
        STORM_LOG_THROW(divisorSign != 0, storm::exceptions::InvalidOperationException, "Tried to compute " << first << " / " << second << ".");
        if (first.isFinite()) {
            return ExtendedNumber(first.value / second.value);
        }
        return (first.sign() * divisorSign) > 0 ? infinity() : negativeInfinity();
    }

    ExtendedNumber& operator+=(ExtendedNumber const& other) {
        return *this = *this + other;
    }

    ExtendedNumber& operator-=(ExtendedNumber const& other) {
        return *this = *this - other;
    }

    ExtendedNumber& operator*=(ExtendedNumber const& other) {
        return *this = *this * other;
    }

    ExtendedNumber& operator/=(ExtendedNumber const& other) {
        return *this = *this / other;
    }

    friend std::ostream& operator<<(std::ostream& out, ExtendedNumber const& number) {
        switch (number.kind) {
            case Kind::PositiveInfinity:
                out << "inf";
                break;
            case Kind::NegativeInfinity:
                out << "-inf";
                break;
            default:
                out << number.value;
                break;
        }
        return out;
    }

   private:
    explicit ExtendedNumber(Kind kind) : kind(kind), value(storm::utility::zero<ValueType>()) {
        // Intentionally left empty
    }

    /*!
     * @return 1 if this is positive, -1 if this is negative, and 0 if this is zero.
     */
    int sign() const {
        switch (kind) {
            case Kind::PositiveInfinity:
                return 1;
            case Kind::NegativeInfinity:
                return -1;
            default:
                if (storm::utility::isZero(value)) {
                    return 0;
                }
                return value < storm::utility::zero<ValueType>() ? -1 : 1;
        }
    }

    Kind kind;

    /// The finite value. Only meaningful if kind is Finite; it is kept at zero otherwise so that copies and comparisons
    /// of infinite values do not depend on leftover data.
    ValueType value;
};

/*!
 * The type to use for values that may be infinite. For value types that have their own infinity, this is the value type
 * itself, so that in particular double keeps using its IEEE infinity and pays nothing for this.
 */
template<typename ValueType>
using ExtendedValueType = std::conditional_t<storm::NumberTraits<ValueType>::HasInfinity, ValueType, ExtendedNumber<ValueType>>;

namespace detail {
template<typename T>
struct IsExtendedNumber : std::false_type {};

template<typename T>
struct IsExtendedNumber<ExtendedNumber<T>> : std::true_type {};

/// The finite value type underlying a (possibly extended) type.
template<typename T>
struct FiniteValueType {
    typedef T type;
};

template<typename T>
struct FiniteValueType<ExtendedNumber<T>> {
    typedef T type;
};
}  // namespace detail

/*!
 * The finite value type underlying a (possibly extended) type. This is what a solver or a matrix operates on once the
 * infinite entries have been split off.
 */
template<typename ValueType>
using FiniteValueType = typename detail::FiniteValueType<ValueType>::type;

/*!
 * @return +infinity, expressed in whichever type is used to extend ValueType with infinities.
 */
template<typename ValueType>
ExtendedValueType<ValueType> positiveInfinity() {
    if constexpr (detail::IsExtendedNumber<ExtendedValueType<ValueType>>::value) {
        return ExtendedValueType<ValueType>::infinity();
    } else {
        return storm::utility::infinity<ValueType>();
    }
}

/*!
 * @return -infinity, expressed in whichever type is used to extend ValueType with infinities.
 */
template<typename ValueType>
ExtendedValueType<ValueType> negativeInfinity() {
    if constexpr (detail::IsExtendedNumber<ExtendedValueType<ValueType>>::value) {
        return ExtendedValueType<ValueType>::negativeInfinity();
    } else {
        return -storm::utility::infinity<ValueType>();
    }
}

/*!
 * Converts a possibly infinite value to another type, which may itself be an ExtendedNumber or a type that has its own
 * infinity. This is what lets an infinite value cross value types without every call site restating what infinity means
 * in the source and in the target type.
 */
template<typename TargetType, typename SourceType>
TargetType convertNumber(ExtendedNumber<SourceType> const& number) {
    if (number.isFinite()) {
        return TargetType(convertNumber<FiniteValueType<TargetType>, SourceType>(number.getFinite()));
    }
    if constexpr (detail::IsExtendedNumber<TargetType>::value) {
        return number.isPositiveInfinity() ? TargetType::infinity() : TargetType::negativeInfinity();
    } else {
        static_assert(storm::NumberTraits<TargetType>::HasInfinity, "Tried to convert an infinite value to a target type that cannot represent infinity.");
        return number.isPositiveInfinity() ? storm::utility::infinity<TargetType>() : -storm::utility::infinity<TargetType>();
    }
}

/*!
 * Converts a finite value into an extended one. Widening a value never has to look at what it holds, so this is just the
 * underlying conversion followed by the implicit constructor.
 */
template<typename TargetType, typename SourceType>
    requires(detail::IsExtendedNumber<TargetType>::value && !detail::IsExtendedNumber<SourceType>::value)
TargetType convertNumber(SourceType const& number) {
    return TargetType(convertNumber<FiniteValueType<TargetType>, SourceType>(number));
}

}  // namespace storm::utility

namespace storm {
/*!
 * An extended number is exactly as exact as what it extends, and it is the type that does have infinity -- that is the
 * whole point of it.
 */
template<typename ValueType>
struct NumberTraits<storm::utility::ExtendedNumber<ValueType>> {
    static const bool SupportsExponential = NumberTraits<ValueType>::SupportsExponential;
    static const bool IsExact = NumberTraits<ValueType>::IsExact;
    static const bool HasInfinity = true;
};
}  // namespace storm

namespace std {
template<typename ValueType>
struct hash<storm::utility::ExtendedNumber<ValueType>> {
    size_t operator()(storm::utility::ExtendedNumber<ValueType> const& number) const {
        // The infinities have no payload to hash, so they get an arbitrary fixed value each.
        if (number.isPositiveInfinity()) {
            return 0x9e3779b9;
        }
        if (number.isNegativeInfinity()) {
            return 0x85ebca6b;
        }
        return std::hash<ValueType>()(number.getFinite());
    }
};

/*!
 * Reports the infinity that ExtendedNumber adds, so that the generic storm::utility::infinity and
 * storm::utility::isInfinity work on it without a special case. The remaining traits are inherited from the underlying
 * type where they still make sense.
 */
template<typename ValueType>
struct numeric_limits<storm::utility::ExtendedNumber<ValueType>> {
    typedef storm::utility::ExtendedNumber<ValueType> type;

    static constexpr bool is_specialized = true;
    static constexpr bool is_signed = true;
    static constexpr bool is_integer = false;
    static constexpr bool is_exact = storm::NumberTraits<ValueType>::IsExact;
    static constexpr bool has_infinity = true;
    static constexpr bool has_quiet_NaN = false;
    static constexpr bool has_signaling_NaN = false;
    static constexpr bool is_iec559 = false;
    static constexpr bool is_bounded = false;

    static type infinity() {
        return type::infinity();
    }

    static type lowest() {
        return type::negativeInfinity();
    }

    static type min() {
        return type(std::numeric_limits<ValueType>::min());
    }

    static type max() {
        return type::infinity();
    }

    static type epsilon() {
        return type(std::numeric_limits<ValueType>::epsilon());
    }
};
}  // namespace std
