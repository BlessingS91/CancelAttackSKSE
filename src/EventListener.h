#pragma once

class EventListener final :
    REX::Singleton<EventListener>,
    public RE::BSTEventSink<RE::MenuOpenCloseEvent>,
    public RE::BSTEventSink<RE::InputEvent*>
{
public:
    static void Register();

protected:
    RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* a_event,
                                          [[maybe_unused]] RE::BSTEventSource<RE::MenuOpenCloseEvent>* a_source) override;
    RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* a_event,
                                          [[maybe_unused]] RE::BSTEventSource<RE::InputEvent*>* a_source) override;
};
