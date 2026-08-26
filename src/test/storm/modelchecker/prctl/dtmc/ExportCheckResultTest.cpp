#include "storm-config.h"
#include "test/storm_gtest.h"

#include <filesystem>
#include <fstream>
#include <random>

#include "storm-parsers/api/properties.h"
#include "storm-parsers/parser/PrismParser.h"
#include "storm/adapters/JsonAdapter.h"
#include "storm/adapters/RationalNumberAdapter.h"
#include "storm/api/builder.h"
#include "storm/api/export.h"
#include "storm/api/properties.h"
#include "storm/environment/Environment.h"
#include "storm/environment/solver/EigenSolverEnvironment.h"
#include "storm/environment/solver/SolverEnvironment.h"
#include "storm/logic/Formulas.h"
#include "storm/modelchecker/prctl/SparseDtmcPrctlModelChecker.h"
#include "storm/modelchecker/results/ExplicitQuantitativeCheckResult.h"
#include "storm/models/sparse/Dtmc.h"
#include "storm/models/sparse/StandardRewardModel.h"
#include "storm/utility/ExtendedNumber.h"
#include "storm/utility/constants.h"

namespace {

// From state 1 the target is never reached, so the expected reward is infinite there and in the initial state, which
// reaches state 1 with probability 1/2. States 2 and 3 have a finite expected reward of 1 and 0, respectively.
std::string const modelDescription = R"(
dtmc

module main
    s : [0..3] init 0;
    [] s=0 -> 1/2 : (s'=1) + 1/2 : (s'=2);
    [] s=1 -> 1 : (s'=1);
    [] s=2 -> 1 : (s'=3);
    [] s=3 -> 1 : (s'=3);
endmodule

label "target" = s=3;

rewards "steps"
    s<3 : 1;
endrewards
)";

std::filesystem::path getTemporaryFilePath() {
    std::error_code ec;
    auto tmpDir = std::filesystem::temp_directory_path(ec);
    EXPECT_EQ(0, ec.value()) << "Unable to get temporary directory for check result export test: " << ec.message();
    std::random_device rd;
    std::filesystem::path result;
    do {
        result = tmpDir / std::filesystem::path("storm_check_result_export_test_" + std::to_string(rd()) + ".json");
    } while (std::filesystem::exists(result));
    return result;
}

/*!
 * Checks an expected reward property whose value is infinite in some states, exports the result as JSON and reads the
 * export back. Infinite values must survive that round trip -- for the exact value type just as for double, where the
 * value type has an infinity of its own.
 */
template<typename ValueType>
void runExportTest(storm::Environment const& env) {
    storm::prism::Program program = storm::parser::PrismParser::parseFromString(modelDescription, "testfile");
    auto formulas = storm::api::extractFormulasFromProperties(storm::api::parsePropertiesForPrismProgram("R{\"steps\"}=? [F \"target\"]", program));
    ASSERT_EQ(1ull, formulas.size());
    auto model = storm::api::buildSparseModel<ValueType>(program, formulas)->template as<storm::models::sparse::Dtmc<ValueType>>();
    ASSERT_EQ(4ull, model->getNumberOfStates());

    storm::modelchecker::SparseDtmcPrctlModelChecker<storm::models::sparse::Dtmc<ValueType>> checker(*model);
    std::unique_ptr<storm::modelchecker::CheckResult> result = checker.check(env, *formulas[0]);
    ASSERT_TRUE(result->isExplicitQuantitativeCheckResult());
    auto const& values = result->template asExplicitQuantitativeCheckResult<ValueType>().getValueVector();
    ASSERT_EQ(4ull, values.size());

    // Two states cannot reach the target, so their expected reward is infinite. Without those, the export below would
    // not exercise anything.
    uint64_t numberOfInfiniteValues = 0;
    for (auto const& value : values) {
        if (storm::utility::isInfinity(value)) {
            ++numberOfInfiniteValues;
        }
    }
    ASSERT_EQ(2ull, numberOfInfiniteValues);

    auto const path = getTemporaryFilePath();
    ASSERT_NO_THROW(storm::api::exportCheckResultToJson<ValueType>(model, result, path.string()));
    std::ifstream exportedStream(path);
    ASSERT_TRUE(exportedStream.good());
    storm::json<double> exported;
    ASSERT_NO_THROW(exportedStream >> exported);
    exportedStream.close();
    std::filesystem::remove(path);

    ASSERT_TRUE(exported.is_array());
    ASSERT_EQ(values.size(), exported.size());
    for (auto const& entry : exported) {
        ASSERT_TRUE(entry.count("s") == 1 && entry.count("v") == 1);
        uint64_t const state = entry.at("s").template get<uint64_t>();
        ASSERT_LT(state, values.size());
        if (storm::utility::isInfinity(values[state])) {
            // JSON has no infinity of its own, so an infinite value is exported as this string.
            ASSERT_TRUE(entry.at("v").is_string()) << "State " << state << " has an infinite value, but was exported as " << entry.at("v").dump() << ".";
            EXPECT_EQ("inf", entry.at("v").template get<std::string>());
        } else {
            ASSERT_TRUE(entry.at("v").is_number()) << "State " << state << " has the finite value " << values[state] << ", but was exported as "
                                                   << entry.at("v").dump() << ".";
            EXPECT_EQ(storm::utility::convertNumber<double>(storm::utility::getFinite(values[state])), entry.at("v").template get<double>());
        }
    }
}
}  // namespace

TEST(ExportCheckResultTest, InfiniteRewardDouble) {
    storm::Environment env;
    runExportTest<double>(env);
}

TEST(ExportCheckResultTest, InfiniteRewardExact) {
    storm::Environment env;
    env.solver().setLinearEquationSolverType(storm::solver::EquationSolverType::Eigen);
    env.solver().eigen().setMethod(storm::solver::EigenLinearEquationSolverMethod::SparseLU);
    runExportTest<storm::RationalNumber>(env);
}
