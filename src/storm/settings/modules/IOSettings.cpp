#include "storm/settings/modules/IOSettings.h"

#include "storm/settings/SettingsManager.h"
#include "storm/settings/SettingMemento.h"
#include "storm/settings/Option.h"
#include "storm/settings/OptionBuilder.h"
#include "storm/settings/ArgumentBuilder.h"
#include "storm/settings/Argument.h"
#include "storm/exceptions/InvalidSettingsException.h"
#include "storm/parser/CSVParser.h"

#include "storm/utility/macros.h"
#include "storm/exceptions/IllegalArgumentValueException.h"

namespace storm {
    namespace settings {
        namespace modules {
            
            const std::string IOSettings::moduleName = "io";
            const std::string IOSettings::exportDotOptionName = "exportdot";
            const std::string IOSettings::exportExplicitOptionName = "exportexplicit";
            const std::string IOSettings::exportJaniDotOptionName = "exportjanidot";
            const std::string IOSettings::exportCdfOptionName = "exportcdf";
            const std::string IOSettings::exportCdfOptionShortName = "cdf";
            const std::string IOSettings::explicitOptionName = "explicit";
            const std::string IOSettings::explicitOptionShortName = "exp";
            const std::string IOSettings::explicitDrnOptionName = "explicit-drn";
            const std::string IOSettings::explicitDrnOptionShortName = "drn";
            const std::string IOSettings::explicitImcaOptionName = "explicit-imca";
            const std::string IOSettings::explicitImcaOptionShortName = "imca";
            const std::string IOSettings::prismInputOptionName = "prism";
            const std::string IOSettings::janiInputOptionName = "jani";
            const std::string IOSettings::prismToJaniOptionName = "prism2jani";

            const std::string IOSettings::transitionRewardsOptionName = "transrew";
            const std::string IOSettings::stateRewardsOptionName = "staterew";
            const std::string IOSettings::choiceLabelingOptionName = "choicelab";
            const std::string IOSettings::constantsOptionName = "constants";
            const std::string IOSettings::constantsOptionShortName = "const";

