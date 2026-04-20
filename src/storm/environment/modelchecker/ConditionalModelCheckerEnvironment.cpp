#include "storm/environment/modelchecker/ConditionalModelCheckerEnvironment.h"

#include "storm/settings/SettingsManager.h"
#include "storm/settings/modules/ModelCheckerSettings.h"

namespace storm {

ConditionalModelCheckerEnvironment::ConditionalModelCheckerEnvironment() {
    auto const& mcSettings = storm::settings::getModule<storm::settings::modules::ModelCheckerSettings>();
    algorithm = mcSettings.getConditionalAlgorithmSetting();
    tolerance = mcSettings.getConditionalTolerance();
    allowOptimizationForBoundedProperties = true;
}

ConditionalModelCheckerEnvironment::~ConditionalModelCheckerEnvironment() {
    // Intentionally left empty
}

ConditionalAlgorithmSetting ConditionalModelCheckerEnvironment::getAlgorithm() const {
    return algorithm;
}

void ConditionalModelCheckerEnvironment::setAlgorithm(ConditionalAlgorithmSetting value) {
    algorithm = value;
}

storm::RationalNumber ConditionalModelCheckerEnvironment::getTolerance() const {
    return tolerance;
}

void ConditionalModelCheckerEnvironment::setTolerance(storm::RationalNumber const& value) {
    tolerance = value;
}

bool ConditionalModelCheckerEnvironment::isAllowOptimizationForBoundedPropertiesSet() const {
    return allowOptimizationForBoundedProperties;
}

void ConditionalModelCheckerEnvironment::setAllowOptimizationForBoundedProperties(bool value) {
    allowOptimizationForBoundedProperties = value;
}

}  // namespace storm
