#include "storm-config.h"
#include "test/storm_gtest.h"

#include "storm-parsers/api/properties.h"
#include "storm-parsers/parser/PrismParser.h"
#include "storm/api/builder.h"
#include "storm/api/properties.h"
#include "storm/environment/dd/DdEnvironment.h"
#include "storm/environment/dd/SylvanDdManagerEnvironment.h"
#include "storm/environment/modelchecker/ConditionalModelCheckerEnvironment.h"
#include "storm/environment/modelchecker/ModelCheckerEnvironment.h"
#include "storm/modelchecker/prctl/SparseMdpPrctlModelChecker.h"
#include "storm/modelchecker/prctl/SymbolicMdpPrctlModelChecker.h"
#include "storm/modelchecker/results/ExplicitQuantitativeCheckResult.h"
#include "storm/modelchecker/results/SymbolicQualitativeCheckResult.h"
#include "storm/modelchecker/results/SymbolicQuantitativeCheckResult.h"
#include "storm/models/sparse/Mdp.h"
#include "storm/models/symbolic/Mdp.h"

namespace {

template<storm::dd::DdType Type, typename VT, storm::ConditionalAlgorithmSetting Algorithm>
class SymbolicConfig {
   public:
    static const storm::dd::DdType ddType = Type;
    typedef VT ValueType;
    static const bool isExact = std::is_same_v<VT, storm::RationalNumber>;
    static storm::Environment createEnvironment() {
        storm::Environment env;
        env.modelchecker().conditional().setAlgorithm(Algorithm);
        if constexpr (!isExact) {
            env.modelchecker().conditional().setPrecision(storm::utility::convertNumber<storm::RationalNumber>(1e-8), false);
        }
        env.dd().sylvan().setNumberOfThreads(1);
        return env;
    }
};

using CuddBisection = SymbolicConfig<storm::dd::DdType::CUDD, double, storm::ConditionalAlgorithmSetting::Bisection>;
using CuddBisectionAdvanced = SymbolicConfig<storm::dd::DdType::CUDD, double, storm::ConditionalAlgorithmSetting::BisectionAdvanced>;
using SylvanBisection = SymbolicConfig<storm::dd::DdType::Sylvan, double, storm::ConditionalAlgorithmSetting::Bisection>;
using SylvanRationalBisection = SymbolicConfig<storm::dd::DdType::Sylvan, storm::RationalNumber, storm::ConditionalAlgorithmSetting::Bisection>;
using SylvanRationalBisectionAdvanced = SymbolicConfig<storm::dd::DdType::Sylvan, storm::RationalNumber, storm::ConditionalAlgorithmSetting::BisectionAdvanced>;

template<typename TestType>
class SymbolicConditionalMdpPrctlModelCheckerTest : public ::testing::Test {
   public:
    typedef typename TestType::ValueType ValueType;
    static const storm::dd::DdType DdType = TestType::ddType;
    typedef storm::models::symbolic::Mdp<DdType, ValueType> SymbolicModelType;

    SymbolicConditionalMdpPrctlModelCheckerTest() : _environment(TestType::createEnvironment()) {}

    storm::Environment const& env() const {
        return _environment;
    }
    ValueType parseNumber(std::string const& input) const {
        return storm::utility::convertNumber<ValueType>(input);
    }
    ValueType precision() const {
        return TestType::isExact ? parseNumber("0") : parseNumber("1e-6");
    }

    std::pair<std::shared_ptr<SymbolicModelType>, std::vector<std::shared_ptr<storm::logic::Formula const>>> buildModelFormulas(
        storm::prism::Program const& inputProgram, std::string const& formulasAsString) const {
        std::pair<std::shared_ptr<SymbolicModelType>, std::vector<std::shared_ptr<storm::logic::Formula const>>> result;
        auto program = inputProgram.substituteConstantsFormulas();
        result.second = storm::api::extractFormulasFromProperties(storm::api::parsePropertiesForPrismProgram(formulasAsString, program));
        result.first = storm::api::buildSymbolicModel<DdType, ValueType>(env(), program, result.second)->template as<SymbolicModelType>();
        return result;
    }

