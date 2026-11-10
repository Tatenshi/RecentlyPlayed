#include "main.hpp"
#include "assets.hpp"
#include "HistoryLevelCategoryInfo.hpp"
#include "custom-types/shared/register.hpp"
#include "bsml/shared/Helpers/utilities.hpp"
#include "System/Collections/Generic/List_1.hpp"
#include "System/Type.hpp"

#include <algorithm>

using LevelCategory = GlobalNamespace::SelectLevelCategoryViewController::LevelCategory;
using LevelCategoryInfo = GlobalNamespace::SelectLevelCategoryViewController::LevelCategoryInfo;

static RecentlyPlayed::HistoryLevelCategoryInfo* GetHistoryCategoryInfo(GlobalNamespace::SelectLevelCategoryViewController* self)
{
    if(!self || !self->_allLevelCategoryInfos) return nullptr;
    for(auto* info : self->_allLevelCategoryInfos)
    {
        if(auto* historyInfo = i2c::try_cast<RecentlyPlayed::HistoryLevelCategoryInfo*>(info))
        {
            return historyInfo;
        }
    }
    return nullptr;
}

static bool disableVCScroll = false;

MAKE_HOOK_MATCH(SelectLevelCategoryViewControllerSetupHook, &GlobalNamespace::SelectLevelCategoryViewController::Setup, void, GlobalNamespace::SelectLevelCategoryViewController *self, GlobalNamespace::SelectLevelCategoryViewController::LevelCategory selectedCategory, ArrayW<GlobalNamespace::SelectLevelCategoryViewController::LevelCategory> enabledLevelCategories)
{
    auto* historyLevelCategoryInfo = GetHistoryCategoryInfo(self);
    if(!historyLevelCategoryInfo)
    {
        // Reserve a value above all built-in and currently registered categories.
        auto nextCategoryValue = LevelCategory::None.value__;
        auto builtInCategories = ArrayW<LevelCategory>(static_cast<System::Type*>(i2c::cs_type_of<LevelCategory>())->GetEnumValues());
        for(auto category : builtInCategories)
        {
            nextCategoryValue = std::max(nextCategoryValue, category.value__);
        }
        for(auto* info : self->_allLevelCategoryInfos)
        {
            nextCategoryValue = std::max(nextCategoryValue, info->levelCategory.value__);
        }
        for(auto category : enabledLevelCategories)
        {
            nextCategoryValue = std::max(nextCategoryValue, category.value__);
        }

        historyLevelCategoryInfo = RecentlyPlayed::HistoryLevelCategoryInfo::New_ctor();
        historyLevelCategoryInfo->levelCategory = LevelCategory(nextCategoryValue + 1);
        historyLevelCategoryInfo->localizedKey = "";
        historyLevelCategoryInfo->categoryIcon = PNG_SPRITE(icon);

        auto extendedLevelCategoryInfos = ArrayW<LevelCategoryInfo*>(self->_allLevelCategoryInfos.size() + 1);
        self->_allLevelCategoryInfos.copy_to(extendedLevelCategoryInfos, 0);
        extendedLevelCategoryInfos.back() = historyLevelCategoryInfo;
        self->_allLevelCategoryInfos = extendedLevelCategoryInfos;
    }

    const auto category = historyLevelCategoryInfo->levelCategory;
    if(std::find(enabledLevelCategories.begin(), enabledLevelCategories.end(), category) == enabledLevelCategories.end())
    {
        auto extendedLevelCategories = ArrayW<LevelCategory>(enabledLevelCategories.size() + 1);
        enabledLevelCategories.copy_to(extendedLevelCategories, 0);
        extendedLevelCategories.back() = category;
        enabledLevelCategories = extendedLevelCategories;
    }

    SelectLevelCategoryViewControllerSetupHook(self, selectedCategory, enabledLevelCategories);
}

MAKE_HOOK_MATCH(SelectLevelCategoryViewControllerDidActivateHook, &GlobalNamespace::SelectLevelCategoryViewController::DidActivate, void, GlobalNamespace::SelectLevelCategoryViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling)
{
    if(firstActivation)
    {
        // Resize the ViewController so we have enough click space to make all categorys usable
        self->get_rectTransform()->set_sizeDelta({45,15});
        // Move the Page selection a bit to the left, so that our new page has enough space
        self->_levelFilterCategoryIconSegmentedControl->get_transform()->set_localPosition({45,-7.5,0});
    }

    // Base Call
    SelectLevelCategoryViewControllerDidActivateHook(self, firstActivation, addedToHierarchy, screenSystemEnabling);
}

