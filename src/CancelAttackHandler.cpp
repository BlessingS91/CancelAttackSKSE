#include "CancelAttackHandler.h"
#include "Settings.h"

void CancelAttackHandler::UpdateAttackState(const RE::PlayerCharacter* player)
{
    const bool isAttacking = player->IsAttacking();
    if (isAttacking && !wasAttacking) {
        lastAttackStartTime = std::chrono::steady_clock::now();
    }
    wasAttacking = isAttacking;
}

void CancelAttackHandler::UpdateKeyMappings()
{
    auto* controlMap = RE::ControlMap::GetSingleton();
    const auto* userEvents = RE::UserEvents::GetSingleton();

    blockMapping.keyboard = controlMap->GetMappedKey(userEvents->leftAttack, RE::INPUT_DEVICE::kKeyboard);
    blockMapping.mouse = controlMap->GetMappedKey(userEvents->leftAttack, RE::INPUT_DEVICE::kMouse);
    blockMapping.gamepad = controlMap->GetMappedKey(userEvents->leftAttack, RE::INPUT_DEVICE::kGamepad);

    readyWeaponMapping.keyboard = controlMap->GetMappedKey(userEvents->readyWeapon, RE::INPUT_DEVICE::kKeyboard);
    readyWeaponMapping.mouse = controlMap->GetMappedKey(userEvents->readyWeapon, RE::INPUT_DEVICE::kMouse);
    readyWeaponMapping.gamepad = controlMap->GetMappedKey(userEvents->readyWeapon, RE::INPUT_DEVICE::kGamepad);

    isKeysMapped = true;
}

CancelAttackHandler::TriggerType CancelAttackHandler::MatchTrigger(const RE::INPUT_DEVICE dev, const std::uint32_t key) const
{
    const auto settings = Settings::GetSingleton();

    switch (dev) {
    case RE::INPUT_DEVICE::kKeyboard:
        if (settings->useReadyWeaponButton && key == readyWeaponMapping.keyboard)
            return TriggerType::kReadyWeapon;
        if (settings->useBlockButton && key == blockMapping.keyboard)
            return TriggerType::kBlock;
        break;

    case RE::INPUT_DEVICE::kMouse:
        if (settings->useReadyWeaponButton && key == readyWeaponMapping.mouse)
            return TriggerType::kReadyWeapon;
        if (settings->useBlockButton && key == blockMapping.mouse)
            return TriggerType::kBlock;
        break;

    case RE::INPUT_DEVICE::kGamepad:
        if (settings->useReadyWeaponButton && key == readyWeaponMapping.gamepad)
            return TriggerType::kReadyWeapon;
        if (settings->useBlockButton && key == blockMapping.gamepad)
            return TriggerType::kBlock;
        break;

    default:
        break;
    }

    return TriggerType::kNone;
}

bool CancelAttackHandler::HasTwoHandedWeaponEquipped(const RE::PlayerCharacter* player)
{
    const auto* rightHand = player->GetEquippedObject(false);
    const auto* rightWeapon = rightHand ? rightHand->As<RE::TESObjectWEAP>() : nullptr;
    return rightWeapon && (rightWeapon->IsTwoHandedAxe() || rightWeapon->IsTwoHandedSword());
}

bool CancelAttackHandler::IsRestrictedCancelWindow(const RE::PlayerCharacter* player) const
{
    const auto* settings = Settings::GetSingleton();
    if (!settings->restrictCancelWindow || (settings->cancelWindow1H <= 0 && settings->cancelWindow2H <= 0)) {
        return false;
    }
    if (lastAttackStartTime.time_since_epoch().count() == 0) {
        return true;
    }
    int cancelWindowMs = HasTwoHandedWeaponEquipped(player) ? settings->cancelWindow2H : settings->cancelWindow1H;
    return std::chrono::steady_clock::now() - lastAttackStartTime > std::chrono::milliseconds(cancelWindowMs);
}

float CancelAttackHandler::GetStaminaCost(const RE::PlayerCharacter* player)
{
    const auto* settings = Settings::GetSingleton();
    return HasTwoHandedWeaponEquipped(player) ? settings->staminaCost2H : settings->staminaCost1H;
}