    /// Checks the formula and returns the value at the (single) initial state.
    ValueType check(SymbolicModelType const& model, storm::logic::Formula const& formula) const {
        storm::modelchecker::SymbolicMdpPrctlModelChecker<SymbolicModelType> checker(model);
        auto result = checker.check(env(), storm::modelchecker::CheckTask<storm::logic::Formula, ValueType>(formula, true));
        auto const& values = result->template asSymbolicQuantitativeCheckResult<DdType, ValueType>().getValueVector();
        return (values * model.getInitialStates().template toAdd<ValueType>()).sumAbstract(model.getRowVariables()).getMax();
    }

    /// Checks the (bounded) formula and returns its truth value at the (single) initial state.
    bool checkQualitative(SymbolicModelType const& model, storm::logic::Formula const& formula) const {
        storm::modelchecker::SymbolicMdpPrctlModelChecker<SymbolicModelType> checker(model);
        auto result = checker.check(env(), storm::modelchecker::CheckTask<storm::logic::Formula, ValueType>(formula, true));
        return !(result->template asSymbolicQualitativeCheckResult<DdType>().getTruthValuesVector() && model.getInitialStates()).isZero();
    }

    /// Computes the reference value with the sparse engine.
    ValueType checkSparse(storm::prism::Program const& inputProgram, std::string const& formulaAsString) const {
        auto program = inputProgram.substituteConstantsFormulas();
        auto formulas = storm::api::extractFormulasFromProperties(storm::api::parsePropertiesForPrismProgram(formulaAsString, program));
        auto model = storm::api::buildSparseModel<ValueType>(program, formulas)->template as<storm::models::sparse::Mdp<ValueType>>();
        storm::modelchecker::SparseMdpPrctlModelChecker<storm::models::sparse::Mdp<ValueType>> checker(*model);
        auto result = checker.check(env(), storm::modelchecker::CheckTask<storm::logic::Formula, ValueType>(*formulas.front(), true));
        auto const& value = result->template asExplicitQuantitativeCheckResult<ValueType>().getValueVector()[*model->getInitialStates().begin()];
        if constexpr (std::is_same_v<std::decay_t<decltype(value)>, ValueType>) {
            return value;
        } else {
            return value.getFinite();
        }
    }

