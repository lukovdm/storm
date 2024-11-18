#pragma once
#include "storm/models/sparse/Dtmc.h"
#include "storm/models/sparse/Mdp.h"
#include "storm/models/sparse/Pomdp.h"

namespace storm::generator {

template<typename ValueType>
class MonitorVerifier {
   public:
    MonitorVerifier(const storm::models::sparse::Pomdp<ValueType>& product, const std::map<std::pair<uint32_t, bool>, uint32_t>& observationMap);

    const std::map<std::pair<uint32_t, bool>, uint32_t>& getObservationMap();
    const storm::models::sparse::Pomdp<ValueType>& getProduct();

   private:
    storm::models::sparse::Pomdp<ValueType> product;
    std::map<std::pair<uint32_t, bool>, uint32_t> observationMap;
};

template<typename ValueType>
class GenerateMonitorVerifier {
   public:
    struct Options {
        std::string goodLabel = "good";
        std::string acceptingLabel = "accepting";
        std::string stepPrefix = "step";
        std::string horizonLabel = "horizon";
        bool useRisk = false;
    };
    GenerateMonitorVerifier(storm::models::sparse::Dtmc<ValueType> const& mc, storm::models::sparse::Mdp<ValueType> const& monitor,
                            std::shared_ptr<storm::expressions::ExpressionManager>& exprManager, Options const& options);
    std::shared_ptr<MonitorVerifier<ValueType>> createProduct();
    void setRisk(std::vector<ValueType> const& risk);

   private:
    const storm::models::sparse::Dtmc<ValueType>& mc;
    const storm::models::sparse::Mdp<ValueType>& monitor;
    std::shared_ptr<storm::expressions::ExpressionManager>& exprManager;
    std::vector<ValueType> risk;
    storm::expressions::Variable monvar;
    storm::expressions::Variable mcvar;
    Options options;
};

}  // namespace storm::generator