            const std::string IOSettings::janiPropertyOptionName = "janiproperty";
            const std::string IOSettings::janiPropertyOptionShortName = "jprop";
            const std::string IOSettings::propertyOptionName = "prop";
            const std::string IOSettings::propertyOptionShortName = "prop";

            
            IOSettings::IOSettings() : ModuleSettings(moduleName) {
                this->addOption(storm::settings::OptionBuilder(moduleName, exportDotOptionName, "", "If given, the loaded model will be written to the specified file in the dot format.")
                                .addArgument(storm::settings::ArgumentBuilder::createStringArgument("filename", "The name of the file to which the model is to be written.").build()).build());
                this->addOption(storm::settings::OptionBuilder(moduleName, exportJaniDotOptionName, "", "If given, the loaded jani model will be written to the specified file in the dot format.")
                                        .addArgument(storm::settings::ArgumentBuilder::createStringArgument("filename", "The name of the file to which the model is to be written.").build()).build());
                this->addOption(storm::settings::OptionBuilder(moduleName, exportCdfOptionName, false, "Exports the cumulative density function for reward bounded properties into a .csv file.").setShortName(exportCdfOptionShortName).addArgument(storm::settings::ArgumentBuilder::createStringArgument("directory", "A path to an existing directory where the cdf files will be stored.").build()).build());
                this->addOption(storm::settings::OptionBuilder(moduleName, exportExplicitOptionName, "", "If given, the loaded model will be written to the specified file in the drn format.")
                                .addArgument(storm::settings::ArgumentBuilder::createStringArgument("filename", "the name of the file to which the model is to be writen.").build()).build());
                this->addOption(storm::settings::OptionBuilder(moduleName, explicitOptionName, false, "Parses the model given in an explicit (sparse) representation.").setShortName(explicitOptionShortName)
                                .addArgument(storm::settings::ArgumentBuilder::createStringArgument("transition filename", "The name of the file from which to read the transitions.").addValidatorString(ArgumentValidatorFactory::createExistingFileValidator()).build())
                                .addArgument(storm::settings::ArgumentBuilder::createStringArgument("labeling filename", "The name of the file from which to read the state labeling.").addValidatorString(ArgumentValidatorFactory::createExistingFileValidator()).build()).build());
                this->addOption(storm::settings::OptionBuilder(moduleName, explicitDrnOptionName, false, "Parses the model given in the DRN format.").setShortName(explicitDrnOptionShortName)
                                .addArgument(storm::settings::ArgumentBuilder::createStringArgument("drn filename", "The name of the DRN file containing the model.").addValidatorString(ArgumentValidatorFactory::createExistingFileValidator()).build())
                                .build());
                this->addOption(storm::settings::OptionBuilder(moduleName, explicitImcaOptionName, false, "Parses the model given in the IMCA format.").setShortName(explicitImcaOptionShortName)
                                .addArgument(storm::settings::ArgumentBuilder::createStringArgument("imca filename", "The name of the imca file containing the model.").addValidatorString(ArgumentValidatorFactory::createExistingFileValidator()).build())
                                .build());
                this->addOption(storm::settings::OptionBuilder(moduleName, prismInputOptionName, false, "Parses the model given in the PRISM format.")
                                .addArgument(storm::settings::ArgumentBuilder::createStringArgument("filename", "The name of the file from which to read the PRISM input.").addValidatorString(ArgumentValidatorFactory::createExistingFileValidator()).build()).build());
                this->addOption(storm::settings::OptionBuilder(moduleName, janiInputOptionName, false, "Parses the model given in the JANI format.")
                                .addArgument(storm::settings::ArgumentBuilder::createStringArgument("filename", "The name of the file from which to read the JANI input.").addValidatorString(ArgumentValidatorFactory::createExistingFileValidator()).build()).build());
                this->addOption(storm::settings::OptionBuilder(moduleName, prismToJaniOptionName, false, "If set, the input PRISM model is transformed to JANI.").build());
                this->addOption(storm::settings::OptionBuilder(moduleName, propertyOptionName, false, "Specifies the properties to be checked on the model.").setShortName(propertyOptionShortName)
                                        .addArgument(storm::settings::ArgumentBuilder::createStringArgument("property or filename", "The formula or the file containing the formulas.").build())
                                        .addArgument(storm::settings::ArgumentBuilder::createStringArgument("filter", "The names of the properties to check.").setDefaultValueString("all").build())
                                        .build());

                this->addOption(storm::settings::OptionBuilder(moduleName, transitionRewardsOptionName, false, "If given, the transition rewards are read from this file and added to the explicit model. Note that this requires the model to be given as an explicit model (i.e., via --" + explicitOptionName + ").")
                                .addArgument(storm::settings::ArgumentBuilder::createStringArgument("filename", "The file from which to read the transition rewards.").addValidatorString(ArgumentValidatorFactory::createExistingFileValidator()).build()).build());
                this->addOption(storm::settings::OptionBuilder(moduleName, stateRewardsOptionName, false, "If given, the state rewards are read from this file and added to the explicit model. Note that this requires the model to be given as an explicit model (i.e., via --" + explicitOptionName + ").")
                                .addArgument(storm::settings::ArgumentBuilder::createStringArgument("filename", "The file from which to read the state rewards.").addValidatorString(ArgumentValidatorFactory::createExistingFileValidator()).build()).build());
                this->addOption(storm::settings::OptionBuilder(moduleName, choiceLabelingOptionName, false, "If given, the choice labels are read from this file and added to the explicit model. Note that this requires the model to be given as an explicit model (i.e., via --" + explicitOptionName + ").")
                                .addArgument(storm::settings::ArgumentBuilder::createStringArgument("filename", "The file from which to read the choice labels.").addValidatorString(ArgumentValidatorFactory::createExistingFileValidator()).build()).build());
                this->addOption(storm::settings::OptionBuilder(moduleName, constantsOptionName, false, "Specifies the constant replacements to use in symbolic models. Note that this requires the model to be given as an symbolic model (i.e., via --" + prismInputOptionName + " or --" + janiInputOptionName + ").").setShortName(constantsOptionShortName)
                                .addArgument(storm::settings::ArgumentBuilder::createStringArgument("values", "A comma separated list of constants and their value, e.g. a=1,b=2,c=3.").setDefaultValueString("").build()).build());
                this->addOption(storm::settings::OptionBuilder(moduleName, janiPropertyOptionName, false, "Specifies the properties from the jani model (given by --" + janiInputOptionName + ")  to be checked.").setShortName(janiPropertyOptionShortName)
                                .addArgument(storm::settings::ArgumentBuilder::createStringArgument("values", "A comma separated list of properties to be checked").setDefaultValueString("").build()).build());
            }

