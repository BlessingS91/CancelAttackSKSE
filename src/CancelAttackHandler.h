#pragma once

class CancelAttackHandler : public REX::Singleton<CancelAttackHandler>
{
public:
    enum class TriggerType { kNone, kReadyWeapon, kBlock };

    void UpdateAttackState(const RE::PlayerCharacter* player);
    void UpdateKeyMappings();
    void CancelAttack(RE::PlayerCharacter* player, RE::INPUT_DEVICE dev, std::uint32_t key);
    [[nodiscard]] bool ProcessEvent(RE::InputEvent* const* a_event);

    [[nodiscard]] static bool HasTwoHandedWeaponEquipped(const RE::PlayerCharacter* player);
    [[nodiscard]] bool IsRestrictedCancelWindow(const RE::PlayerCharacter* player) const;
    [[nodiscard]] static float GetStaminaCost(const RE::PlayerCharacter* player);
    [[nodiscard]] static bool TryConsumeStamina(RE::PlayerCharacter* player);
    [[nodiscard]] TriggerType MatchTrigger(RE::INPUT_DEVICE dev, std::uint32_t key) const;

private:
    struct KeyMapping {
        std::uint32_t keyboard = 255;
        std::uint32_t mouse = 255;
        std::uint32_t gamepad = 255;
    };

    KeyMapping blockMapping;
    KeyMapping readyWeaponMapping;

    bool isKeysMapped = false;

    std::optional<std::pair<RE::INPUT_DEVICE, std::uint32_t>> triggerKey;

    std::chrono::steady_clock::time_point lastAttackStartTime{};
    bool wasAttacking = false;

    void HandleTriggerKeyRelease(RE::InputEvent* const* a_event, RE::PlayerCharacter* player);
    bool HandleCancelAttempt(RE::InputEvent* const* a_event, RE::PlayerCharacter* player);
};
