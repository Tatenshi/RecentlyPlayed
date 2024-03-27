#pragma once

// beatsaber-hook is a modding framework that lets us call functions and fetch field values from in the game
// It also allows creating objects, configuration, and importantly, hooking methods to modify their values
#include "beatsaber-hook/shared/utils/logging.hpp"
#include "beatsaber-hook/shared/config/config-utils.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-functions.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"

// Includes for the functionality of the mod
#include "GlobalNamespace/SelectLevelCategoryViewController.hpp"
#include "GlobalNamespace/LevelFilteringNavigationController.hpp"
#include "GlobalNamespace/LevelCollectionNavigationController.hpp"
#include "GlobalNamespace/AnnotatedBeatmapLevelCollectionsViewController.hpp"
#include "GlobalNamespace/GameplayCoreInstaller.hpp"
#include "GlobalNamespace/GameCoreSceneSetupData.hpp"
#include "GlobalNamespace/GameplayCoreSceneSetupData.hpp"
#include "HMUI/IconSegmentedControl.hpp"
#include "HMUI/ScrollView.hpp"
#include "System/Collections/Generic/List_1.hpp"
#include "System/Collections/Generic/IReadOnlyList_1.hpp"
#include "System/Array.hpp"
#include "HMUI/Touchable.hpp"

#include "bsml/shared/BSML.hpp"

#include "conditional-dependencies/shared/main.hpp"

#include "songcore/shared/SongCore.hpp"

#include "include/logger.hpp"
#include "include/config.hpp"
#include "include/icon.hpp"

void DidActivate(HMUI::ViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling);

inline bool IsInReplay() noexcept
{
    static auto function = CondDeps::FindUnsafe<bool>("replay", "IsInReplay");
    return function.value()();
}
inline bool ReplayInstalled() noexcept
{
    return !!CondDeps::FindUnsafe<bool, std::string>("replay", "PlayBSORFromFile");
}