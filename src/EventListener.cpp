#include "EventListener.h"
#include "CancelAttackHandler.h"

void EventListener::Register()
{
    const auto listener = GetSingleton();
    if (!listener) {
        logger::error("Failed to get EventListener singleton");
        return;
    }

    RE::UI::GetSingleton()->AddEventSink<RE::MenuOpenCloseEvent>(listener);
    RE::BSInputDeviceManager::GetSingleton()->AddEventSink(listener);
    logger::info("EventListener registered successfully");
}

RE::BSEventNotifyControl
EventListener::ProcessEvent(const RE::MenuOpenCloseEvent* a_event,
                            [[maybe_unused]] RE::BSTEventSource<RE::MenuOpenCloseEvent>* a_source)
{
    if (!a_event || a_event->menuName.empty()) {
        return RE::BSEventNotifyControl::kContinue;
    }

    if (a_event->menuName == RE::InterfaceStrings::GetSingleton()->journalMenu && !a_event->opening) {
        CancelAttackHandler::GetSingleton()->UpdateKeyMappings();
    }

    return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl
EventListener::ProcessEvent(RE::InputEvent* const* a_event,
                            [[maybe_unused]] RE::BSTEventSource<RE::InputEvent*>* a_source)
{
    (void)CancelAttackHandler::GetSingleton()->ProcessEvent(a_event);
    return RE::BSEventNotifyControl::kContinue;
}
