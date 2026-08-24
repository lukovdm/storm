#include "storm-config.h"
#include "test/storm_gtest.h"

#include "storm/adapters/RationalFunctionAdapter.h"
#include "storm/adapters/RationalNumberAdapter.h"
#include "storm/exceptions/InvalidOperationException.h"
#include "storm/utility/ExtendedNumber.h"
#include "storm/utility/constants.h"

namespace {
typedef storm::utility::ExtendedNumber<storm::RationalNumber> ExtendedRationalNumber;

storm::RationalNumber rational(double value) {
    return storm::utility::convertNumber<storm::RationalNumber>(value);
}
}  // namespace

TEST(ExtendedNumberTest, extendedValueTypeOnlyWrapsWhereNeeded) {
    // double has its own infinity, so it must not be wrapped: the floating point path pays nothing for this.
    EXPECT_TRUE((std::is_same_v<storm::utility::ExtendedValueType<double>, double>));
    EXPECT_TRUE((std::is_same_v<storm::utility::ExtendedValueType<storm::RationalNumber>, ExtendedRationalNumber>));
}

TEST(ExtendedNumberTest, kinds) {
    ExtendedRationalNumber const two(rational(2));
    EXPECT_TRUE(two.isFinite());
    EXPECT_FALSE(two.isInfinite());
    EXPECT_EQ(rational(2), two.getFinite());

    EXPECT_TRUE(ExtendedRationalNumber::infinity().isPositiveInfinity());
    EXPECT_TRUE(ExtendedRationalNumber::infinity().isInfinite());
    EXPECT_TRUE(ExtendedRationalNumber::negativeInfinity().isNegativeInfinity());
    EXPECT_FALSE(ExtendedRationalNumber::negativeInfinity().isPositiveInfinity());

    // The default value is a finite zero.
    EXPECT_TRUE(ExtendedRationalNumber().isFinite());
    EXPECT_TRUE(storm::utility::isZero(ExtendedRationalNumber().getFinite()));
}

TEST(ExtendedNumberTest, ordering) {
    ExtendedRationalNumber const inf = ExtendedRationalNumber::infinity();
    ExtendedRationalNumber const negInf = ExtendedRationalNumber::negativeInfinity();
    ExtendedRationalNumber const two(rational(2));

    EXPECT_LT(negInf, two);
    EXPECT_LT(two, inf);
    EXPECT_LT(negInf, inf);
    EXPECT_LE(inf, inf);
    EXPECT_GT(inf, two);

    // The previous representation of infinity was the literal 100000000000, so any larger value compared greater than
    // "infinity". Every finite value must now be below it.
    EXPECT_LT(ExtendedRationalNumber(rational(1e12)), inf);
    EXPECT_LT(ExtendedRationalNumber(rational(1e11)), inf);
}

TEST(ExtendedNumberTest, equality) {
    ExtendedRationalNumber const inf = ExtendedRationalNumber::infinity();
    EXPECT_EQ(inf, ExtendedRationalNumber::infinity());
    EXPECT_NE(inf, ExtendedRationalNumber::negativeInfinity());
    EXPECT_NE(inf, ExtendedRationalNumber(rational(1e11)));
    EXPECT_EQ(ExtendedRationalNumber(rational(2)), ExtendedRationalNumber(rational(2)));
}

TEST(ExtendedNumberTest, arithmeticIsAbsorbing) {
    ExtendedRationalNumber const inf = ExtendedRationalNumber::infinity();
    ExtendedRationalNumber const negInf = ExtendedRationalNumber::negativeInfinity();
    ExtendedRationalNumber const two(rational(2));
    ExtendedRationalNumber const zero;

    EXPECT_EQ(inf, inf + two);
    EXPECT_EQ(inf, two + inf);
    EXPECT_EQ(inf, inf - two);
    EXPECT_EQ(inf, inf + inf);
    EXPECT_EQ(inf, inf * two);
    EXPECT_EQ(negInf, inf * -two);
    EXPECT_EQ(inf, inf / two);
    EXPECT_EQ(negInf, inf / -two);
    EXPECT_EQ(zero, two / inf);
    EXPECT_EQ(negInf, -inf);
    EXPECT_EQ(inf, -negInf);

    EXPECT_EQ(ExtendedRationalNumber(rational(6)), ExtendedRationalNumber(rational(2)) * ExtendedRationalNumber(rational(3)));
}

TEST(ExtendedNumberTest, undefinedFormsThrow) {
    ExtendedRationalNumber const inf = ExtendedRationalNumber::infinity();
    ExtendedRationalNumber const negInf = ExtendedRationalNumber::negativeInfinity();
    ExtendedRationalNumber const zero;

    // The exact value types have no NaN, so these throw rather than producing a quiet junk value.
    STORM_SILENT_EXPECT_THROW(inf - inf, storm::exceptions::InvalidOperationException);
    STORM_SILENT_EXPECT_THROW(inf + negInf, storm::exceptions::InvalidOperationException);
    STORM_SILENT_EXPECT_THROW(inf * zero, storm::exceptions::InvalidOperationException);
    STORM_SILENT_EXPECT_THROW(inf / inf, storm::exceptions::InvalidOperationException);
    STORM_SILENT_EXPECT_THROW(inf / zero, storm::exceptions::InvalidOperationException);
}