bool CancelAttackHandler::TryConsumeStamina(RE::PlayerCharacter* player)
{
    const float cost = GetStaminaCost(player);
    if (cost <= 0.0f) {
        return true;
    }
    if (player->AsActorValueOwner()->GetActorValue(RE::ActorValue::kStamina) >= cost) {
        player->AsActorValueOwner()->RestoreActorValue(RE::ACTOR_VALUE_MODIFIER::kDamage, RE::ActorValue::kStamina, -cost);
        return true;
    }
    RE::HUDMenu::FlashMeter(RE::ActorValue::kStamina);
    return false;
}

void CancelAttackHandler::CancelAttack(RE::PlayerCharacter* player, RE::INPUT_DEVICE dev, std::uint32_t key)
{
    player->NotifyAnimationGraph("attackStop");

    const bool triggerIsReady = MatchTrigger(dev, key) == TriggerType::kReadyWeapon;

    bool shouldStartBlock = triggerIsReady;
    if (!shouldStartBlock) {
        const auto* left = player->GetEquippedObject(true);
        const auto* armor = left ? left->As<RE::TESObjectARMO>() : nullptr;
        shouldStartBlock = armor && armor->IsShield();
    }

    if (shouldStartBlock) {
        bool isBlocking = false;
        player->GetGraphVariableBool("IsBlocking", isBlocking);
        player->AsActorState()->actorState2.wantBlocking = 1;
        if (!isBlocking)
            player->NotifyAnimationGraph("blockStart");
        triggerKey = std::make_pair(dev, key);
    } else {
        triggerKey.reset();
    }
}

void CancelAttackHandler::HandleTriggerKeyRelease(RE::InputEvent* const* a_event, RE::PlayerCharacter* player)
{
    for (auto e = *a_event; e; e = e->next) {
        if (auto* btn = e->AsButtonEvent()) {
            if (btn->IsUp() &&
                triggerKey &&
                btn->GetDevice() == triggerKey->first &&
                btn->GetIDCode() == triggerKey->second) {
                player->AsActorState()->actorState2.wantBlocking = 0;
                bool isBlocking = false;
                player->GetGraphVariableBool("IsBlocking", isBlocking);
                if (isBlocking)
                    player->NotifyAnimationGraph("blockStop");
                triggerKey.reset();
            }
        }
    }
}

bool CancelAttackHandler::HandleCancelAttempt(RE::InputEvent* const* a_event, RE::PlayerCharacter* player)
{
    const auto* rightHand = player->GetEquippedObject(false);
    if (const auto* rightWeapon = rightHand ? rightHand->As<RE::TESObjectWEAP>() : nullptr;
        !rightWeapon || !rightWeapon->IsMelee() || rightWeapon->IsHandToHandMelee()) {
        return false;
    }

    bool consumed = false;
    for (auto* in = *a_event; in; in = in->next) {
        auto* btn = in->AsButtonEvent();
        if (!btn || !btn->IsPressed() || btn->HeldDuration() > 0.0f)
            continue;

        auto trig = MatchTrigger(btn->GetDevice(), btn->GetIDCode());
        if (trig == TriggerType::kNone)
            continue;

        if (auto* perk = Settings::GetSingleton()->cancelPerk) {
            if (!player->HasPerk(perk)) {
                logger::info("Player missing required perk, aborting cancel");
                return false;    // block cancel attempt
            }
        }
        
        if (IsRestrictedCancelWindow(player) || !TryConsumeStamina(player))
            return (trig == TriggerType::kReadyWeapon);

        CancelAttack(player, btn->GetDevice(), btn->GetIDCode());
        consumed = (trig == TriggerType::kReadyWeapon);
    }
    return consumed;
}

bool CancelAttackHandler::ProcessEvent(RE::InputEvent* const* a_event)
{
    auto* player = RE::PlayerCharacter::GetSingleton();
    if (!player || !a_event) {
        return false;
    }

    HandleTriggerKeyRelease(a_event, player);
    UpdateAttackState(player);
    if (!player->IsAttacking()) {
        return false;
    }
    if (!isKeysMapped) {
        UpdateKeyMappings();
    }
    return HandleCancelAttempt(a_event, player);
}
