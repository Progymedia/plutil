// Copyright 2013-present Facebook. All Rights Reserved.

#include <pbxsetting/pbxsetting.h>
#include <xcsdk/xcsdk.h>
#include <pbxproj/pbxproj.h>
#include <pbxspec/pbxspec.h>

int
main(int argc, char **argv)
{
    if (argc < 4) {
        fprintf(stderr, "usage: %s developer project specs\n", argv[0]);
        return -1;
    }

    std::shared_ptr<xcsdk::SDK::Manager> xcsdk_manager = xcsdk::SDK::Manager::Open(argv[1]);
    if (!xcsdk_manager) {
        fprintf(stderr, "no developer dir found at %s\n", argv[1]);
        return -1;
    }

    auto project = pbxproj::PBX::Project::Open(argv[2]);
    if (!project) {
        fprintf(stderr, "error opening project at %s (%s)\n", argv[2], strerror(errno));
        return -1;
    }

    auto spec_manager = pbxspec::Manager::Open(argv[3]);
    if (!spec_manager) {
        fprintf(stderr, "error opening specifications at %s (%s)\n", argv[3], strerror(errno));
        return -1;
    }

    std::vector<pbxsetting::Level> levels;

    printf("Project: %s\n", project->name().c_str());

    auto projectConfigurationList = project->buildConfigurationList();
    std::string projectConfigurationName = projectConfigurationList->defaultConfigurationName();
    auto projectConfiguration = *std::find_if(projectConfigurationList->begin(), projectConfigurationList->end(), [&](pbxproj::XC::BuildConfiguration::shared_ptr configuration) -> bool {
        return configuration->name() == projectConfigurationName;
    });
    printf("Project Configuration: %s\n", projectConfiguration->name().c_str());

    auto target = project->targets().front();
    printf("Target: %s\n", target->name().c_str());

    auto targetConfigurationList = target->buildConfigurationList();
    std::string targetConfigurationName = targetConfigurationList->defaultConfigurationName();
    auto targetConfiguration = *std::find_if(targetConfigurationList->begin(), targetConfigurationList->end(), [&](pbxproj::XC::BuildConfiguration::shared_ptr configuration) -> bool {
        return configuration->name() == targetConfigurationName;
    });
    printf("Target Configuration: %s\n", targetConfiguration->name().c_str());

    auto platform = *std::find_if(xcsdk_manager->platforms().begin(), xcsdk_manager->platforms().end(), [](std::shared_ptr<xcsdk::SDK::Platform> platform) -> bool {
        return platform->name() == "iphoneos";
    });
    printf("Platform: %s\n", platform->name().c_str());

    auto sdk = platform->targets().front();
    printf("SDK: %s\n", sdk->displayName().c_str());

    pbxsetting::Level specDefaultSettings = spec_manager->defaultSettings();

    // TODO(grp): targetConfiguration->baseConfigurationReference()
    levels.push_back(targetConfiguration->buildSettings());
    levels.push_back(target->settings());
    // TODO(grp): projectConfiguration->baseConfigurationReference()
    levels.push_back(projectConfiguration->buildSettings());
    levels.push_back(project->settings());

    levels.push_back(platform->overrideProperties());
    levels.push_back(specDefaultSettings);
    levels.push_back(sdk->customProperties());
    levels.push_back(sdk->settings());
    levels.push_back(platform->settings());
    levels.push_back(sdk->defaultProperties());
    levels.push_back(platform->defaultProperties());
    levels.push_back(xcsdk_manager->computedSettings());
    // TODO(grp): system defaults?
    levels.push_back(pbxsetting::DefaultSettings::Environment());
    levels.push_back(pbxsetting::DefaultSettings::Internal());
    levels.push_back(pbxsetting::DefaultSettings::System());
    levels.push_back(pbxsetting::DefaultSettings::Build());

    pbxsetting::Environment environment = pbxsetting::Environment(levels, levels);

    pbxsetting::Condition condition = pbxsetting::Condition({
        { "sdk", sdk->canonicalName() },
        { "arch", "arm64" }, // TODO(grp): Use a real architcture.
        { "variant", "default" },
    });

    std::unordered_map<std::string, std::string> values = environment.computeValues(condition);
    std::map<std::string, std::string> orderedValues = std::map<std::string, std::string>(values.begin(), values.end());

    printf("\n\nBuild Settings:\n\n");
    for (auto const &value : orderedValues) {
        auto defaultValue = specDefaultSettings.get(value.first, condition);
        if (!defaultValue.first || value.second != defaultValue.second.raw()) {
            printf("    %s = %s\n", value.first.c_str(), value.second.c_str());
        }
    }

    return 0;
}
