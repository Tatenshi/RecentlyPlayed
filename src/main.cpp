#include "main.hpp"

static GlobalNamespace::SelectLevelCategoryViewController::LevelCategory historyCategorie = 5;

MAKE_HOOK_MATCH(SelectLevelCategoryViewControllerSetupHook, &GlobalNamespace::SelectLevelCategoryViewController::Setup, void, GlobalNamespace::SelectLevelCategoryViewController *self, GlobalNamespace::SelectLevelCategoryViewController::LevelCategory selectedCategory, ArrayW<GlobalNamespace::SelectLevelCategoryViewController::LevelCategory> enabledLevelCategories)
{
    if(self->allLevelCategoryInfos->Length() == 4)
    {
        // Create new history CategoryInfo
        auto historyLevelCategoryInfo = GlobalNamespace::SelectLevelCategoryViewController::LevelCategoryInfo::New_ctor();
        historyLevelCategoryInfo->levelCategory = historyCategorie;
        historyLevelCategoryInfo->localizedKey = "";
        historyLevelCategoryInfo->categoryIcon = QuestUI::BeatSaberUI::Base64ToSprite(historyIcon);

        // Extend allLevelCategoryinfos with our HistoryCategoryInfo
        auto extendedLevelCategorieInfos = Array<GlobalNamespace::SelectLevelCategoryViewController::LevelCategoryInfo *>::NewLength(5);
        self->allLevelCategoryInfos->CopyTo(extendedLevelCategorieInfos, 0);
        auto extendedLevelCategorieInfosW = ArrayW<GlobalNamespace::SelectLevelCategoryViewController::LevelCategoryInfo *>(extendedLevelCategorieInfos);
        extendedLevelCategorieInfosW[4] = historyLevelCategoryInfo;
        self->allLevelCategoryInfos = extendedLevelCategorieInfosW;        
    }

    // Extend EnabledCategories with our history category
    auto extendedLevelCategories = Array<GlobalNamespace::SelectLevelCategoryViewController::LevelCategory>::NewLength(enabledLevelCategories->Length() + 1);
    enabledLevelCategories.CopyTo(extendedLevelCategories, 0);
    auto extendedLevelCategoriesW = ArrayW<GlobalNamespace::SelectLevelCategoryViewController::LevelCategory>(extendedLevelCategories);
    extendedLevelCategoriesW[extendedLevelCategoriesW.Length() - 1] = historyCategorie;


    // Base Call
    SelectLevelCategoryViewControllerSetupHook(self, selectedCategory, extendedLevelCategoriesW);
}

MAKE_HOOK_MATCH(SelectLevelCategoryViewControllerDidActivateHook, &GlobalNamespace::SelectLevelCategoryViewController::DidActivate, void, GlobalNamespace::SelectLevelCategoryViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling)
{
    if(firstActivation)
    {
        // Resize the ViewController so we have enough click space to make all categorys usable
        self->get_rectTransform()->set_sizeDelta({45,15});
        // Move the Page selection a bit to the left, so that our new page has enough space
        self->levelFilterCategoryIconSegmentedControl->get_transform()->set_localPosition({45,-7.5,0});
    }

    // Base Call
    SelectLevelCategoryViewControllerDidActivateHook(self, firstActivation, addedToHierarchy, screenSystemEnabling);
}

MAKE_HOOK_MATCH(LevelFilteringNavigationControllerDidActivateHook, &GlobalNamespace::LevelFilteringNavigationController::DidActivate, void, GlobalNamespace::LevelFilteringNavigationController *self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling)
{
    // Always refresh content, because if we are on the recently played tab the content can change when the player starts a song (needs to be prepended)
    // If we are added to hierarchy the base call does this for us
    if(!addedToHierarchy && self->selectLevelCategoryViewController->get_selectedLevelCategory() == historyCategorie)
    {
        self->UpdateSecondChildControllerContent(self->selectLevelCategoryViewController->get_selectedLevelCategory());
    }

    // Base call
    LevelFilteringNavigationControllerDidActivateHook(self, firstActivation, addedToHierarchy, screenSystemEnabling);
}

