#include "Hooks.h"
#include "CancelAttackHandler.h"
#include "Settings.h"

namespace Hooks
{
    void Install() noexcept
    {
        if (Settings::GetSingleton()->useReadyWeaponButton) {
            stl::write_vfunc<ReadyWeaponCanProcessHook>(RE::ReadyWeaponHandler::VTABLE[0]);

            logger::info("Hooks installed successfully");
        }
    }

    bool ReadyWeaponCanProcessHook::Thunk(RE::ReadyWeaponHandler* a_this, RE::InputEvent* a_event) noexcept
    {
        return CancelAttackHandler::GetSingleton()->ProcessEvent(&a_event) ? false : func(a_this, a_event);
    }

}
