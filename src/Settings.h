// ReSharper disable CppDFAConstantParameter
#pragma once

class Settings : public REX::Singleton<Settings>
{
public:
    void Load()
    {
        const auto path = std::format("Data/SKSE/Plugins/{}.ini", Plugin::NAME);

        CSimpleIniA ini;
        ini.SetUnicode();

        ini.LoadFile(path.c_str());

        get_value(ini, debugLogging, "General", "bDebugLogging", "; Toggle debug logging");
        get_value(ini, useReadyWeaponButton, "General", "bUseReadyWeaponButton", "; Use the Ready Weapon (by default: R) button to trigger the attack cancel (works better in general and more compatible with mods like One Click Power Attack)");
        get_value(ini, useBlockButton, "General", "bUseBlockButton", "; Use the Block (by default: Mouse Right Click) button to trigger the attack cancel");
        get_value(ini, staminaCost1H, "General", "StaminaCost1H", "; Stamina cost when using 1H weapons");
        get_value(ini, staminaCost2H, "General", "StaminaCost2H", "; Stamina cost when using 2H weapons");
        get_value(ini, restrictCancelWindow, "General", "RestrictCancelWindow", "; Only allow cancelling during early attack phase");
        get_value(ini, cancelWindow1H, "General", "CancelWindow1H", "; Max ms to allow cancel after 1H attack starts");
        get_value(ini, cancelWindow2H, "General", "CancelWindow2H", "; Max ms to allow cancel after 2H attack starts");
        get_value(ini, perkModFileName, "General", "perkModFileName", "; Plugin name containing the perk");
        std::string formStr;
        get_value(ini, formStr, "General", "perkFormID", "...");

        if (!formStr.empty()) {
            perkFormId = std::stoul(formStr, nullptr, 0);    // base 0 = auto-detect hex
        }
        (void)ini.SaveFile(path.c_str());

        logger::info("Restrictions, perk mod: {}, perk form: {}", perkModFileName, perkFormId);
    }

    void ResolveForms()
    {
        cancelPerk = nullptr;

        if (perkModFileName.empty() || perkFormId == 0) {
            logger::info("No restriction perk configured");
            return;
        }

        auto* dataHandler = RE::TESDataHandler::GetSingleton();
        if (!dataHandler) {
            logger::error("TESDataHandler not available");
            return;
        }

        cancelPerk = skyrim_cast<RE::BGSPerk*>(
            dataHandler->LookupForm(perkFormId, perkModFileName));

        if (cancelPerk) {
            logger::info("Successfully resolved restriction perk");
        } else {
            logger::error("Failed to resolve restriction perk ({} | 0x{:X})",
                          perkModFileName,
                          perkFormId);
        }
    }
    // members
    bool debugLogging{false};
    bool useReadyWeaponButton{true};
    bool useBlockButton{true};
    float staminaCost1H{0.0f};
    float staminaCost2H{0.0f};
    bool restrictCancelWindow{false};
    uint32_t cancelWindow1H{0};
    uint32_t cancelWindow2H{0};
    std::string perkModFileName{""};
    uint32_t perkFormId{0};
    RE::BGSPerk* cancelPerk{nullptr};

private:
    template <class T>
    static void get_value(CSimpleIniA& a_ini, T& a_value, const char* a_section, const char* a_key, const char* a_comment)
    {
        clib_util::ini::get_value(a_ini, a_value, a_section, a_key, a_comment);
    }

    static void get_value(CSimpleIniA& a_ini, std::vector<std::pair<std::string, uint32_t>>& a_value, const char* a_section, const char* a_key, const char* a_comment)
    {
        std::vector<std::string> raw;
        raw.reserve(a_value.size());
        for (auto&& [plugin, id] : a_value)
            raw.emplace_back(std::format("{}|0x{:X}", plugin, id));

        clib_util::ini::get_value(a_ini, raw, a_section, a_key, a_comment, ",");

        a_value.clear();
        for (auto&& entry : raw)
            if (auto parsed = stl::detail::parse_plugin_form(entry))
                a_value.emplace_back(std::move(*parsed));
    }
};