MAKE_HOOK_MATCH(ScrollViewScrollToHook, &HMUI::ScrollView::ScrollTo, void, HMUI::ScrollView *self, float x, bool animated)
{
    if(!disableVCScroll){
        ScrollViewScrollToHook(self, x, animated);
    }
}

MAKE_HOOK_MATCH(LevelCollectionNavigationControllerPresentViewControllersForPackHook, &GlobalNamespace::LevelCollectionNavigationController::PresentViewControllersForPack, void, GlobalNamespace::LevelCollectionNavigationController *self)
{
    if(!disableVCScroll){
        LevelCollectionNavigationControllerPresentViewControllersForPackHook(self);
    }
}

MAKE_HOOK_MATCH(LevelFilteringNavigationControllerDidActivateHook, &GlobalNamespace::LevelFilteringNavigationController::DidActivate, void, GlobalNamespace::LevelFilteringNavigationController *self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling)
{
    // Always refresh content, because if we are on the recently played tab the content can change when the player starts a song (needs to be prepended)
    // If we are added to hierarchy the base call does this for us
    const auto* historyInfo = GetHistoryCategoryInfo(self->_selectLevelCategoryViewController);
    if(!addedToHierarchy && historyInfo && self->_selectLevelCategoryViewController->get_selectedLevelCategory() == historyInfo->levelCategory)
    {
        // We need to temporarily disable the scrolling of the viewcontroller, which happens when setting new data
        disableVCScroll = true;
        self->UpdateSecondChildControllerContent(self->_selectLevelCategoryViewController->get_selectedLevelCategory());
        disableVCScroll = false;
    }

    // Base call
    LevelFilteringNavigationControllerDidActivateHook(self, firstActivation, addedToHierarchy, screenSystemEnabling);
}

MAKE_HOOK_MATCH(LevelFilteringNavigationControllerUpdateSecondChildControllerContentHook, &GlobalNamespace::LevelFilteringNavigationController::UpdateSecondChildControllerContent, void, GlobalNamespace::LevelFilteringNavigationController *self, GlobalNamespace::SelectLevelCategoryViewController::LevelCategory levelCategory)
{
    // If our history catergory is selected we cant call into base, as it would throw an argument exception in the switch
    const auto* historyInfo = GetHistoryCategoryInfo(self->_selectLevelCategoryViewController);
    if(historyInfo && levelCategory == historyInfo->levelCategory)
    {
        // Select the header of the songs
        self->_currentNoDataInfoPrefab = self->_emptyCustomSongListInfoPrefab;

        // Load History from config
        std::vector<std::string> historyIDVector = getModConfig().History.GetValue();

        // Filter Duplicates, if wanted
        if (getModConfig().FilterDuplicates.GetValue())
        {
            std::vector<std::string> uniqueIDs = {};
            for(auto levelid : historyIDVector)
            {
                if(std::find(uniqueIDs.begin(), uniqueIDs.end(), levelid) == uniqueIDs.end())
                {
                    uniqueIDs.push_back(levelid);
                }
            }
            historyIDVector = uniqueIDs;
        }

        // Load the PreviewBeatmapLevels from songloader api, if we still have it locally
        std::vector<GlobalNamespace::BeatmapLevel*> historyLevelVector = {};
        for(auto levelid : historyIDVector)
        {
            if(self->_beatmapLevelsModel)
            {
                GlobalNamespace::BeatmapLevel* beatmapLevel = self->_beatmapLevelsModel->GetBeatmapLevel(levelid, false);
                if (beatmapLevel)
                {
                    historyLevelVector.push_back(beatmapLevel);
                }
            }
        }

        // Package our new collection in a BeatmapLevelPack and push it into the ViewController
        // auto collection = GlobalNamespace::CustomBeatmapLevelCollection::New_ctor(il2cpp_utils::vectorToArray(historyLevelVector));
        auto historySprite = PNG_SPRITE(icon);
        auto historyLevelPack = SongCore::SongLoader::CustomLevelPack::New("custom_levelPack_HLP","Recently Played", historySprite);
        historyLevelPack->SetLevels(historyLevelVector);
        auto historyLevelPackList = System::Collections::Generic::List_1<GlobalNamespace::BeatmapLevelPack*>::New_ctor();
        historyLevelPackList->Add(i2c::cast<GlobalNamespace::BeatmapLevelPack*>(historyLevelPack));
        self->ShowPacksInSecondChildController(i2c::cast<System::Collections::Generic::IReadOnlyList_1<GlobalNamespace::BeatmapLevelPack*>*>(historyLevelPackList));
    }
    else
    {
        LevelFilteringNavigationControllerUpdateSecondChildControllerContentHook(self, levelCategory);
    }
    // Make sure, the Playlist/Package selection is active, when we are not on the history tab (only relevant for tab 1 or 2 (Favorites and Filter doesnt have it anyways))
    if(self->_annotatedBeatmapLevelCollectionsViewController)
    {
        self->_annotatedBeatmapLevelCollectionsViewController->get_gameObject()->set_active(levelCategory.value__ <= 2);
    }
}