            bool IOSettings::isExportDotSet() const {
                return this->getOption(exportDotOptionName).getHasOptionBeenSet();
            }

            std::string IOSettings::getExportDotFilename() const {
                return this->getOption(exportDotOptionName).getArgumentByName("filename").getValueAsString();
            }

            bool IOSettings::isExportJaniDotSet() const {
                return this->getOption(exportJaniDotOptionName).getHasOptionBeenSet();
            }

            std::string IOSettings::getExportJaniDotFilename() const {
                return this->getOption(exportJaniDotOptionName).getArgumentByName("filename").getValueAsString();
            }

            bool IOSettings::isExportExplicitSet() const {
                return this->getOption(exportExplicitOptionName).getHasOptionBeenSet();
            }

            std::string IOSettings::getExportExplicitFilename() const {
                return this->getOption(exportExplicitOptionName).getArgumentByName("filename").getValueAsString();
            }
            
            bool IOSettings::isExportCdfSet() const {
                return this->getOption(exportCdfOptionName).getHasOptionBeenSet();
            }
            
            std::string IOSettings::getExportCdfDirectory() const {
                std::string result = this->getOption(exportCdfOptionName).getArgumentByName("directory").getValueAsString();
                if (result.back() != '/') {
                    result.push_back('/');
                }
                return result;
            }
            
            bool IOSettings::isExplicitSet() const {
                return this->getOption(explicitOptionName).getHasOptionBeenSet();
            }

            std::string IOSettings::getTransitionFilename() const {
                return this->getOption(explicitOptionName).getArgumentByName("transition filename").getValueAsString();
            }

            std::string IOSettings::getLabelingFilename() const {
                return this->getOption(explicitOptionName).getArgumentByName("labeling filename").getValueAsString();
            }

            bool IOSettings::isExplicitDRNSet() const {
                return this->getOption(explicitDrnOptionName).getHasOptionBeenSet();
            }

            std::string IOSettings::getExplicitDRNFilename() const {
                return this->getOption(explicitDrnOptionName).getArgumentByName("drn filename").getValueAsString();
            }

            bool IOSettings::isExplicitIMCASet() const {
                return this->getOption(explicitImcaOptionName).getHasOptionBeenSet();
            }

            std::string IOSettings::getExplicitIMCAFilename() const {
                return this->getOption(explicitImcaOptionName).getArgumentByName("imca filename").getValueAsString();
            }

            bool IOSettings::isPrismInputSet() const {
                return this->getOption(prismInputOptionName).getHasOptionBeenSet();
            }

            bool IOSettings::isPrismOrJaniInputSet() const {
                return isJaniInputSet() || isPrismInputSet();
            }

            bool IOSettings::isPrismToJaniSet() const {
                return this->getOption(prismToJaniOptionName).getHasOptionBeenSet();
            }

            std::string IOSettings::getPrismInputFilename() const {
                return this->getOption(prismInputOptionName).getArgumentByName("filename").getValueAsString();
            }

