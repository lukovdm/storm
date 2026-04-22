#include "storm/settings/modules/ConditionalSettings.h"

#include "storm/settings/ArgumentBuilder.h"
#include "storm/settings/Option.h"
#include "storm/settings/OptionBuilder.h"
#include "storm/settings/SettingsManager.h"

namespace storm::settings::modules {

const std::string ConditionalSettings::moduleName = "conditional";
const std::string ConditionalSettings::conditionalAlgorithmOptionName = "conditional-algorithm";
const std::string ConditionalSettings::conditionalPrecisionOptionName = "precision";
const std::string ConditionalSettings::conditionalPrecisionRelativeOptionName = "relative";

ConditionalSettings::ConditionalSettings() : ModuleSettings(moduleName) {
    std::vector<std::string> const conditionalAlgs = {"default", "restart", "bisection", "bisection-advanced", "bisection-pt", "bisection-advanced-pt", "pi"};
    this->addOption(storm::settings::OptionBuilder(moduleName, conditionalAlgorithmOptionName, false, "The used algorithm for conditional probabilities.")
                        .setIsAdvanced()
                        .addArgument(storm::settings::ArgumentBuilder::createStringArgument("name", "The name of the method to use.")
                                         .addValidatorString(ArgumentValidatorFactory::createMultipleChoiceValidator(conditionalAlgs))
                                         .setDefaultValueString("default")
                                         .build())
                        .build());

    this->addOption(storm::settings::OptionBuilder(moduleName, conditionalPrecisionOptionName, false,
                                                   "The internally used precision for computing conditional probabilities..")
                        .setIsAdvanced()
                        .addArgument(storm::settings::ArgumentBuilder::createDoubleArgument("value", "The precision to use.")
                                         .setDefaultValueDouble(1e-06)
                                         .addValidatorDouble(ArgumentValidatorFactory::createDoubleRangeValidatorIncluding(0.0, 1.0))
                                         .build())
                        .build());

    this->addOption(storm::settings::OptionBuilder(moduleName, conditionalPrecisionRelativeOptionName, false,
                                                   "Whether the precision for computing conditional probabilities is considered relative.")
                        .setIsAdvanced()
                        .build());
}

bool ConditionalSettings::isConditionalAlgorithmSet() const {
    return this->getOption(conditionalAlgorithmOptionName).getHasOptionBeenSet();
}

ConditionalAlgorithmSetting ConditionalSettings::getConditionalAlgorithmSetting() const {
    return conditionalAlgorithmSettingFromString(this->getOption(conditionalAlgorithmOptionName).getArgumentByName("name").getValueAsString());
}

double ConditionalSettings::getConditionalPrecision() const {
    return this->getOption(conditionalPrecisionOptionName).getArgumentByName("value").getValueAsDouble();
}

bool ConditionalSettings::isConditionalPrecisionSetFromDefaultValue() const {
    return !this->getOption(conditionalPrecisionOptionName).getArgumentByName("value").getHasBeenSet() ||
           this->getOption(conditionalPrecisionOptionName).getArgumentByName("value").wasSetFromDefaultValue();
}

bool ConditionalSettings::isConditionalPrecisionRelative() const {
    return this->getOption(conditionalPrecisionRelativeOptionName).getHasOptionBeenSet();
}

}  // namespace storm::settings::modules