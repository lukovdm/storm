#include "test/storm_gtest.h"
#include "storm-config.h"
#include "storm/api/verification.h"
#include "storm-figaro/api/storm-figaro.h"
#include "figaro-test-headers.h"

#include <string>
#include "storm/modelchecker/results/ExplicitQualitativeCheckResult.h"
#include <iostream>
#include "storm-dft/api/storm-dft.h"
#include "storm-parsers/api/storm-parsers.h"




// Configurations for DFT approximation
namespace {
    struct FigaroAnalysisConfig {
        storm::builder::ApproximationHeuristic heuristic;
        bool useSR;
    };

    class ApproxDepthConfig {
    public:
        typedef double ValueType;

        static FigaroAnalysisConfig createConfig() {
            return FigaroAnalysisConfig{storm::builder::ApproximationHeuristic::DEPTH, false};

        }
    };

    class ApproxProbabilityConfig {
    public:
        typedef double ValueType;

        static FigaroAnalysisConfig createConfig() {
            return FigaroAnalysisConfig{storm::builder::ApproximationHeuristic::PROBABILITY, false};
        }
    };

    class ApproxBoundDifferenceConfig {
    public:
        typedef double ValueType;

        static FigaroAnalysisConfig createConfig() {
            return FigaroAnalysisConfig{storm::builder::ApproximationHeuristic::BOUNDDIFFERENCE, false};
        }
    };

    template<typename TestType>
    class FigaroApproximationTest : public ::testing::Test {
    public:
        typedef typename TestType::ValueType ValueType;

        FigaroApproximationTest() : config(TestType::createConfig()) {}

        FigaroApproximationTest const &getConfig() const {
            return config;
        }

        std::pair<double, double>
        analyzeUnReliability(std::shared_ptr<storm::figaro::FigaroProgram> figaromodel, uint32_t missiontime,
                             double errorBound) {
            std::string property = "Pmin=? [F<=" + std::to_string(missiontime) + "\"failed\"]";
            std::vector<std::shared_ptr<storm::logic::Formula const>> properties = storm::api::extractFormulasFromProperties(
                    storm::api::parseProperties(property));

            storm::figaro::modelchecker::FigaroModelChecker<double>::figaro_results results =
                    storm::figaro::api::analyzeFigaro<double>(*figaromodel, properties, errorBound, config.heuristic, true);
            return boost::get<storm::figaro::modelchecker::FigaroModelChecker<double>::approximation_result>(
                    results[0]);
        }

        std::pair<double, double>
        analyzeUnAvailability(std::shared_ptr<storm::figaro::FigaroProgram> figaromodel, uint32_t missiontime,
                              double errorBound) {
            std::string property =
                    "Pmin=? [F[" + std::to_string(missiontime) + "," + std::to_string(missiontime) + "] \"failed\"]";
            std::vector<std::shared_ptr<storm::logic::Formula const>> properties = storm::api::extractFormulasFromProperties(
                    storm::api::parseProperties(property));

            storm::figaro::modelchecker::FigaroModelChecker<double>::figaro_results results =
                    storm::figaro::api::analyzeFigaro<double>(*figaromodel, properties, errorBound, config.heuristic, true);
            return boost::get<storm::figaro::modelchecker::FigaroModelChecker<double>::approximation_result>(
                    results[0]);
        }

    private:
        FigaroAnalysisConfig config;

    };

    typedef ::testing::Types<
            ApproxDepthConfig,
            ApproxProbabilityConfig,
            ApproxBoundDifferenceConfig
    > TestingTypes;

    TYPED_TEST_SUITE(FigaroApproximationTest, TestingTypes,);

    /***********************           Test Case1              ***********************/
    TYPED_TEST(FigaroApproximationTest, 01_2trainsElec_No_trim_No_repair) {
        uint32_t missiontime = 100;
        double errorBound = 0.0001;

        std::pair<double, double> approxResult = this->analyzeUnReliability(
                std::make_shared<storm::figaro::FigaroProgram_BDMP_01_2trainsElec_No_trim_No_repair>(
                        storm::figaro::FigaroProgram_BDMP_01_2trainsElec_No_trim_No_repair()), missiontime, errorBound);
//        EXPECT_LE(approxResult.first, 0.000103935911);
//        EXPECT_GE(approxResult.second, 0.000103935911);
        EXPECT_NEAR(approxResult.first, 0.000103935911, errorBound);
        EXPECT_NEAR(approxResult.second, 0.000103935911, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<"0.000103935911"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
    }


        TYPED_TEST(FigaroApproximationTest, 01_2trainsElec_Trim_Max_No_repair) {
      uint32_t missiontime = 100;
        double errorBound = 0.0001;

        std::pair<double, double> approxResult = this->analyzeUnReliability(
                std::make_shared<storm::figaro::FigaroProgram_BDMP_01_2trainsElec_Trim_Max_No_repair>(
                        storm::figaro::FigaroProgram_BDMP_01_2trainsElec_Trim_Max_No_repair()), missiontime, errorBound);
        EXPECT_NEAR(approxResult.first, 0.000103935911,errorBound);
//        EXPECT_GE(approxResult.second, 0.000103935911);
        EXPECT_NEAR(approxResult.second, 0.000103935911, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<"0.000103935911"<<" Upper limit:"<<approxResult.second<<std::endl;
        EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
}
    TYPED_TEST(FigaroApproximationTest, 01_2trainsElec_Trim_Article_No_repair) {
      uint32_t missiontime = 100;
        double errorBound = 0.0001;

        std::pair<double, double> approxResult = this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_BDMP_01_2trainsElec_Trim_Article_No_repair>(storm::figaro::FigaroProgram_BDMP_01_2trainsElec_Trim_Article_No_repair()), missiontime, errorBound);
//        EXPECT_LE(approxResult.first, 0.000103935911);
//        EXPECT_GE(approxResult.second,0.000103935911 );
        EXPECT_NEAR(approxResult.first, 0.000103935911, errorBound);
        EXPECT_NEAR(approxResult.second,0.000103935911, errorBound );
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<"0.000103935911"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
}
    TYPED_TEST(FigaroApproximationTest, 01_2trainsElec_No_trim_repair) {
      uint32_t missiontime = 10000;
        double errorBound = 0.0001;
        std::pair<double, double> approxResult = this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_BDMP_01_2trainsElec_No_trim_repair>(storm::figaro::FigaroProgram_BDMP_01_2trainsElec_No_trim_repair()), missiontime, errorBound);
//        EXPECT_LE(approxResult.first, 0.002488490481);
//        EXPECT_GE(approxResult.second, 0.002488490481);
        EXPECT_NEAR(approxResult.first, 0.002488490481, errorBound);
        EXPECT_NEAR(approxResult.second, 0.002488490481, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<"0.002488490481"<<" Upper limit:"<<approxResult.second<<std::endl;
//        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);

        approxResult = this->analyzeUnAvailability(std::make_shared<storm::figaro::FigaroProgram_BDMP_01_2trainsElec_No_trim_repair>(storm::figaro::FigaroProgram_BDMP_01_2trainsElec_No_trim_repair()), missiontime, errorBound);
//        EXPECT_LE(approxResult.first, 0.000001249092981);
//        EXPECT_GE(approxResult.second, 0.000001249092981);
        EXPECT_NEAR(approxResult.first, 0.000001249092981, errorBound);
        EXPECT_NEAR(approxResult.second, 0.000001249092981, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<"0.000001249092981"<<" Upper limit:"<<approxResult.second<<std::endl;
//        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
}
////
    TYPED_TEST(FigaroApproximationTest, 01_2trainsElec_Trim_Max_repair) {
      uint32_t missiontime = 10000;
        double errorBound = 0.0001;

        std::pair<double, double> approxResult = this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_BDMP_01_2trainsElec_Trim_Max_repair>(storm::figaro::FigaroProgram_BDMP_01_2trainsElec_Trim_Max_repair()), missiontime, errorBound);
//        EXPECT_LE(approxResult.first,0.002487996285 );
//        EXPECT_GE(approxResult.second, 0.002487996285);
        EXPECT_NEAR(approxResult.first,0.002487996285, errorBound);
        EXPECT_NEAR(approxResult.second, 0.002487996285, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<"0.002487996285"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);

        approxResult = this->analyzeUnAvailability(std::make_shared<storm::figaro::FigaroProgram_BDMP_01_2trainsElec_Trim_Max_repair>(storm::figaro::FigaroProgram_BDMP_01_2trainsElec_Trim_Max_repair()), missiontime, errorBound);
//        EXPECT_LE(approxResult.first, 0.000001248511209);
//        EXPECT_GE(approxResult.second,0.000001248511209 );
        EXPECT_NEAR(approxResult.first, 0.000001248511209, errorBound);
        EXPECT_NEAR(approxResult.second,0.000001248511209, errorBound );
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<"0.000001248511209"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
}
//
//
    TYPED_TEST(FigaroApproximationTest, 01_2trainsElec_Trim_Article_repair) {
      uint32_t missiontime = 10000;
        double errorBound = 0.0001;
        std::pair<double, double> approxResult = this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_BDMP_01_2trainsElec_Trim_Article_repair>(storm::figaro::FigaroProgram_BDMP_01_2trainsElec_Trim_Article_repair()), missiontime, errorBound);
//        EXPECT_LE(approxResult.first,0.002487996285 );
//        EXPECT_GE(approxResult.second,0.002487996285 );
        EXPECT_NEAR(approxResult.first,0.002487996285, errorBound );
        EXPECT_NEAR(approxResult.second,0.002487996285, errorBound );
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<"0.002487996285"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
        approxResult = this->analyzeUnAvailability(std::make_shared<storm::figaro::FigaroProgram_BDMP_01_2trainsElec_Trim_Article_repair>(storm::figaro::FigaroProgram_BDMP_01_2trainsElec_Trim_Article_repair()), missiontime, errorBound);
//        EXPECT_LE(approxResult.first, 0.000001248611032);
//        EXPECT_GE(approxResult.second, 0.000001248611032);
        EXPECT_NEAR(approxResult.first, 0.000001248611032, errorBound);
        EXPECT_NEAR(approxResult.second, 0.000001248611032, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<"0.000001248611032"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
}
/////***********************           Test Case 2              ***********************/
//
    TYPED_TEST(FigaroApproximationTest, 02_3trains_standby_redundancy_No_trim_No_repair) {
        uint32_t missiontime = 100;
        double errorBound = 0.0001;
        std::pair<double, double> approxResult =
        this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_BDMP_02_3trains_standby_redundancy_No_trim_No_repair>(storm::figaro::FigaroProgram_BDMP_02_3trains_standby_redundancy_No_trim_No_repair()), missiontime, errorBound);
//        EXPECT_LE(approxResult.first, 0.0000003399185924);
//        EXPECT_GE(approxResult.second,0.0000003399185924 );
        EXPECT_NEAR(approxResult.first, 0.0000003399185924, errorBound);
        EXPECT_NEAR(approxResult.second,0.0000003399185924, errorBound );
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<"0.0000003399185924"<<" Upper limit:"<<approxResult.second<<std::endl;
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
}
//
        TYPED_TEST(FigaroApproximationTest, 02_3trains_standby_redundancy_Trim_Max_No_repair) {
   uint32_t missiontime = 100;
    double errorBound = 0.0001;
    std::pair<double, double> approxResult= this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_BDMP_02_3trains_standby_redundancy_Trim_Max_No_repair>(storm::figaro::FigaroProgram_BDMP_02_3trains_standby_redundancy_Trim_Max_No_repair()), missiontime, errorBound);
        EXPECT_NEAR(approxResult.first, 0.0000003399185924, errorBound);
        EXPECT_NEAR(approxResult.second, 0.0000003399185924, errorBound);
//        EXPECT_LE(approxResult.first, 0.0000003399185924);
//        EXPECT_GE(approxResult.second, 0.0000003399185924);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.0000003399185924"<<" Upper limit:"<<approxResult.second<<std::endl;
//        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
}

    TYPED_TEST(FigaroApproximationTest, 02_3trains_standby_redundancy_Trim_Article_No_repair) {
   uint32_t missiontime = 100;
    double errorBound = 0.0001;
    std::pair<double, double> approxResult= this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_BDMP_02_3trains_standby_redundancy_Trim_Article_No_repair>(storm::figaro::FigaroProgram_BDMP_02_3trains_standby_redundancy_Trim_Article_No_repair()), missiontime, errorBound);
