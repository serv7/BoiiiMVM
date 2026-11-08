#include "game_context.h"

#include "assets/addresses.h"
#include "assets/functions.h"

#include <assets/memory.h>
#include <assets/patch.h>
#include <MH/MinHook.h>

#include "../mvm.h"

bool gc::game_context::initialize()
{
    m_gameContextInstance = this;

    HMODULE _hModule = GetModuleHandle(L"BlackOps3.exe");
    if (!_hModule)
        return false;

    while (!(game::_base = utils::memory::base_address(_hModule)))
        continue;

    m_featuresState = false;
    game::_theater = (uintptr_t)(game::_base + 0x197EBA00);
    while (!m_inTheater) {
        m_inTheater = ((*(int*)game::_theater) != NULL);
        continue;
    }

    m_directorCamera = std::make_shared<gc::director_camera>();
    m_lightman = std::make_shared<gc::lightman>();

    initialize_features();
    while (!m_featuresState)
        continue;

    m_initialized = true;
    return true;
}

bool gc::game_context::shutdown()
{
    m_initialized = false;
    return true;
}

void gc::game_context::initialize_features()
{
    HMODULE _hModule = GetModuleHandle(L"BlackOps3.exe");
    if (m_featuresState || !_hModule)
        return;

    game::_blockInput = (bool*)(uintptr_t)(game::_base + 0x162E2280);
    game::_hideHud = (bool*)(uintptr_t)(game::_base + 0x3413BD8);
    utils::register_patch("set_cursor_pos_nop", utils::signature::find("FF 15 ?? ?? ?? ?? 48 8B 4C 24 30 48 33 CC"), 6);

    if (vector3* _cameraPos = (vector3*)(uintptr_t)(gc::game::_base + 0x347EF08)) {
        while (_cameraPos->null())
            continue;
    }

    game::_joints = std::vector<gc::game::joint>();
    game::_joints.push_back({ "j_helmet", (int)gc::register_tag("j_helmet") });
    game::_joints.push_back({ "j_head", (int)gc::register_tag("j_head") });
    game::_joints.push_back({ "j_spine4", (int)gc::register_tag("j_spine4") });
    game::_joints.push_back({ "j_spineupper", (int)gc::register_tag("j_spineupper") });
    game::_joints.push_back({ "j_spinelower", (int)gc::register_tag("j_spinelower") });
    game::_joints.push_back({ "j_mainroot", (int)gc::register_tag("j_mainroot") });
    game::_joints.push_back({ "j_hip_ri", (int)gc::register_tag("j_hip_ri") });
    game::_joints.push_back({ "j_knee_ri", (int)gc::register_tag("j_knee_ri") });
    game::_joints.push_back({ "j_ankle_ri", (int)gc::register_tag("j_ankle_ri") });
    game::_joints.push_back({ "j_ball_ri", (int)gc::register_tag("j_ball_ri") });
    game::_joints.push_back({ "j_hip_le", (int)gc::register_tag("j_hip_le") });
    game::_joints.push_back({ "j_knee_le", (int)gc::register_tag("j_knee_le") });
    game::_joints.push_back({ "j_ankle_le", (int)gc::register_tag("j_ankle_le") });
    game::_joints.push_back({ "j_ball_le", (int)gc::register_tag("j_ball_le") });
    game::_joints.push_back({ "j_clavicle_le", (int)gc::register_tag("j_clavicle_le") });
    game::_joints.push_back({ "j_elbow_le", (int)gc::register_tag("j_elbow_le") });
    game::_joints.push_back({ "j_wrist_le", (int)gc::register_tag("j_wrist_le") });
    game::_joints.push_back({ "j_clavicle_ri", (int)gc::register_tag("j_clavicle_ri") });
    game::_joints.push_back({ "j_elbow_ri", (int)gc::register_tag("j_elbow_ri") });
    game::_joints.push_back({ "j_wrist_ri", (int)gc::register_tag("j_wrist_ri") });
    game::_joints.push_back({ "tag_weapon_left", (int)gc::register_tag("tag_weapon_left") });
    game::_joints.push_back({ "tag_weapon_right", (int)gc::register_tag("tag_weapon_right") });
    game::_joints.push_back({ "tag_weapon_chest", (int)gc::register_tag("tag_weapon_chest") });

    game::_tick = (int*)(game::_base + 0x3448598);
    game::_theaterMode = (int*)utils::memory::pointer(game::_theater, 0x7DAE64);

    m_switchPlayer = utils::signature::find(_hModule, "40 57 48 83 EC ?? 83 CF ??");
    m_freeCameraMove = game::_base + 0x25CE540;

    MH_Initialize();
    MH_CreateHook(m_switchPlayer, game_context::switch_player, m_switchPlayer.address_of());
    MH_CreateHook(m_freeCameraMove, game_context::free_camera_move, m_freeCameraMove.address_of());
    MH_EnableHook(MH_ALL_HOOKS);

    m_directorCamera->initialize();
    m_lightman->initialize();

    m_featuresState = true;
}

void gc::game_context::update()
{
    m_inTheater = ((*(int*)game::_theater) != NULL);

    if (m_inTheater && m_featuresState) {
        if (gc::game::_blockInput) {
            bool _needBlocking = mvm::_core->ui_wrapper()->enabled();
            if (*gc::game::_blockInput != _needBlocking) {
                *gc::game::_blockInput = _needBlocking;
                if (auto _patch = utils::find_patch("set_cursor_pos_nop"); _patch) {
                    if (*gc::game::_blockInput)
                        _patch->apply();
                    else _patch->restore();
                }
            }
        }

        float _time = (float)*game::_tick / 1000.0f;
        float _frameTime = mvm::_core->render()->metrics()->frame_time();
        m_directorCamera->update(_time, _frameTime);
        m_lightman->update(_time, _frameTime);
    }
}

__int64 __stdcall gc::game_context::switch_player()
{
    if (mvm::_core->ui_wrapper()->enabled())
        return NULL;

    return m_switchPlayer.invoke<__int64>();
}

__int64 __stdcall gc::game_context::free_camera_move()
{
    m_gameContextInstance->update();
    return m_freeCameraMove.invoke<__int64>();
}