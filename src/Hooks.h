#pragma once

namespace Hooks
{
    void Install() noexcept;

    class ReadyWeaponCanProcessHook : REX::Singleton<ReadyWeaponCanProcessHook>
    {
    public:
        static bool Thunk(RE::ReadyWeaponHandler* a_this, RE::InputEvent* a_event) noexcept;

        inline static REL::Relocation<decltype(Thunk)> func;
        static constexpr std::size_t idx = 0x1;
    };
}