//        EXPECT_LE(approxResult.first, 0.0000003399185924);
//        EXPECT_GE(approxResult.second,0.0000003399185924 );
        EXPECT_NEAR(approxResult.first, 0.0000003399185924, errorBound);
        EXPECT_NEAR(approxResult.second,0.0000003399185924, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.0000003399185924"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
}
//
    TYPED_TEST(FigaroApproximationTest, 02_3trains_standby_redundancy_No_trim_repair) {
           uint32_t missiontime = 10000;
    double errorBound = 0.0001;
    std::pair<double, double> approxResult = this->analyzeUnReliability(
                std::make_shared<storm::figaro::FigaroProgram_BDMP_02_3trains_standby_redundancy_No_trim_repair>(
                        storm::figaro::FigaroProgram_BDMP_02_3trains_standby_redundancy_No_trim_repair()), missiontime, errorBound);
//        EXPECT_LE(approxResult.first, 0.000001146189041);
//        EXPECT_GE(approxResult.second, 0.000001146189041);
        EXPECT_NEAR(approxResult.first, 0.000001146189041, errorBound);
        EXPECT_NEAR(approxResult.second, 0.000001146189041, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.000001146189041"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);

        approxResult =  this->analyzeUnAvailability(
                std::make_shared<storm::figaro::FigaroProgram_BDMP_02_3trains_standby_redundancy_No_trim_repair>(
                        storm::figaro::FigaroProgram_BDMP_02_3trains_standby_redundancy_No_trim_repair()), missiontime, errorBound);
//        EXPECT_LE(approxResult.first, 0.0000000003994478745);
//        EXPECT_GE(approxResult.second,0.0000000003994478745 );
        EXPECT_NEAR(approxResult.first, 0.0000000003994478745, errorBound);
        EXPECT_NEAR(approxResult.second,0.0000000003994478745, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.0000000003994478745"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
    }
//
    TYPED_TEST(FigaroApproximationTest, 02_3trains_standby_redundancy_Trim_Max_repair) {
   uint32_t missiontime = 10000;
    double errorBound = 0.0001;
    std::pair<double, double> approxResult = this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_BDMP_02_3trains_standby_redundancy_Trim_Max_repair>(storm::figaro::FigaroProgram_BDMP_02_3trains_standby_redundancy_Trim_Max_repair()), missiontime, errorBound);
//        EXPECT_LE(approxResult.first, 0.000001146180739);
//        EXPECT_GE(approxResult.second, 0.000001146180739);
        EXPECT_NEAR(approxResult.first, 0.000001146180739, errorBound);
        EXPECT_NEAR(approxResult.second, 0.000001146180739, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.000001146180739"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);

        approxResult =  this->analyzeUnAvailability(std::make_shared<storm::figaro::FigaroProgram_BDMP_02_3trains_standby_redundancy_Trim_Max_repair>(storm::figaro::FigaroProgram_BDMP_02_3trains_standby_redundancy_Trim_Max_repair()), missiontime, errorBound);
//        EXPECT_LE(approxResult.first,0.0000000003994423063 );
//        EXPECT_GE(approxResult.second, 0.0000000003994423063);
        EXPECT_NEAR(approxResult.first,0.0000000003994423063, errorBound);
        EXPECT_NEAR(approxResult.second, 0.0000000003994423063, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.0000000003994423063"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
}
//
    TYPED_TEST(FigaroApproximationTest, 02_3trains_standby_redundancy_Trim_Article_repair) {
   uint32_t missiontime = 10000;
    double errorBound = 0.0001;
    std::pair<double, double> approxResult = this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_BDMP_02_3trains_standby_redundancy_Trim_Article_repair>(storm::figaro::FigaroProgram_BDMP_02_3trains_standby_redundancy_Trim_Article_repair()), missiontime, errorBound);
//        EXPECT_LE(approxResult.first, 0.000001146180739);
//        EXPECT_GE(approxResult.second, 0.000001146180739);
        EXPECT_NEAR(approxResult.first, 0.000001146180739, errorBound);
        EXPECT_NEAR(approxResult.second, 0.000001146180739, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<"0.000001146180739 "<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);

        approxResult =  this->analyzeUnAvailability(std::make_shared<storm::figaro::FigaroProgram_BDMP_02_3trains_standby_redundancy_Trim_Article_repair>(storm::figaro::FigaroProgram_BDMP_02_3trains_standby_redundancy_Trim_Article_repair()), missiontime, errorBound);
//        EXPECT_LE(approxResult.first, 0.000000000399442315);
//        EXPECT_GE(approxResult.second, 0.000000000399442315);
        EXPECT_NEAR(approxResult.first, 0.000000000399442315, errorBound);
        EXPECT_NEAR(approxResult.second, 0.000000000399442315, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<"0.000000000399442315 "<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
}
//
//
//
/////***********************           Test Case 3              ***********************/
    TYPED_TEST(FigaroApproximationTest, 03_CCF_No_Trim_No_repair) {
   uint32_t missiontime = 100;
    double errorBound = 0.0001;
    std::pair<double, double> approxResult= this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_BDMP_03_CCF_No_Trim_No_repair>(storm::figaro::FigaroProgram_BDMP_03_CCF_No_Trim_No_repair()), missiontime, errorBound);
//        EXPECT_LE(approxResult.first,0.0002951056907 );
//        EXPECT_GE(approxResult.second, 0.0002951056907);
        EXPECT_NEAR(approxResult.first,0.0002951056907, errorBound );
        EXPECT_NEAR(approxResult.second, 0.0002951056907, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.0002951056907"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
}
    TYPED_TEST(FigaroApproximationTest, 03_CCF_Trim_Max_No_repair) {
           uint32_t missiontime = 100;
    double errorBound = 0.0001;
    std::pair<double, double> approxResult= this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_BDMP_03_CCF_Trim_Max_No_repair>(storm::figaro::FigaroProgram_BDMP_03_CCF_Trim_Max_No_repair()), missiontime, errorBound);
//        EXPECT_LE(approxResult.first, 0.0002951056907);
//        EXPECT_GE(approxResult.second, 0.0002951056907);
        EXPECT_NEAR(approxResult.first, 0.0002951056907, errorBound);
        EXPECT_NEAR(approxResult.second, 0.0002951056907, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<"0.0002951056907 "<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
    }
    TYPED_TEST(FigaroApproximationTest, 03_CCF_Trim_Article_No_repair) {
           uint32_t missiontime = 100;
    double errorBound = 0.0001;
    std::pair<double, double> approxResult= this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_BDMP_03_CCF_Trim_Article_No_repair>(storm::figaro::FigaroProgram_BDMP_03_CCF_Trim_Article_No_repair()), missiontime, errorBound);
//        EXPECT_LE(approxResult.first,0.0002951056907 );
//        EXPECT_GE(approxResult.second, 0.0002951056907);
        EXPECT_NEAR(approxResult.first,0.0002951056907, errorBound);
        EXPECT_NEAR(approxResult.second, 0.0002951056907, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<"0.0002951056907 "<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
    }
//
    TYPED_TEST(FigaroApproximationTest, 03_CCF_No_Trim_repair) {
           uint32_t missiontime = 10000;
    double errorBound = 0.0001;
    std::pair<double, double> approxResult = this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_BDMP_03_CCF_No_Trim_repair>(storm::figaro::FigaroProgram_BDMP_03_CCF_No_Trim_repair()), missiontime, errorBound);
//        EXPECT_LE(approxResult.first, 0.005947705193);
//        EXPECT_GE(approxResult.second, 0.005947705193);
        EXPECT_NEAR(approxResult.first, 0.005947705193, errorBound);
        EXPECT_NEAR(approxResult.second, 0.005947705193, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.005947705193"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);

        approxResult =  this->analyzeUnAvailability(std::make_shared<storm::figaro::FigaroProgram_BDMP_03_CCF_No_Trim_repair>(storm::figaro::FigaroProgram_BDMP_03_CCF_No_Trim_repair()), missiontime, errorBound);
//        EXPECT_LE(approxResult.first, 0.000002992627521);
//        EXPECT_GE(approxResult.second, 0.000002992627521);
        EXPECT_NEAR(approxResult.first, 0.000002992627521, errorBound);
        EXPECT_NEAR(approxResult.second, 0.000002992627521, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.000002992627521"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
    }

    TYPED_TEST(FigaroApproximationTest, 03_CCF_Trim_Max_repair) {
   uint32_t missiontime = 10000;
    double errorBound = 0.0001;
    std::pair<double, double> approxResult = this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_BDMP_03_CCF_Trim_Max_repair>(storm::figaro::FigaroProgram_BDMP_03_CCF_Trim_Max_repair()), missiontime, errorBound);
//        EXPECT_GE(approxResult.second,0.005947704899 );
//        EXPECT_LE(approxResult.first, 0.005947704899);
        EXPECT_NEAR(approxResult.second,0.005947704899, errorBound);
        EXPECT_NEAR(approxResult.first, 0.005947704899, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.005947704899"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);

        approxResult =  this->analyzeUnAvailability(std::make_shared<storm::figaro::FigaroProgram_BDMP_03_CCF_Trim_Max_repair>(storm::figaro::FigaroProgram_BDMP_03_CCF_Trim_Max_repair()), missiontime, errorBound);
//        EXPECT_LE(approxResult.first, 0.000002991629812);
//        EXPECT_GE(approxResult.second, 0.000002991629812);
        EXPECT_NEAR(approxResult.first, 0.000002991629812, errorBound);
        EXPECT_NEAR(approxResult.second, 0.000002991629812, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<"0.000002991629812 "<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
}

    TYPED_TEST(FigaroApproximationTest, 03_CCF_Trim_Article_repair) {
   uint32_t missiontime = 10000;
    double errorBound = 0.0001;
    std::pair<double, double> approxResult = this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_BDMP_03_CCF_Trim_Article_repair>(storm::figaro::FigaroProgram_BDMP_03_CCF_Trim_Article_repair()), missiontime, errorBound);
//        EXPECT_LE(approxResult.first, 0.005947704899);
//        EXPECT_GE(approxResult.second, 0.005947704899);
        EXPECT_NEAR(approxResult.first, 0.005947704899, errorBound);
        EXPECT_NEAR(approxResult.second, 0.005947704899, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.005947704899"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);

        approxResult =  this->analyzeUnAvailability(std::make_shared<storm::figaro::FigaroProgram_BDMP_03_CCF_Trim_Article_repair>(storm::figaro::FigaroProgram_BDMP_03_CCF_Trim_Article_repair()), missiontime, errorBound);
//        EXPECT_LE(approxResult.first, 0.000002991630011);
//        EXPECT_GE(approxResult.second, 0.000002991630011);
        EXPECT_NEAR(approxResult.first, 0.000002991630011, errorBound);
        EXPECT_NEAR(approxResult.second, 0.000002991630011, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<"0.000002991630011 "<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
}
//
/////***********************           Test Case 4              ***********************/
    TYPED_TEST(FigaroApproximationTest, 04_Demoeng_No_trim_No_repair) {
   uint32_t missiontime = 100;
    double errorBound = 0.0001;
    std::pair<double, double> approxResult= this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_BDMP_04_Demoeng_No_trim_No_repair>(storm::figaro::FigaroProgram_BDMP_04_Demoeng_No_trim_No_repair()), missiontime, errorBound);
//        EXPECT_LE(approxResult.first, 0.0001040405415);
//        EXPECT_GE(approxResult.second, 0.0001040405415);
        EXPECT_NEAR(approxResult.first, 0.0001040405415, errorBound);
        EXPECT_NEAR(approxResult.second, 0.0001040405415, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.0001040405415"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
}
    TYPED_TEST(FigaroApproximationTest, 04_Demoeng_Trim_Max_No_repair) {
           uint32_t missiontime = 100;
    double errorBound = 0.0001;
    std::pair<double, double> approxResult= this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_BDMP_04_Demoeng_Trim_Max_No_repair>(storm::figaro::FigaroProgram_BDMP_04_Demoeng_Trim_Max_No_repair()), missiontime, errorBound);
//        EXPECT_LE(approxResult.first, 0.0001040405415);
//        EXPECT_GE(approxResult.second,0.0001040405415 );
        EXPECT_NEAR(approxResult.first, 0.0001040405415, errorBound);
        EXPECT_NEAR(approxResult.second,0.0001040405415, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<"0.0001040405415 "<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
}
    TYPED_TEST(FigaroApproximationTest, 04_Demoeng_Trim_Article_No_repair) {
           uint32_t missiontime = 100;
    double errorBound = 0.0001;
    std::pair<double, double> approxResult= this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_BDMP_04_Demoeng_Trim_Article_No_repair>(storm::figaro::FigaroProgram_BDMP_04_Demoeng_Trim_Article_No_repair()), missiontime, errorBound);
//        EXPECT_LE(approxResult.first, 0.0001040405415);
//        EXPECT_GE(approxResult.second,0.0001040405415 );
        EXPECT_NEAR(approxResult.first, 0.0001040405415, errorBound);
        EXPECT_NEAR(approxResult.second,0.0001040405415, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.0001040405415"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
}
    TYPED_TEST(FigaroApproximationTest, 04_Demoeng_No_trim_repair) {
           uint32_t missiontime = 10000;
    double errorBound = 0.0001;
    std::pair<double, double> approxResult = this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_BDMP_04_Demoeng_No_trim_repair>(storm::figaro::FigaroProgram_BDMP_04_Demoeng_No_trim_repair()), missiontime, errorBound);
//        EXPECT_LE(approxResult.first, 0.002200475631);
//        EXPECT_GE(approxResult.second, 0.002200475631);
        EXPECT_NEAR(approxResult.first, 0.002200475631, errorBound);
        EXPECT_NEAR(approxResult.second, 0.002200475631, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.002200475631"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);

        approxResult =  this->analyzeUnAvailability(std::make_shared<storm::figaro::FigaroProgram_BDMP_04_Demoeng_No_trim_repair>(storm::figaro::FigaroProgram_BDMP_04_Demoeng_No_trim_repair()), missiontime, errorBound);
//        EXPECT_LE(approxResult.first, 0.000001102397375);
//        EXPECT_GE(approxResult.second,0.000001102397375 );
        EXPECT_NEAR(approxResult.first, 0.000001102397375, errorBound);
        EXPECT_NEAR(approxResult.second,0.000001102397375,errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.000001102397375"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
    }

    TYPED_TEST(FigaroApproximationTest, 04_Demoeng_Trim_Max_repair) {
   uint32_t missiontime = 10000;
    double errorBound = 0.0001;
    std::pair<double, double> approxResult = this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_BDMP_04_Demoeng_Trim_Max_repair>(storm::figaro::FigaroProgram_BDMP_04_Demoeng_Trim_Max_repair()), missiontime, errorBound);
//        EXPECT_LE(approxResult.first, 0.002200433779);
//        EXPECT_GE(approxResult.second, 0.002200433779);
        EXPECT_NEAR(approxResult.first, 0.002200433779, errorBound);
        EXPECT_NEAR(approxResult.second, 0.002200433779, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.002200433779"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);

        approxResult =  this->analyzeUnAvailability(std::make_shared<storm::figaro::FigaroProgram_BDMP_04_Demoeng_Trim_Max_repair>(storm::figaro::FigaroProgram_BDMP_04_Demoeng_Trim_Max_repair()), missiontime, errorBound);
//        EXPECT_LE(approxResult.first, 0.000001102173139);
//        EXPECT_GE(approxResult.second, 0.000001102173139);
        EXPECT_NEAR(approxResult.first, 0.000001102173139, errorBound);
        EXPECT_NEAR(approxResult.second, 0.000001102173139, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.000001102173139"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
}
    TYPED_TEST(FigaroApproximationTest, 04_Demoeng_Trim_Article_repair) {
   uint32_t missiontime = 10000;
    double errorBound = 0.0001;
    std::pair<double, double> approxResult = this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_BDMP_04_Demoeng_Trim_Article_repair>(storm::figaro::FigaroProgram_BDMP_04_Demoeng_Trim_Article_repair()), missiontime, errorBound);
//        EXPECT_LE(approxResult.first, 0.002200433779);
//        EXPECT_GE(approxResult.second, 0.002200433779);
        EXPECT_NEAR(approxResult.first, 0.002200433779, errorBound);
        EXPECT_NEAR(approxResult.second, 0.002200433779, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.002200433779"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);

        approxResult =  this->analyzeUnAvailability(std::make_shared<storm::figaro::FigaroProgram_BDMP_04_Demoeng_Trim_Article_repair>(storm::figaro::FigaroProgram_BDMP_04_Demoeng_Trim_Article_repair()), missiontime, errorBound);
//        EXPECT_LE(approxResult.first,0.000001102173139 );
//        EXPECT_GE(approxResult.second, 0.000001102173139);
        EXPECT_NEAR(approxResult.first,0.000001102173139, errorBound);
        EXPECT_NEAR(approxResult.second, 0.000001102173139, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.000001102173139"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
}
/////***********************           Test Case 05              ***********************/
    TYPED_TEST(FigaroApproximationTest, 05_No_trim_No_repair) {
   uint32_t missiontime = 100;
    double errorBound = 0.0001;
    std::pair<double, double> approxResult= this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_BDMP_05_No_trim_No_repair>(storm::figaro::FigaroProgram_BDMP_05_No_trim_No_repair()), missiontime, errorBound);
//        EXPECT_LE(approxResult.first, 0.0001019244817);
//        EXPECT_GE(approxResult.second,0.0001019244817 );
        EXPECT_NEAR(approxResult.first, 0.0001019244817, errorBound);
        EXPECT_NEAR(approxResult.second,0.0001019244817, errorBound );
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<"0.0001019244817 "<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
}
    TYPED_TEST(FigaroApproximationTest, 05_Trim_Max_No_repair) {
           uint32_t missiontime = 100;
    double errorBound = 0.0001;
    std::pair<double, double> approxResult= this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_BDMP_05_Trim_Max_No_repair>(storm::figaro::FigaroProgram_BDMP_05_Trim_Max_No_repair()), missiontime, errorBound);
//        EXPECT_LE(approxResult.first, 0.0001019244817);
//        EXPECT_GE(approxResult.second, 0.0001019244817);
        EXPECT_NEAR(approxResult.first, 0.0001019244817, errorBound);
        EXPECT_NEAR(approxResult.second, 0.0001019244817, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<"0.0001019244817 "<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
}
    TYPED_TEST(FigaroApproximationTest, 05_Trim_Article_No_repair) {
           uint32_t missiontime = 100;
    double errorBound = 0.0001;
    std::pair<double, double> approxResult= this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_BDMP_05_Trim_Article_No_repair>(storm::figaro::FigaroProgram_BDMP_05_Trim_Article_No_repair()), missiontime, errorBound);
//        EXPECT_LE(approxResult.first, 0.0001019244817);
//        EXPECT_GE(approxResult.second, 0.0001019244817);
        EXPECT_NEAR(approxResult.first, 0.0001019244817, errorBound);
        EXPECT_NEAR(approxResult.second, 0.0001019244817, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.0001019244817"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
}
    TYPED_TEST(FigaroApproximationTest, 05_No_trim_repair) {
           uint32_t missiontime = 10000;
    double errorBound = 0.0001;
    std::pair<double, double> approxResult = this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_BDMP_05_No_trim_repair>(storm::figaro::FigaroProgram_BDMP_05_No_trim_repair()), missiontime, errorBound);
//        EXPECT_LE(approxResult.first, 0.001998998636);
//        EXPECT_GE(approxResult.second,0.001998998636 );
        EXPECT_NEAR(approxResult.first, 0.001998998636, errorBound);
        EXPECT_NEAR(approxResult.second,0.001998998636, errorBound );
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.001998998636"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);

        approxResult =  this->analyzeUnAvailability(std::make_shared<storm::figaro::FigaroProgram_BDMP_05_No_trim_repair>(storm::figaro::FigaroProgram_BDMP_05_No_trim_repair()), missiontime, errorBound);
//        EXPECT_LE(approxResult.first, 0.000001001677339);
//        EXPECT_GE(approxResult.second,0.000001001677339 );
        EXPECT_NEAR(approxResult.first, 0.000001001677339, errorBound);
        EXPECT_NEAR(approxResult.second,0.000001001677339, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<"0.000001001677339 "<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
    }
    TYPED_TEST(FigaroApproximationTest, 05_Trim_Max_repair) {
   uint32_t missiontime = 10000;
    double errorBound = 0.0001;
    std::pair<double, double> approxResult = this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_BDMP_05_Trim_Max_repair>(storm::figaro::FigaroProgram_BDMP_05_Trim_Max_repair()), missiontime, errorBound);
//        EXPECT_LE(approxResult.first, 0.001998961649);
//        EXPECT_GE(approxResult.second, 0.001998961649);
        EXPECT_NEAR(approxResult.first, 0.001998961649, errorBound);
        EXPECT_NEAR(approxResult.second, 0.001998961649, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<"0.001998961649 "<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);

        approxResult =  this->analyzeUnAvailability(std::make_shared<storm::figaro::FigaroProgram_BDMP_05_Trim_Max_repair>(storm::figaro::FigaroProgram_BDMP_05_Trim_Max_repair()), missiontime, errorBound);
//        EXPECT_LE(approxResult.first, 0.000001001489273);
//        EXPECT_GE(approxResult.second, 0.000001001489273);
        EXPECT_NEAR(approxResult.first, 0.000001001489273, errorBound);
        EXPECT_NEAR(approxResult.second, 0.000001001489273, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.000001001489273"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
}
    TYPED_TEST(FigaroApproximationTest, 05_Trim_Article_repair) {
   uint32_t missiontime = 10000;
    double errorBound = 0.0001;
    std::pair<double, double> approxResult = this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_BDMP_05_Trim_Article_repair>(storm::figaro::FigaroProgram_BDMP_05_Trim_Article_repair()), missiontime, errorBound);
//        EXPECT_LE(approxResult.first, 0.001998961649);
//        EXPECT_GE(approxResult.second, 0.001998961649);
        EXPECT_NEAR(approxResult.first, 0.001998961649, errorBound);
        EXPECT_NEAR(approxResult.second, 0.001998961649, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.001998961649"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);

        approxResult =  this->analyzeUnAvailability(std::make_shared<storm::figaro::FigaroProgram_BDMP_05_Trim_Article_repair>(storm::figaro::FigaroProgram_BDMP_05_Trim_Article_repair()), missiontime, errorBound);
//        EXPECT_LE(approxResult.first, 0.000001001489273);
//        EXPECT_GE(approxResult.second, 0.000001001489273);
        EXPECT_NEAR(approxResult.first, 0.000001001489273, errorBound);
        EXPECT_NEAR(approxResult.second, 0.000001001489273, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.000001001489273"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
}

///***********************           Test Case 08              ***********************/
    TYPED_TEST(FigaroApproximationTest, 08_PC_No_trim_No_repair) {
   uint32_t missiontime = 100;
    double errorBound = 0.0001;
    std::pair<double, double> approxResult= this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_BDMP_08_PC_No_trim_No_repair>(storm::figaro::FigaroProgram_BDMP_08_PC_No_trim_No_repair()), missiontime, errorBound);
        EXPECT_NEAR(approxResult.first,0.04981195917 , errorBound);
        EXPECT_NEAR(approxResult.second, 0.04981195917, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.04981195917"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
}
    TYPED_TEST(FigaroApproximationTest, 08_PC_Trim_Article_No_repair) {
           uint32_t missiontime = 100;
    double errorBound = 0.0001;
    std::pair<double, double> approxResult= this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_BDMP_08_PC_Trim_Article_No_repair>(storm::figaro::FigaroProgram_BDMP_08_PC_Trim_Article_No_repair()), missiontime, errorBound);
        EXPECT_NEAR(approxResult.first, 0.04981195917, errorBound);
        EXPECT_NEAR(approxResult.second, 0.04981195917, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.04981195917"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
}
    TYPED_TEST(FigaroApproximationTest, 08_PC_Trim_Max_No_repair) {
           uint32_t missiontime = 100;
    double errorBound = 0.0001;
    std::pair<double, double> approxResult= this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_BDMP_08_PC_Trim_Max_No_repair>(storm::figaro::FigaroProgram_BDMP_08_PC_Trim_Max_No_repair()), missiontime, errorBound);
        EXPECT_NEAR(approxResult.first, 0.04981195917, errorBound);
        EXPECT_NEAR(approxResult.second, 0.04981195917, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<"0.04981195917 "<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
}
    TYPED_TEST(FigaroApproximationTest, 08_PC_No_trim_repair) {
           uint32_t missiontime = 10000;
    double errorBound = 0.0001;
    std::pair<double, double> approxResult = this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_BDMP_08_PC_No_trim_repair>(storm::figaro::FigaroProgram_BDMP_08_PC_No_trim_repair()), missiontime, errorBound);
        EXPECT_NEAR(approxResult.first, 0.9939153995, errorBound);
        EXPECT_NEAR(approxResult.second, 0.9939153995, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.9939153995"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);

        approxResult =  this->analyzeUnAvailability(std::make_shared<storm::figaro::FigaroProgram_BDMP_08_PC_No_trim_repair>(storm::figaro::FigaroProgram_BDMP_08_PC_No_trim_repair()), missiontime, errorBound);
        EXPECT_NEAR(approxResult.first,0.005086413952 , errorBound);
        EXPECT_NEAR(approxResult.second, 0.005086413952, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.005086413952"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
    }
    TYPED_TEST(FigaroApproximationTest, 08_PC_Trim_Max_repair) {
   uint32_t missiontime = 10000;
    double errorBound = 0.0001;
    std::pair<double, double> approxResult = this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_BDMP_08_PC_Trim_Max_repair>(storm::figaro::FigaroProgram_BDMP_08_PC_Trim_Max_repair()), missiontime, errorBound);
        EXPECT_NEAR(approxResult.first,0.9939153995 , errorBound);
        EXPECT_NEAR(approxResult.second,0.9939153995 , errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.9939153995"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);

        approxResult =  this->analyzeUnAvailability(std::make_shared<storm::figaro::FigaroProgram_BDMP_08_PC_Trim_Max_repair>(storm::figaro::FigaroProgram_BDMP_08_PC_Trim_Max_repair()), missiontime, errorBound);
        EXPECT_NEAR(approxResult.first,0.005075551948 , errorBound);
        EXPECT_NEAR(approxResult.second, 0.005075551948, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.005075551948"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
}
    TYPED_TEST(FigaroApproximationTest, 08_PC_Trim_Article_repair) {
   uint32_t missiontime = 10000;
    double errorBound = 0.0001;
    std::pair<double, double> approxResult = this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_BDMP_08_PC_Trim_Article_repair>(storm::figaro::FigaroProgram_BDMP_08_PC_Trim_Article_repair()), missiontime, errorBound);
        EXPECT_NEAR(approxResult.first, 0.9939153995, errorBound);
        EXPECT_NEAR(approxResult.second,0.9939153995 , errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.9939153995"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);

        approxResult =  this->analyzeUnAvailability(std::make_shared<storm::figaro::FigaroProgram_BDMP_08_PC_Trim_Article_repair>(storm::figaro::FigaroProgram_BDMP_08_PC_Trim_Article_repair()), missiontime, errorBound);
        EXPECT_NEAR(approxResult.first, 0.005077532246, errorBound);
        EXPECT_NEAR(approxResult.second, 0.005077532246, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<"0.005077532246 "<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
}
/***********************           Test Case 10              ***********************/

    TYPED_TEST(FigaroApproximationTest, 10_Project_risks_No_trim_No_repair) {
   uint32_t missiontime = 100;
    double errorBound = 0.0001;
    std::pair<double, double> approxResult= this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_BDMP_10_Project_risks_No_trim_No_repair>(storm::figaro::FigaroProgram_BDMP_10_Project_risks_No_trim_No_repair()), missiontime, errorBound);
        EXPECT_NEAR(approxResult.first, 0.01985001112, errorBound);
        EXPECT_NEAR(approxResult.second, 0.01985001112, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<"0.01985001112 "<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
}
    TYPED_TEST(FigaroApproximationTest, 10_Project_risks_Trim_Article_No_repair) {
   uint32_t missiontime = 100;
    double errorBound = 0.0001;
    std::pair<double, double> approxResult= this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_BDMP_10_Project_risks_Trim_Article_No_repair>(storm::figaro::FigaroProgram_BDMP_10_Project_risks_Trim_Article_No_repair()), missiontime, errorBound);
        EXPECT_NEAR(approxResult.first, 0.01985001112, errorBound);
        EXPECT_NEAR(approxResult.second, 0.01985001112, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.01985001112"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
}
    TYPED_TEST(FigaroApproximationTest, 10_Project_risks_Trim_Max_No_repair) {
   uint32_t missiontime = 100;
    double errorBound = 0.0001;
    std::pair<double, double> approxResult= this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_BDMP_10_Project_risks_Trim_Max_No_repair>(storm::figaro::FigaroProgram_BDMP_10_Project_risks_Trim_Max_No_repair()), missiontime, errorBound);
        EXPECT_NEAR(approxResult.first, 0.01985001112, errorBound);
        EXPECT_NEAR(approxResult.second, 0.01985001112, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.01985001112"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
}
    TYPED_TEST(FigaroApproximationTest, 10_Project_risks_No_trim_repair) {
   uint32_t missiontime = 10000;
    double errorBound = 0.0001;
    std::pair<double, double> approxResult = this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_BDMP_10_Project_risks_No_trim_repair>(storm::figaro::FigaroProgram_BDMP_10_Project_risks_No_trim_repair()), missiontime, errorBound);
        EXPECT_NEAR(approxResult.first, 0.86479958, errorBound);
        EXPECT_NEAR(approxResult.second, 0.86479958, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.86479958"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);

        approxResult =  this->analyzeUnAvailability(std::make_shared<storm::figaro::FigaroProgram_BDMP_10_Project_risks_No_trim_repair>(storm::figaro::FigaroProgram_BDMP_10_Project_risks_No_trim_repair()), missiontime, errorBound);
        EXPECT_NEAR(approxResult.first, 0.001997502498, errorBound);
        EXPECT_NEAR(approxResult.second,0.001997502498 , errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<"0.001997502498 "<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
}
    TYPED_TEST(FigaroApproximationTest, 10_Project_risks_Trim_Max_repair) {
   uint32_t missiontime = 10000;
    double errorBound = 0.0001;
    std::pair<double, double> approxResult = this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_BDMP_10_Project_risks_Trim_Max_repair>(storm::figaro::FigaroProgram_BDMP_10_Project_risks_Trim_Max_repair()), missiontime, errorBound);
        EXPECT_NEAR(approxResult.first, 0.86479958, errorBound);
        EXPECT_NEAR(approxResult.second, 0.86479958, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.86479958"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);

        approxResult =  this->analyzeUnAvailability(std::make_shared<storm::figaro::FigaroProgram_BDMP_10_Project_risks_Trim_Max_repair>(storm::figaro::FigaroProgram_BDMP_10_Project_risks_Trim_Max_repair()), missiontime, errorBound);
        EXPECT_NEAR(approxResult.first,0.0019965051 , errorBound);
        EXPECT_NEAR(approxResult.second, 0.0019965051, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.0019965051"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);

}
    TYPED_TEST(FigaroApproximationTest, 10_Project_risks_Trim_Article_repair) {
   uint32_t missiontime = 10000;
    double errorBound = 0.0001;
    std::pair<double, double> approxResult = this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_BDMP_10_Project_risks_Trim_Article_repair>(storm::figaro::FigaroProgram_BDMP_10_Project_risks_Trim_Article_repair()), missiontime, errorBound);
        EXPECT_NEAR(approxResult.first, 0.86479958, errorBound);
        EXPECT_NEAR(approxResult.second, 0.86479958, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.86479958"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);

        approxResult =  this->analyzeUnAvailability(std::make_shared<storm::figaro::FigaroProgram_BDMP_10_Project_risks_Trim_Article_repair>(storm::figaro::FigaroProgram_BDMP_10_Project_risks_Trim_Article_repair()), missiontime, errorBound);
        EXPECT_NEAR(approxResult.first, 0.0019965058, errorBound);
        EXPECT_NEAR(approxResult.second, 0.0019965058, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.0019965058"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);

}



   /***********************           Test Case 13              ***********************/
    TYPED_TEST(FigaroApproximationTest, 13_Share1_No_trim_No_repair) {
   uint32_t missiontime = 100;
    double errorBound = 0.0001;
    std::pair<double, double> approxResult= this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_BDMP_13_Share1_No_trim_No_repair>(storm::figaro::FigaroProgram_BDMP_13_Share1_No_trim_No_repair()), missiontime, errorBound);
        EXPECT_NEAR(approxResult.first,0.00009999092446 , errorBound);
        EXPECT_NEAR(approxResult.second, 0.00009999092446, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.00009999092446"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
}
    TYPED_TEST(FigaroApproximationTest, 13_Share1_Trim_Max_No_repair) {
           uint32_t missiontime = 100;
    double errorBound = 0.0001;
    std::pair<double, double> approxResult= this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_BDMP_13_Share1_Trim_Max_No_repair>(storm::figaro::FigaroProgram_BDMP_13_Share1_Trim_Max_No_repair()), missiontime, errorBound);
        EXPECT_NEAR(approxResult.first,0.00009999092446 , errorBound);
        EXPECT_NEAR(approxResult.second, 0.00009999092446, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<"0.00009999092446 "<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
}
    TYPED_TEST(FigaroApproximationTest, 13_Share1_Trim_Article_No_repair) {
   uint32_t missiontime = 100;
    double errorBound = 0.0001;
    std::pair<double, double> approxResult= this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_BDMP_13_Share1_Trim_Article_No_repair>(storm::figaro::FigaroProgram_BDMP_13_Share1_Trim_Article_No_repair()), missiontime, errorBound);
        EXPECT_NEAR(approxResult.first, 0.00009999092446, errorBound);
        EXPECT_NEAR(approxResult.second, 0.00009999092446, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.00009999092446"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
}
    TYPED_TEST(FigaroApproximationTest, 13_Share1_No_trim_repair) {
   uint32_t missiontime = 10000;
    double errorBound = 0.0001;
    std::pair<double, double> approxResult = this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_BDMP_13_Share1_No_trim_repair>(storm::figaro::FigaroProgram_BDMP_13_Share1_No_trim_repair()), missiontime, errorBound);
        EXPECT_NEAR(approxResult.first,0.002090587692 , errorBound);
        EXPECT_NEAR(approxResult.second, 0.002090587692, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.002090587692"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);

        approxResult =  this->analyzeUnAvailability(std::make_shared<storm::figaro::FigaroProgram_BDMP_13_Share1_No_trim_repair>(storm::figaro::FigaroProgram_BDMP_13_Share1_No_trim_repair()), missiontime, errorBound);
        EXPECT_NEAR(approxResult.first, 0.000001048608958, errorBound);
        EXPECT_NEAR(approxResult.second, 0.000001048608958, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.000001048608958"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
}
    TYPED_TEST(FigaroApproximationTest, 13_Share1_Trim_Max_repair) {
   uint32_t missiontime = 10000;
    double errorBound = 0.0001;
    std::pair<double, double> approxResult = this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_BDMP_13_Share1_Trim_Max_repair>(storm::figaro::FigaroProgram_BDMP_13_Share1_Trim_Max_repair()), missiontime, errorBound);
        EXPECT_NEAR(approxResult.first, 0.002090587675, errorBound);
        EXPECT_NEAR(approxResult.second,0.002090587675 , errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<"0.002090587675 "<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);

        approxResult =  this->analyzeUnAvailability(std::make_shared<storm::figaro::FigaroProgram_BDMP_13_Share1_Trim_Max_repair>(storm::figaro::FigaroProgram_BDMP_13_Share1_Trim_Max_repair()), missiontime, errorBound);
        EXPECT_NEAR(approxResult.first, 0.000001048517353, errorBound);
        EXPECT_NEAR(approxResult.second, 0.000001048517353, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.000001048517353"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
}
    TYPED_TEST(FigaroApproximationTest, 13_Share1_Trim_Article_repair) {
   uint32_t missiontime = 10000;
    double errorBound = 0.0001;
    std::pair<double, double> approxResult = this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_BDMP_13_Share1_Trim_Article_repair>(storm::figaro::FigaroProgram_BDMP_13_Share1_Trim_Article_repair()), missiontime, errorBound);
        EXPECT_NEAR(approxResult.first, 0.002090587675, errorBound);
        EXPECT_NEAR(approxResult.second,0.002090587675 , errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.002090587675"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);

        approxResult =  this->analyzeUnAvailability(std::make_shared<storm::figaro::FigaroProgram_BDMP_13_Share1_Trim_Article_repair>(storm::figaro::FigaroProgram_BDMP_13_Share1_Trim_Article_repair()), missiontime, errorBound);
        EXPECT_NEAR(approxResult.first, 0.000001048517353, errorBound);
        EXPECT_NEAR(approxResult.second, 0.000001048517353, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.000001048517353"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
}



/***********************           Test Case 18              ***********************/
    TYPED_TEST(FigaroApproximationTest, 18_ESREL_2013_Event_trees_and_Petri_nets_No_trim_No_repair) {
   uint32_t missiontime = 100;
    double errorBound = 0.0001;
    std::pair<double, double> approxResult= this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_BDMP_18_ESREL_2013_Event_trees_and_Petri_nets_No_trim_No_repair>(storm::figaro::FigaroProgram_BDMP_18_ESREL_2013_Event_trees_and_Petri_nets_No_trim_No_repair()), missiontime, errorBound);
        EXPECT_NEAR(approxResult.first,0.000000995016635 , errorBound);
        EXPECT_NEAR(approxResult.second, 0.000000995016635, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.000000995016635"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
}
    TYPED_TEST(FigaroApproximationTest, 18_ESREL_2013_Event_trees_and_Petri_nets_Trim_Max_No_repair) {
   uint32_t missiontime = 100;
    double errorBound = 0.0001;
    std::pair<double, double> approxResult= this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_BDMP_18_ESREL_2013_Event_trees_and_Petri_nets_Trim_Max_No_repair>(storm::figaro::FigaroProgram_BDMP_18_ESREL_2013_Event_trees_and_Petri_nets_Trim_Max_No_repair()), missiontime, errorBound);

        EXPECT_NEAR(approxResult.first, 0.000000995016635, errorBound);
        EXPECT_NEAR(approxResult.second, 0.000000995016635, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.000000995016635"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
}
    TYPED_TEST(FigaroApproximationTest, 18_ESREL_2013_Event_trees_and_Petri_nets_Trim_Article_No_repair) {
   uint32_t missiontime = 100;
    double errorBound = 0.0001;
    std::pair<double, double> approxResult= this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_BDMP_18_ESREL_2013_Event_trees_and_Petri_nets_Trim_Article_No_repair>(storm::figaro::FigaroProgram_BDMP_18_ESREL_2013_Event_trees_and_Petri_nets_Trim_Article_No_repair()), missiontime, errorBound);

        EXPECT_NEAR(approxResult.first, 0.000000995016635, errorBound);
        EXPECT_NEAR(approxResult.second, 0.000000995016635, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<"0.000000995016635 "<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
}
/***********************           Test Case 21              ***********************/
    TYPED_TEST(FigaroApproximationTest, 21_Remote_Access_Server_Security_No_trim_No_repair) {
   uint32_t missiontime = 100;
    double errorBound = 0.0001;
    std::pair<double, double> approxResult= this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_BDMP_21_Remote_Access_Server_Security_No_trim_No_repair>(storm::figaro::FigaroProgram_BDMP_21_Remote_Access_Server_Security_No_trim_No_repair()), missiontime, errorBound);
        EXPECT_NEAR(approxResult.first, 0.00009916876808, errorBound);
        EXPECT_NEAR(approxResult.second, 0.00009916876808, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.00009916876808"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
}
    TYPED_TEST(FigaroApproximationTest, 21_Remote_Access_Server_Security_Trim_Max_No_repair) {
   uint32_t missiontime = 100;
    double errorBound = 0.0001;
    std::pair<double, double> approxResult= this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_BDMP_21_Remote_Access_Server_Security_Trim_Max_No_repair>(storm::figaro::FigaroProgram_BDMP_21_Remote_Access_Server_Security_Trim_Max_No_repair()), missiontime, errorBound);
        EXPECT_NEAR(approxResult.first, 0.00009916876808, errorBound);
        EXPECT_NEAR(approxResult.second,0.00009916876808 , errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<"0.00009916876808 "<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
}
    TYPED_TEST(FigaroApproximationTest, 21_Remote_Access_Server_Security_Trim_Article_No_repair) {
   uint32_t missiontime = 100;
    double errorBound = 0.0001;
    std::pair<double, double> approxResult= this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_BDMP_21_Remote_Access_Server_Security_Trim_Article_No_repair>(storm::figaro::FigaroProgram_BDMP_21_Remote_Access_Server_Security_Trim_Article_No_repair()), missiontime, errorBound);
        EXPECT_NEAR(approxResult.first, .00009916876808, errorBound);
        EXPECT_NEAR(approxResult.second, .00009916876808, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<".00009916876808 "<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
}
/***********************           Test Case 23              ***********************/

    TYPED_TEST(FigaroApproximationTest, 23_Two_proc_comp_sys_No_Trim_No_Repair) {
   uint32_t missiontime = 100;
    double errorBound = 0.0001;
    std::pair<double, double> approxResult= this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_BDMP_23_Two_proc_comp_sys_No_Trim_No_Repair>(storm::figaro::FigaroProgram_BDMP_23_Two_proc_comp_sys_No_Trim_No_Repair()), missiontime, errorBound);
        EXPECT_NEAR(approxResult.first, 0.01990115696, errorBound);
        EXPECT_NEAR(approxResult.second, 0.01990115696, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.01990115696"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
}
    TYPED_TEST(FigaroApproximationTest, 23_Two_proc_comp_sys_Trim_Max_No_Repair) {
   uint32_t missiontime = 100;
    double errorBound = 0.0001;
    std::pair<double, double> approxResult= this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_BDMP_23_Two_proc_comp_sys_Trim_Max_No_Repair>(storm::figaro::FigaroProgram_BDMP_23_Two_proc_comp_sys_Trim_Max_No_Repair()), missiontime, errorBound);
        EXPECT_NEAR(approxResult.first, 0.01990115696, errorBound);
        EXPECT_NEAR(approxResult.second, 0.01990115696, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.01990115696"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
}
    TYPED_TEST(FigaroApproximationTest, 23_Two_proc_comp_sys_Trim_article_No_Repair) {
   uint32_t missiontime = 100;
    double errorBound = 0.0001;
    std::pair<double, double> approxResult= this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_BDMP_23_Two_proc_comp_sys_Trim_article_No_Repair>(storm::figaro::FigaroProgram_BDMP_23_Two_proc_comp_sys_Trim_article_No_Repair()), missiontime, errorBound);
        EXPECT_NEAR(approxResult.first,0.01990115696 , errorBound);
        EXPECT_NEAR(approxResult.second, 0.01990115696, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.01990115696"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
}
    TYPED_TEST(FigaroApproximationTest, 23_Two_proc_comp_sys_No_Trim_Repair) {
   uint32_t missiontime = 10000;
    double errorBound = 0.0001;
    std::pair<double, double> approxResult = this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_BDMP_23_Two_proc_comp_sys_No_Trim_Repair>(storm::figaro::FigaroProgram_BDMP_23_Two_proc_comp_sys_No_Trim_Repair()), missiontime, errorBound);
        EXPECT_NEAR(approxResult.first, 0.8649352087, errorBound);
        EXPECT_NEAR(approxResult.second, 0.8649352087, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.8649352087"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);

        approxResult =  this->analyzeUnAvailability(std::make_shared<storm::figaro::FigaroProgram_BDMP_23_Two_proc_comp_sys_No_Trim_Repair>(storm::figaro::FigaroProgram_BDMP_23_Two_proc_comp_sys_No_Trim_Repair()), missiontime, errorBound);
        EXPECT_NEAR(approxResult.first,0.001998002891 , errorBound);
        EXPECT_NEAR(approxResult.second,0.001998002891 , errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.001998002891"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
}
    TYPED_TEST(FigaroApproximationTest, 23_Two_proc_comp_sys_Trim_Max_Repair) {
   uint32_t missiontime = 10000;
    double errorBound = 0.0001;
    std::pair<double, double> approxResult = this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_BDMP_23_Two_proc_comp_sys_Trim_Max_Repair>(storm::figaro::FigaroProgram_BDMP_23_Two_proc_comp_sys_Trim_Max_Repair()), missiontime, errorBound);
        EXPECT_NEAR(approxResult.first,0.8649352083 , errorBound);
        EXPECT_NEAR(approxResult.second, 0.8649352083, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<"0.8649352083 "<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);

        approxResult =  this->analyzeUnAvailability(std::make_shared<storm::figaro::FigaroProgram_BDMP_23_Two_proc_comp_sys_Trim_Max_Repair>(storm::figaro::FigaroProgram_BDMP_23_Two_proc_comp_sys_Trim_Max_Repair()), missiontime, errorBound);
        EXPECT_NEAR(approxResult.first,0.001997003889 , errorBound);
        EXPECT_NEAR(approxResult.second, 0.001997003889, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" "<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
}
    TYPED_TEST(FigaroApproximationTest, 23_Two_proc_comp_sys_Trim_article_Repair) {
   uint32_t missiontime = 10000;
    double errorBound = 0.0001;
    std::pair<double, double> approxResult = this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_BDMP_23_Two_proc_comp_sys_Trim_article_Repair>(storm::figaro::FigaroProgram_BDMP_23_Two_proc_comp_sys_Trim_article_Repair()), missiontime, errorBound);
        EXPECT_NEAR(approxResult.first, 0.8649352084, errorBound);
        EXPECT_NEAR(approxResult.second, 0.8649352084, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.8649352084"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);

        approxResult =  this->analyzeUnAvailability(std::make_shared<storm::figaro::FigaroProgram_BDMP_23_Two_proc_comp_sys_Trim_article_Repair>(storm::figaro::FigaroProgram_BDMP_23_Two_proc_comp_sys_Trim_article_Repair()), missiontime, errorBound);
        EXPECT_NEAR(approxResult.first, 0.001997003891, errorBound);
        EXPECT_NEAR(approxResult.second, 0.001997003891, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<"0.001997003891 "<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
}

/***********************           Test Case BDMP              ***********************/
    TYPED_TEST(FigaroApproximationTest,         figaro_BDMP) {
   uint32_t missiontime = 100;
    double errorBound = 0.0001;
    std::pair<double, double> approxResult= this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_figaro_BDMP>(storm::figaro::FigaroProgram_figaro_BDMP()), missiontime, errorBound);
        EXPECT_NEAR(approxResult.first, 0.01787733191, errorBound);
        EXPECT_NEAR(approxResult.second,0.01787733191 , errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<"0.01787733191 "<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);

        approxResult =  this->analyzeUnAvailability(std::make_shared<storm::figaro::FigaroProgram_figaro_BDMP>(storm::figaro::FigaroProgram_figaro_BDMP()), missiontime, errorBound);
        EXPECT_NEAR(approxResult.first, 0.016357312, errorBound);
        EXPECT_NEAR(approxResult.second, 0.016357312, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.016357312"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);

}
/***********************           Test Case Petri net              ***********************/
    TYPED_TEST(FigaroApproximationTest, figaro_Petrinet) {
   uint32_t missiontime = 100;
    double errorBound = 0.0001;
    std::pair<double, double> approxResult= this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_figaro_Petrinet>(storm::figaro::FigaroProgram_figaro_Petrinet()), missiontime, errorBound);
        EXPECT_NEAR(approxResult.first, 0.7090497745, errorBound);
        EXPECT_NEAR(approxResult.second, 0.7090497745, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.7090497745"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);

        approxResult =  this->analyzeUnAvailability(std::make_shared<storm::figaro::FigaroProgram_figaro_Petrinet>(storm::figaro::FigaroProgram_figaro_Petrinet()), missiontime, errorBound);
        EXPECT_NEAR(approxResult.first, 0.21117046, errorBound);
        EXPECT_NEAR(approxResult.second, 0.21117046, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.21117046"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
}
/***********************           Test Case Telecom              ***********************/

    TYPED_TEST(FigaroApproximationTest, figaro_Telecom) {
   uint32_t missiontime = 100;
    double errorBound = 0.0001;
    std::pair<double, double> approxResult= this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_figaro_Telecom>(storm::figaro::FigaroProgram_figaro_Telecom()), missiontime, errorBound);
        EXPECT_NEAR(approxResult.first, 0.002014698, errorBound);
        EXPECT_NEAR(approxResult.second, 0.002014698, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.002014698"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);

        approxResult =  this->analyzeUnAvailability(std::make_shared<storm::figaro::FigaroProgram_figaro_Telecom>(storm::figaro::FigaroProgram_figaro_Telecom()), missiontime, errorBound);
        EXPECT_NEAR(approxResult.first, 0.00021015652, errorBound);
        EXPECT_NEAR(approxResult.second, 0.00021015652, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<"0.00021015652 "<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
}
/***********************           Test Case RBD              ***********************/
    TYPED_TEST(FigaroApproximationTest, figaro_RBD) {
       uint32_t missiontime = 10000;
    double errorBound = 0.0001;
    std::pair<double, double> approxResult = this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_figaro_RBD>(storm::figaro::FigaroProgram_figaro_RBD()), missiontime, errorBound);
        EXPECT_NEAR(approxResult.first, 0.010702526, errorBound);
        EXPECT_NEAR(approxResult.second,0.010702526 , errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" "<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);

        approxResult =  this->analyzeUnAvailability(std::make_shared<storm::figaro::FigaroProgram_figaro_RBD>(storm::figaro::FigaroProgram_figaro_RBD()), missiontime, errorBound);
        EXPECT_NEAR(approxResult.first, 0.000041469983, errorBound);
        EXPECT_NEAR(approxResult.second, 0.000041469983, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" "<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);
}
/***********************           Test Case Miniplant              ***********************/

    TYPED_TEST(FigaroApproximationTest, figaro_Miniplant) {
   uint32_t missiontime = 10000;
    double errorBound = 0.0001;
    std::pair<double, double> approxResult = this->analyzeUnReliability(std::make_shared<storm::figaro::FigaroProgram_figaro_Miniplant>(storm::figaro::FigaroProgram_figaro_Miniplant()), missiontime, errorBound);
        EXPECT_NEAR(approxResult.first, 0.00201696, errorBound);
        EXPECT_NEAR(approxResult.second, 0.00201696, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<"0.00201696 "<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);

        approxResult =  this->analyzeUnAvailability(std::make_shared<storm::figaro::FigaroProgram_figaro_Miniplant>(storm::figaro::FigaroProgram_figaro_Miniplant()), missiontime, errorBound);
        EXPECT_NEAR(approxResult.first,0.0000020271448 , errorBound);
        EXPECT_NEAR(approxResult.second, 0.0000020271448, errorBound);
        std::cout<<std::endl<<"Lower limit: "<<approxResult.first<<"  Nominal Value: "<<" 0.0000020271448"<<" Upper limit:"<<approxResult.second<<std::endl;
        //EXPECT_LE(approxResult.second - approxResult.first, errorBound);
        // Ensure results are not equal -> not exact values were computed
        //EXPECT_GE(approxResult.second - approxResult.first, errorBound / 10);

}


}