   private:
    storm::Environment _environment;
};

typedef ::testing::Types<CuddBisection, CuddBisectionAdvanced, SylvanBisection, SylvanRationalBisection, SylvanRationalBisectionAdvanced> TestingTypes;

TYPED_TEST_SUITE(SymbolicConditionalMdpPrctlModelCheckerTest, TestingTypes, );

TYPED_TEST(SymbolicConditionalMdpPrctlModelCheckerTest, two_dice) {
    std::string const formulasString =
        "Pmax=? [F \"twelve\" || F d1=6];"
        "Pmin=? [F \"twelve\" || F d1=6];"
        "Pmax=? [F \"seven\" || F d2=4];"
        "Pmin=? [F \"seven\" || F d2=4];"
        "Pmax=? [F \"two\" || F d2=2];"
        "Pmin=? [F \"two\" || F d2=2];"
        "Pmax=? [F d1=6 || F \"twelve\"];"
        "Pmin=? [F d1=6 || F \"twelve\"];";

    auto program = storm::parser::PrismParser::parse(STORM_TEST_RESOURCES_DIR "/mdp/two_dice.nm");
    auto [model, formulas] = this->buildModelFormulas(program, formulasString);
    EXPECT_EQ(169ul, model->getNumberOfStates());
    std::vector<std::string> const expected = {"1/6", "1/6", "1/6", "1/6", "0", "0", "1", "1"};
    for (uint64_t i = 0; i < expected.size(); ++i) {
        EXPECT_NEAR(this->parseNumber(expected[i]), this->check(*model, *formulas[i]), this->precision()) << formulas[i]->toString();
    }
}

TYPED_TEST(SymbolicConditionalMdpPrctlModelCheckerTest, consensus) {
    std::string const formulasString =
        "Pmax=? [F \"all_coins_equal_0\" & \"finished\" || F \"agree\" & \"finished\"];"
        "Pmin=? [F \"all_coins_equal_0\" & \"finished\" || F \"agree\" & \"finished\"];"
        "Pmax=? [F \"all_coins_equal_1\" & \"finished\" || F \"agree\" & \"finished\"];"
        "Pmin=? [F \"all_coins_equal_1\" & \"finished\" || F \"agree\" & \"finished\"];"
        "P<=560/953 [F \"all_coins_equal_1\" & \"finished\" || F \"agree\" & \"finished\"];"
        "P<562/953 [F \"all_coins_equal_1\" & \"finished\" || F \"agree\" & \"finished\"];"
        "P>393/953 [F \"all_coins_equal_1\" & \"finished\" || F \"agree\" & \"finished\"];"
        "P>=391/953 [F \"all_coins_equal_1\" & \"finished\" || F \"agree\" & \"finished\"];";

    auto program = storm::parser::PrismParser::parse(STORM_TEST_RESOURCES_DIR "/mdp/coin2-2.nm");
    auto [model, formulas] = this->buildModelFormulas(program, formulasString);
    EXPECT_EQ(272ul, model->getNumberOfStates());
    std::vector<std::string> const expected = {"561/953", "392/953", "561/953", "392/953"};
    for (uint64_t i = 0; i < expected.size(); ++i) {
        EXPECT_NEAR(this->parseNumber(expected[i]), this->check(*model, *formulas[i]), this->precision()) << formulas[i]->toString();
    }
    EXPECT_FALSE(this->checkQualitative(*model, *formulas[4]));
    EXPECT_TRUE(this->checkQualitative(*model, *formulas[5]));
    EXPECT_FALSE(this->checkQualitative(*model, *formulas[6]));
    EXPECT_TRUE(this->checkQualitative(*model, *formulas[7]));
}

TYPED_TEST(SymbolicConditionalMdpPrctlModelCheckerTest, simple) {
    std::string const programAsString = R"(mdp
module main
	x : [0..2];
	[] x=0 -> 0.7 : (x'=1) + 0.3 : (x'=2);
	[] x=1 -> 1 : (x'=1);
	[] x=2 -> 1 : (x'=1);
endmodule
)";
    auto program = storm::parser::PrismParser::parseFromString(programAsString, "<no filename>");
    auto [model, formulas] = this->buildModelFormulas(program, "Pmin=? [F x=2 || F x=1];Pmax=? [F x=2 || F x=1];");
    EXPECT_NEAR(this->parseNumber("3/10"), this->check(*model, *formulas[0]), this->precision());
    EXPECT_NEAR(this->parseNumber("3/10"), this->check(*model, *formulas[1]), this->precision());
}

TYPED_TEST(SymbolicConditionalMdpPrctlModelCheckerTest, end_components) {
    // A model with end components among the maybe states and an initial component with several states (x=0 and x=1 can both avoid the condition).
    std::string const programAsString = R"(mdp
module main
	x : [0..7];
	[a] x=0 -> 0.5 : (x'=1) + 0.5 : (x'=7);
	[b] x=0 -> 0.4 : (x'=2) + 0.6 : (x'=3);
	[a] x=1 -> 1 : (x'=0);
	[b] x=1 -> 0.3 : (x'=4) + 0.7 : (x'=2);
	[a] x=2 -> 1 : (x'=3);
	[b] x=2 -> 0.5 : (x'=5) + 0.5 : (x'=6);
	[a] x=3 -> 1 : (x'=2);
	[b] x=3 -> 0.2 : (x'=6) + 0.8 : (x'=4);
	[a] x=4 -> 1 : (x'=4);
	[a] x=5 -> 0.5 : (x'=5) + 0.5 : (x'=6);
	[b] x=5 -> 1 : (x'=4);
	[a] x=6 -> 1 : (x'=6);
	[a] x=7 -> 1 : (x'=7);
endmodule
)";
    auto program = storm::parser::PrismParser::parseFromString(programAsString, "<no filename>");
    std::vector<std::string> const formulas = {"Pmax=? [F x=6 || F x>=4 & x<=6]", "Pmin=? [F x=6 || F x>=4 & x<=6]", "Pmax=? [F x=4 || F x=5 | x=6]",
                                               "Pmin=? [F x=4 || F x=5 | x=6]",   "Pmax=? [F x=5 || F x=4 | x=6]",   "Pmin=? [F x=2 || F x=6]"};
    for (auto const& formula : formulas) {
        auto [model, parsed] = this->buildModelFormulas(program, formula);
        EXPECT_NEAR(this->checkSparse(program, formula), this->check(*model, *parsed.front()), this->precision()) << formula;
    }
}

}  // namespace