            bool IOSettings::isJaniInputSet() const {
                return this->getOption(janiInputOptionName).getHasOptionBeenSet();
            }

            std::string IOSettings::getJaniInputFilename() const {
                return this->getOption(janiInputOptionName).getArgumentByName("filename").getValueAsString();
            }

            bool IOSettings::isTransitionRewardsSet() const {
                return this->getOption(transitionRewardsOptionName).getHasOptionBeenSet();
            }

            std::string IOSettings::getTransitionRewardsFilename() const {
                return this->getOption(transitionRewardsOptionName).getArgumentByName("filename").getValueAsString();
            }

            bool IOSettings::isStateRewardsSet() const {
                return this->getOption(stateRewardsOptionName).getHasOptionBeenSet();
            }

            std::string IOSettings::getStateRewardsFilename() const {
                return this->getOption(stateRewardsOptionName).getArgumentByName("filename").getValueAsString();
            }

            bool IOSettings::isChoiceLabelingSet() const {
                return this->getOption(choiceLabelingOptionName).getHasOptionBeenSet();
            }

            std::string IOSettings::getChoiceLabelingFilename() const {
                return this->getOption(choiceLabelingOptionName).getArgumentByName("filename").getValueAsString();
            }

            bool IOSettings::isConstantsSet() const {
                return this->getOption(constantsOptionName).getHasOptionBeenSet();
            }

            std::string IOSettings::getConstantDefinitionString() const {
                return this->getOption(constantsOptionName).getArgumentByName("values").getValueAsString();
            }

            bool IOSettings::isJaniPropertiesSet() const {
                return this->getOption(janiPropertyOptionName).getHasOptionBeenSet();
            }

            bool IOSettings::areJaniPropertiesSelected() const {
                return this->getOption(janiPropertyOptionName).getHasOptionBeenSet() && (this->getOption(janiPropertyOptionName).getArgumentByName("values").getValueAsString() != "");
            }

            std::vector<std::string> IOSettings::getSelectedJaniProperties() const {
                return storm::parser::parseCommaSeperatedValues(this->getOption(janiPropertyOptionName).getArgumentByName("values").getValueAsString());
            }

            bool IOSettings::isPropertySet() const {
                return this->getOption(propertyOptionName).getHasOptionBeenSet();
            }

            std::string IOSettings::getProperty() const {
                return this->getOption(propertyOptionName).getArgumentByName("property or filename").getValueAsString();
            }

            std::string IOSettings::getPropertyFilter() const {
                return this->getOption(propertyOptionName).getArgumentByName("filter").getValueAsString();
            }

			void IOSettings::finalize() {
                // Intentionally left empty.
            }

            bool IOSettings::check() const {
                // Ensure that not two symbolic input models were given.
                STORM_LOG_THROW(!isJaniInputSet() || !isPrismInputSet(), storm::exceptions::InvalidSettingsException, "Symbolic model ");

                // Ensure that not two explicit input models were given.
                STORM_LOG_THROW(!isExplicitSet() || !isExplicitDRNSet(), storm::exceptions::InvalidSettingsException, "Explicit model ");

                STORM_LOG_THROW(!isExportJaniDotSet() || isJaniInputSet(), storm::exceptions::InvalidSettingsException, "Jani-to-dot export is only available for jani models" );

                // Ensure that the model was given either symbolically or explicitly.
                STORM_LOG_THROW(!isJaniInputSet() || !isPrismInputSet() || !isExplicitSet() || !isExplicitDRNSet(), storm::exceptions::InvalidSettingsException, "The model may be either given in an explicit or a symbolic format (PRISM or JANI), but not both.");
                
                // Make sure PRISM-to-JANI conversion is only set if the actual input is in PRISM format.
                STORM_LOG_THROW(!isPrismToJaniSet() || isPrismInputSet(), storm::exceptions::InvalidSettingsException, "For the transformation from PRISM to JANI, the input model must be given in the prism format.");
                
                return true;
            }

        } // namespace modules
    } // namespace settings
} // namespace storm
