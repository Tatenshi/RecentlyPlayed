#pragma once

// beatsaber-hook is a modding framework that lets us call functions and fetch field values from in the game
// It also allows creating objects, configuration, and importantly, hooking methods to modify their values
#include "beatsaber-hook/shared/utils/logging.hpp"
#include "beatsaber-hook/shared/config/config-utils.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-functions.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"

// Includes for the functionality of the mod
#include "GlobalNamespace/SelectLevelCategoryViewController.hpp"
#include "GlobalNamespace/SelectLevelCategoryViewController_LevelCategoryInfo.hpp"
#include "GlobalNamespace/LevelFilteringNavigationController.hpp"
#include "GlobalNamespace/LevelCollectionNavigationController.hpp"
#include "GlobalNamespace/IBeatmapLevelPack.hpp"
#include "GlobalNamespace/AnnotatedBeatmapLevelCollectionsViewController.hpp"
#include "GlobalNamespace/GameplayCoreInstaller.hpp"
#include "GlobalNamespace/GameCoreSceneSetupData.hpp"
#include "GlobalNamespace/GameplayCoreSceneSetupData_-GetTransformedBeatmapDataAsync-d__15.hpp"
#include "GlobalNamespace/GameplayCoreSceneSetupData_-LoadTransformedBeatmapDataAsync-d__14.hpp"
#include "GlobalNamespace/IDifficultyBeatmap.hpp"
#include "GlobalNamespace/IBeatmapLevel.hpp"
#include "GlobalNamespace/IPreviewBeatmapLevel.hpp"
#include "HMUI/IconSegmentedControl.hpp"
#include "HMUI/ScrollView.hpp"
#include "System/Collections/Generic/List_1.hpp"
#include "System/Collections/Generic/IReadOnlyList_1.hpp"
#include "System/Array.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "questui/shared/QuestUI.hpp"
#include "HMUI/Touchable.hpp"

// Include the modloader header, which allows us to tell the modloader which mod this is, and the version etc.
#include "modloader/shared/modloader.hpp"

#include "conditional-dependencies/shared/main.hpp"

#include "songloader/shared/API.hpp"

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