MAKE_HOOK_MATCH(LevelFilteringNavigationControllerUpdateSecondChildControllerContentHook, &GlobalNamespace::LevelFilteringNavigationController::UpdateSecondChildControllerContent, void, GlobalNamespace::LevelFilteringNavigationController *self, GlobalNamespace::SelectLevelCategoryViewController::LevelCategory levelCategory)
{
    // If our history catergory is selected we cant call into base, as it would throw an argument exception in the switch
    if(levelCategory == historyCategorie)
    {
        // Select the header of the songs
        self->currentNoDataInfoPrefab = self->emptyCustomSongListInfoPrefab;

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
        std::vector<GlobalNamespace::CustomPreviewBeatmapLevel*> historyLevelVector = {};
        for(auto levelid : historyIDVector)
        {
            if(auto level = RuntimeSongLoader::API::GetLevelById(levelid))
            {
                historyLevelVector.push_back(level.value());
            }
        }

        // Package our new collection in a BeatmapLevelPack and push it into the ViewController
        auto collection = GlobalNamespace::CustomBeatmapLevelCollection::New_ctor(il2cpp_utils::vectorToArray(historyLevelVector));
        auto historySprite = QuestUI::BeatSaberUI::Base64ToSprite(historyIcon);
        auto historyLevelPack = GlobalNamespace::CustomBeatmapLevelPack::New_ctor("custom_levelPack_HLP","Recently Played","Recently Played", historySprite, historySprite ,collection);
        auto historyLevelPackList = System::Collections::Generic::List_1<GlobalNamespace::IBeatmapLevelPack*>::New_ctor();
        historyLevelPackList->Add(il2cpp_utils::cast<GlobalNamespace::IBeatmapLevelPack>(historyLevelPack));
        self->ShowPacksInSecondChildController(il2cpp_utils::cast<System::Collections::Generic::IReadOnlyList_1<GlobalNamespace::IBeatmapLevelPack*>>(historyLevelPackList));

        // Disable Package/Playlist selection, because it is useless in the history
        self->annotatedBeatmapLevelCollectionsViewController->get_gameObject()->set_active(false);
    }
    else
    {
        // Make sure, the Playlist/Package selection is active, when we are not on the history tab
        self->annotatedBeatmapLevelCollectionsViewController->get_gameObject()->set_active(true);
        // And just call base
        LevelFilteringNavigationControllerUpdateSecondChildControllerContentHook(self, levelCategory);
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
        historyVector.insert(historyVector.begin(), il2cpp_utils::cast<GlobalNamespace::IPreviewBeatmapLevel>(self->sceneSetupData->difficultyBeatmap->get_level())->get_levelID());
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
        auto* container = QuestUI::BeatSaberUI::CreateScrollableSettingsContainer(self->get_transform());
        
        // Add Options
        AddConfigValueToggle(container->get_transform(), getModConfig().FilterDuplicates);
        // This toggle makes no sense, if replay is not installed. So we just disable it
        if(ReplayInstalled())
        {
            AddConfigValueToggle(container->get_transform(), getModConfig().RecordReplay);
        }
        AddConfigValueIncrementInt(container->get_transform(), getModConfig().Length, 1, 1, 500);
        QuestUI::BeatSaberUI::CreateUIButton(container->get_transform(), "Clear History", []() {
            getModConfig().History.SetValue({});
        });
    }
}

// Called at the early stages of game loading
extern "C" void setup(ModInfo &info)
{
    info.id = MOD_ID;
    info.version = VERSION;
    modInfo = info;

    getConfig().Load(); // Load the config file
    getLogger().info("Completed setup!");
}

// Called later on in the game loading - a good time to install function hooks
extern "C" void load()
{
    // Init things
    getModConfig().Init(modInfo);
    il2cpp_functions::Init();
    QuestUI::Init();

    // Register our Settings
    QuestUI::Register::RegisterModSettingsViewController(modInfo, DidActivate);

    // Install Hooks
    getLogger().info("Installing hooks...");
    INSTALL_HOOK(getLogger(), SelectLevelCategoryViewControllerSetupHook);
    INSTALL_HOOK(getLogger(), SelectLevelCategoryViewControllerDidActivateHook);
    INSTALL_HOOK_ORIG(getLogger(), LevelFilteringNavigationControllerUpdateSecondChildControllerContentHook);
    INSTALL_HOOK(getLogger(), GameplayCoreInstallBindings);
    INSTALL_HOOK(getLogger(), LevelFilteringNavigationControllerDidActivateHook);
    getLogger().info("Installed all hooks!");
}