TEST(ExtendedNumberTest, conversionAcrossValueTypes) {
    ExtendedRationalNumber const inf = ExtendedRationalNumber::infinity();
    ExtendedRationalNumber const negInf = ExtendedRationalNumber::negativeInfinity();

    // Into a type that has its own infinity.
    EXPECT_EQ(storm::utility::infinity<double>(), storm::utility::convertNumber<double>(inf));
    EXPECT_EQ(-storm::utility::infinity<double>(), storm::utility::convertNumber<double>(negInf));
    EXPECT_EQ(2.0, storm::utility::convertNumber<double>(ExtendedRationalNumber(rational(2))));

    // Into another extended type. This is what the parameter lifting model checker needs, where infinity has to travel
    // from the constant type to the coefficient type.
    auto const asFunction = storm::utility::convertNumber<storm::utility::ExtendedNumber<storm::RationalFunction>>(inf);
    EXPECT_TRUE(asFunction.isPositiveInfinity());
    auto const finiteAsFunction = storm::utility::convertNumber<storm::utility::ExtendedNumber<storm::RationalFunction>>(ExtendedRationalNumber(rational(2)));
    EXPECT_TRUE(finiteAsFunction.isFinite());
}

TEST(ExtendedNumberTest, output) {
    std::stringstream stream;
    stream << ExtendedRationalNumber::infinity() << " " << ExtendedRationalNumber::negativeInfinity() << " " << ExtendedRationalNumber(rational(2));
    EXPECT_EQ("inf -inf 2", stream.str());
}

TEST(ExtendedNumberTest, mixesWithFiniteValues) {
    ExtendedRationalNumber const inf = ExtendedRationalNumber::infinity();
    storm::RationalNumber const two = rational(2);

    // The operators are hidden friends, so a finite value converts on either side of them.
    EXPECT_TRUE(two < inf);
    EXPECT_TRUE(inf > two);
    EXPECT_EQ(ExtendedRationalNumber(rational(4)), two + ExtendedRationalNumber(two));
    EXPECT_EQ(ExtendedRationalNumber(rational(4)), ExtendedRationalNumber(two) + two);
    EXPECT_EQ(inf, two + inf);
    EXPECT_EQ(ExtendedRationalNumber(two), two);
}

TEST(ExtendedNumberTest, reportsItsInfinityToTheGenericUtilities) {
    // NumberTraits and numeric_limits both have to say that this type does have an infinity, otherwise the generic
    // storm::utility::infinity and isInfinity would not reach it.
    static_assert(storm::NumberTraits<ExtendedRationalNumber>::HasInfinity);
    static_assert(storm::NumberTraits<ExtendedRationalNumber>::IsExact == storm::NumberTraits<storm::RationalNumber>::IsExact);
    EXPECT_TRUE(std::numeric_limits<ExtendedRationalNumber>::has_infinity);
    EXPECT_EQ(ExtendedRationalNumber::infinity(), std::numeric_limits<ExtendedRationalNumber>::infinity());

    // Extending an already extended type is idempotent.
    EXPECT_TRUE((std::is_same_v<storm::utility::ExtendedValueType<ExtendedRationalNumber>, ExtendedRationalNumber>));
    EXPECT_TRUE((std::is_same_v<storm::utility::FiniteValueType<ExtendedRationalNumber>, storm::RationalNumber>));
    EXPECT_TRUE((std::is_same_v<storm::utility::FiniteValueType<double>, double>));
}

TEST(ExtendedNumberTest, conversionIntoAnExtendedType) {
    auto const widened = storm::utility::convertNumber<ExtendedRationalNumber>(2.0);
    EXPECT_TRUE(widened.isFinite());
    EXPECT_EQ(rational(2), widened.getFinite());
}

TEST(ExtendedNumberTest, infinityOfTheExtendedType) {
    EXPECT_EQ(ExtendedRationalNumber::infinity(), storm::utility::positiveInfinity<storm::RationalNumber>());
    EXPECT_EQ(ExtendedRationalNumber::negativeInfinity(), storm::utility::negativeInfinity<storm::RationalNumber>());

    // For a type that has its own infinity nothing is wrapped, so this stays the IEEE infinity.
    EXPECT_EQ(storm::utility::infinity<double>(), storm::utility::positiveInfinity<double>());
    EXPECT_EQ(-storm::utility::infinity<double>(), storm::utility::negativeInfinity<double>());
}
