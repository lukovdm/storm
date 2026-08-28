#include "storm/utility/Extremum.h"

#include "storm/adapters/RationalNumberAdapter.h"
#include "storm/utility/ExtendedNumber.h"
#include "storm/utility/macros.h"

namespace storm::utility {

template<storm::OptimizationDirection Dir, typename ValueType>
Extremum<Dir, ValueType>::Extremum(ValueType const& value) : data({value}) {
    // Intentionally left empty
}

template<storm::OptimizationDirection Dir, typename ValueType>
Extremum<Dir, ValueType>::Extremum(ValueType&& value) : data({std::move(value)}) {
    // Intentionally left empty
}

template<storm::OptimizationDirection Dir, typename ValueType>
Extremum<Dir, ValueType>& Extremum<Dir, ValueType>::operator=(ValueType const& value) {
    data.value = value;
    return *this;
}

template<storm::OptimizationDirection Dir, typename ValueType>
Extremum<Dir, ValueType>& Extremum<Dir, ValueType>::operator=(ValueType&& value) {
    data.value = std::move(value);
    return *this;
}

template<storm::OptimizationDirection Dir, typename ValueType>
bool Extremum<Dir, ValueType>::better(ValueType const& value) const {
    // Comparing the plain value against the stored one directly. Wrapping it first would copy it, and for a value type
    // whose copy allocates that is a heap allocation per comparison.
    if constexpr (storm::solver::minimize(Dir)) {
        return value < data.value;
    } else {
        static_assert(storm::solver::maximize(Dir));
        return value > data.value;
    }
}

template<storm::OptimizationDirection Dir, typename ValueType>
bool Extremum<Dir, ValueType>::operator&=(Extremum const& other) {
    if (betterThanStored(other.data.value)) {
        data.value = other.data.value;
        return true;
    }
    return false;
}

template<storm::OptimizationDirection Dir, typename ValueType>
bool Extremum<Dir, ValueType>::operator&=(Extremum&& other) {
    if (betterThanStored(other.data.value)) {
        data.value = std::move(other.data.value);
        return true;
    }
    return false;
}

template<storm::OptimizationDirection Dir, typename ValueType>
bool Extremum<Dir, ValueType>::operator&=(ValueType const& value) {
    if (better(value)) {
        data.value = value;
        return true;
    }
    return false;
}

template<storm::OptimizationDirection Dir, typename ValueType>
bool Extremum<Dir, ValueType>::operator&=(ValueType&& value) {
    if (better(value)) {
        data.value = std::move(value);
        return true;
    }
    return false;
}

template<storm::OptimizationDirection Dir, typename ValueType>
bool Extremum<Dir, ValueType>::empty() const {
    if constexpr (StoresPlainValues) {
        return data.value == baseValue();
    } else {
        // Asking the kind rather than comparing against a freshly built base value, which would allocate.
        if constexpr (storm::solver::minimize(Dir)) {
            return data.value.isPositiveInfinity();
        } else {
            static_assert(storm::solver::maximize(Dir));
            return data.value.isNegativeInfinity();
        }
    }
}

template<storm::OptimizationDirection Dir, typename ValueType>
ValueType const& Extremum<Dir, ValueType>::operator*() const {
    if constexpr (StoresPlainValues) {
        return data.value;
    } else {
        // Throws if the value is infinite: there is no value of this type to hand out for it.
        return data.value.getFinite();
    }
}

template<storm::OptimizationDirection Dir, typename ValueType>
ValueType& Extremum<Dir, ValueType>::operator*() {
    if constexpr (StoresPlainValues) {
        return data.value;
    } else {
        return data.value.getFinite();
    }
}

template<storm::OptimizationDirection Dir, typename ValueType>
std::optional<ValueType> Extremum<Dir, ValueType>::getOptionalValue() const {
    if (empty()) {
        return {};
    }
    return **this;
}

template<storm::OptimizationDirection Dir, typename ValueType>
storm::utility::ExtendedValueType<ValueType> const& Extremum<Dir, ValueType>::getExtendedValue() const {
    return data.value;
}

template<storm::OptimizationDirection Dir, typename ValueType>
void Extremum<Dir, ValueType>::reset() {
    data.value = baseValue();
}

template class Extremum<storm::OptimizationDirection::Minimize, double>;
template class Extremum<storm::OptimizationDirection::Maximize, double>;

#if defined(STORM_HAVE_CLN)
template class Extremum<storm::OptimizationDirection::Minimize, storm::ClnRationalNumber>;
template class Extremum<storm::OptimizationDirection::Maximize, storm::ClnRationalNumber>;
#endif
#if defined(STORM_HAVE_GMP)
template class Extremum<storm::OptimizationDirection::Minimize, storm::GmpRationalNumber>;
template class Extremum<storm::OptimizationDirection::Maximize, storm::GmpRationalNumber>;
#endif

}  // namespace storm::utility