MAKE_HOOK_MATCH(GameplayCoreInstallBindings, &GlobalNamespace::GameplayCoreInstaller::InstallBindings, void, GlobalNamespace::GameplayCoreInstaller* self)
{
    GameplayCoreInstallBindings(self);

    // We can record the current play if replay is not installed, we are not in a replay or we are allowed to record replays
    if(!ReplayInstalled() || !IsInReplay() || getModConfig().RecordReplay.GetValue())
    {
        // Record the played song in our mod config
        auto historyVector = getModConfig().History.GetValue();
        historyVector.insert(historyVector.begin(), self->_sceneSetupData->beatmapLevel->levelID);
        // Ensure, that we dont exceed the maximum size
        historyVector.resize(getModConfig().Length.GetValue());
        getModConfig().History.SetValue(historyVector);
    }
}


void DidActivate(HMUI::ViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling){
    if(firstActivation){
        // Make Touchable
        self->get_gameObject()->AddComponent<HMUI::Touchable*>();

        // Create Container
        auto* container = BSML::Lite::CreateScrollableSettingsContainer(self->get_transform());

        // Add Options
        AddConfigValueToggle(container->get_transform(), getModConfig().FilterDuplicates);
        // This toggle makes no sense, if replay is not installed. So we just disable it
        if(ReplayInstalled())
        {
            AddConfigValueToggle(container->get_transform(), getModConfig().RecordReplay);
        }
        AddConfigValueIncrementInt(container->get_transform(), getModConfig().Length, 1, 1, 500);
        BSML::Lite::CreateUIButton(container->get_transform(), "Clear History", []() {
            getModConfig().History.SetValue({});
        });
    }
}

extern "C" __attribute__((visibility("default"))) void setup(CModInfo* info)
{
    info->version = VERSION;
    info->id = MOD_ID;
    info->version_long = 0;
    modInfo.assign(*info);

    // Init things
    getModConfig().Init(modInfo);
    getLogger().info("Completed setup!");
}

// Called later on in the game loading - a good time to install function hooks
extern "C" __attribute__((visibility("default"))) void late_load()
{
    i2c::functions::initialize();
    custom_types::Register::AutoRegister();

    // Register our Settings
    BSML::Register::RegisterSettingsMenu("RecentlyPlayed", DidActivate, true);

    auto logger = Paper::ConstLoggerContext("RecentlyPlayed");
    // Install Hooks
    getLogger().info("Installing hooks...");
    INSTALL_HOOK(logger, SelectLevelCategoryViewControllerSetupHook);
    INSTALL_HOOK(logger, SelectLevelCategoryViewControllerDidActivateHook);
    INSTALL_HOOK_ORIG(logger, LevelFilteringNavigationControllerUpdateSecondChildControllerContentHook);
    INSTALL_HOOK(logger, GameplayCoreInstallBindings);
    INSTALL_HOOK(logger, LevelFilteringNavigationControllerDidActivateHook);
    INSTALL_HOOK_ORIG(logger, ScrollViewScrollToHook);
    INSTALL_HOOK_ORIG(logger, LevelCollectionNavigationControllerPresentViewControllersForPackHook);
    getLogger().info("Installed all hooks!");
}
