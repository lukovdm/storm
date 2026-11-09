#include "storm-config.h"
#include "test/storm_gtest.h"

#include <algorithm>
#include <set>
#include <vector>

#include "storm-parsers/parser/PrismParser.h"
#include "storm/api/builder.h"
#include "storm/dd/DdManager.h"
#include "storm/environment/Environment.h"
#include "storm/environment/dd/DdEnvironment.h"
#include "storm/environment/dd/SylvanDdManagerEnvironment.h"
#include "storm/models/sparse/Mdp.h"
#include "storm/models/symbolic/Mdp.h"
#include "storm/storage/decomposition/MaximalEndComponentDecomposition.h"
#include "storm/storage/decomposition/SymbolicMaximalEndComponentDecomposition.h"
#include "storm/transformer/SymbolicToSparseTransformer.h"

namespace {

template<storm::dd::DdType Type>
std::shared_ptr<storm::models::symbolic::Model<Type, double>> buildModel(storm::prism::Program const& program, std::string const& constants = "") {
    storm::Environment env;
    // Sylvan's default (one worker per hardware thread) leaves no core for the calling thread, which makes the many small BDD operations very slow.
    env.dd().sylvan().setNumberOfThreads(1);
    return storm::api::buildSymbolicModel<Type, double>(env, storm::languages::SymbolicModelDescription(program).preprocess(constants), {});
}

template<storm::dd::DdType Type, storm::storage::SymbolicMecAlgorithm Algorithm>
struct TestConfig {
    static constexpr storm::dd::DdType ddType = Type;
    static constexpr storm::storage::SymbolicMecAlgorithm algorithm = Algorithm;
};

template<typename Config>
class SymbolicMaximalEndComponentDecompositionTest : public ::testing::Test {
   public:
    static constexpr storm::dd::DdType DdType = Config::ddType;
    using StateSets = std::set<std::vector<uint64_t>>;

    /*!
     * Builds the model, computes the MECs symbolically and explicitly and checks that both agree.
     * Returns the number of MECs.
     */
    uint64_t checkAgainstSparse(std::string const& file, std::string const& constants = "") {
        storm::prism::Program program = storm::parser::PrismParser::parse(file);
        auto model = buildModel<DdType>(program, constants)->template as<storm::models::symbolic::Mdp<DdType, double>>();
        auto sparseModel = storm::transformer::SymbolicMdpToSparseMdpTransformer<DdType, double>::translate(*model);
        storm::dd::Odd odd = model->getReachableStates().createOdd();

        StateSets expected;
        storm::storage::MaximalEndComponentDecomposition<double> sparseMecs(*sparseModel);
        for (auto const& mec : sparseMecs) {
            std::vector<uint64_t> states;
            for (auto const& stateChoices : mec) {
                states.push_back(stateChoices.first);
            }
            std::sort(states.begin(), states.end());
            expected.insert(states);
        }

        storm::storage::SymbolicMaximalEndComponentDecomposition<DdType> symbolicMecs(*model, Config::algorithm);
        StateSets actual;
        storm::dd::Bdd<DdType> seen = model->getManager().getBddZero();
        for (uint64_t index = 0; index < symbolicMecs.size(); ++index) {
            auto const& mec = symbolicMecs.getMecStates(index);
            EXPECT_TRUE((seen && mec).isZero()) << "MECs are not disjoint.";
            seen |= mec;
            auto states = mec.toVector(odd);
            actual.insert(std::vector<uint64_t>(states.begin(), states.end()));

            // The choices of the MEC must be exactly the choices that stay inside, and every MEC state needs one.
            auto const choices = symbolicMecs.getMecChoices(index);
            EXPECT_EQ(mec, choices.existsAbstract(model->getNondeterminismVariables()));
            auto const choiceTransitions = model->getTransitionMatrix().notZero() && choices;
            EXPECT_TRUE((choiceTransitions && !mec.swapVariables(model->getRowColumnMetaVariablePairs())).isZero());
        }
        EXPECT_EQ(expected.size(), symbolicMecs.size());
        EXPECT_EQ(expected, actual);
        EXPECT_EQ(seen, symbolicMecs.getAllMecStates());
        return symbolicMecs.size();
    }
};

typedef ::testing::Types<TestConfig<storm::dd::DdType::CUDD, storm::storage::SymbolicMecAlgorithm::Interleave>,
                         TestConfig<storm::dd::DdType::CUDD, storm::storage::SymbolicMecAlgorithm::Naive>,
                         TestConfig<storm::dd::DdType::Sylvan, storm::storage::SymbolicMecAlgorithm::Interleave>,
                         TestConfig<storm::dd::DdType::Sylvan, storm::storage::SymbolicMecAlgorithm::Naive>>
    TestingTypes;

TYPED_TEST_SUITE(SymbolicMaximalEndComponentDecompositionTest, TestingTypes, );

TYPED_TEST(SymbolicMaximalEndComponentDecompositionTest, MecExamples) {
    EXPECT_EQ(2ull, this->checkAgainstSparse(STORM_TEST_RESOURCES_DIR "/mdp/prism-mec-example1.nm"));
    this->checkAgainstSparse(STORM_TEST_RESOURCES_DIR "/mdp/prism-mec-example2.nm");
    this->checkAgainstSparse(STORM_TEST_RESOURCES_DIR "/mdp/multiobj_mecs.nm");
    this->checkAgainstSparse(STORM_TEST_RESOURCES_DIR "/mdp/maybe_state_end_component.nm");
}

TYPED_TEST(SymbolicMaximalEndComponentDecompositionTest, Benchmarks) {
    this->checkAgainstSparse(STORM_TEST_RESOURCES_DIR "/mdp/two_dice.nm");
    this->checkAgainstSparse(STORM_TEST_RESOURCES_DIR "/mdp/leader4.nm");
    this->checkAgainstSparse(STORM_TEST_RESOURCES_DIR "/mdp/coin2-2.nm");
    this->checkAgainstSparse(STORM_TEST_RESOURCES_DIR "/mdp/csma2-2.nm");
    this->checkAgainstSparse(STORM_TEST_RESOURCES_DIR "/mdp/cs_nfail3.nm");
    this->checkAgainstSparse(STORM_TEST_RESOURCES_DIR "/mdp/wlan0-2-2.nm");
}

TYPED_TEST(SymbolicMaximalEndComponentDecompositionTest, Subset) {
    // Restricting to a subset must yield the MECs of the sub-MDP in which all choices leaving the subset are removed.
    storm::prism::Program program = storm::parser::PrismParser::parse(STORM_TEST_RESOURCES_DIR "/mdp/prism-mec-example1.nm");
    auto model = buildModel<TestFixture::DdType>(program)->template as<storm::models::symbolic::Mdp<TestFixture::DdType, double>>();
    storm::storage::SymbolicMaximalEndComponentDecomposition<TestFixture::DdType> all(*model);
    ASSERT_FALSE(all.empty());
    auto const firstMec = all.getMecStates(0);
    storm::storage::SymbolicMaximalEndComponentDecomposition<TestFixture::DdType> restricted(*model, firstMec);
    ASSERT_EQ(1ull, restricted.size());
    EXPECT_EQ(firstMec, restricted.getMecStates(0));
    storm::storage::SymbolicMaximalEndComponentDecomposition<TestFixture::DdType> outside(*model, model->getReachableStates() && !all.getAllMecStates());
    EXPECT_TRUE(outside.empty());
}

}  // namespace
