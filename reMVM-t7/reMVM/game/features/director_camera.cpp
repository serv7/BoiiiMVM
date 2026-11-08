#include "director_camera.h"

#include <MH/MinHook.h>
#include "../../mvm.h"

#include <assets/patch.h>
std::vector<utils::patch_base> _cameraPatches;

#include "../assets/functions.h"

gc::director_camera::director_camera() : m_initialized(false), m_autoFocus(false), m_focusOffset(0.0f), m_focusTarget(NULL), m_focusJoint(NULL)
{
    m_directorCameraInstance = this;
}

gc::director_camera::~director_camera()
{

}

void gc::director_camera::initialize()
{
    HMODULE _hModule = GetModuleHandle(L"BlackOps3.exe");

    m_cameraManager = std::make_shared<mvm::camera_manager>();
    m_customDolly = std::make_shared<mvm::track_player>(custom_dolly_marker::size_of());

    _cameraPatches.push_back({ "camera_nop0", gc::game::_base + 0x565DEB, 28 });
    _cameraPatches.push_back({ "camera_nop1", gc::game::_base + 0x565CFD, 6 });
    _cameraPatches.push_back({ "camera_nop2", gc::game::_base + 0x565DA7, 10 });
    _cameraPatches.push_back({ "camera_nop3", gc::game::_base + 0x565F3F, 8 });
    _cameraPatches.push_back({ "camera_nop4", gc::game::_base + 0x565EFF, 8 });
    _cameraPatches.push_back({ "camera_nop5", gc::game::_base + 0x565EB7, 8 });
    _cameraPatches.push_back({ "camera_nop6", gc::game::_base + 0x113A7E0, 6 });
    _cameraPatches.push_back({ "camera_nop7", gc::game::_base + 0x113A7ED, 6 });
    for (auto _it : _cameraPatches) {
        utils::register_patch(_it.name, _it.address, _it.size);
        if (auto _patch = utils::find_patch(_it.name))
            _patch->apply();
    }

    game::_viewMatrix = (matrix*)(uintptr_t)(game::_base + 0xF8C1EF0);
    game::_camera = (gc::camera*)utils::memory::pointer(game::_base + 0x17B0E390, 0x131D28);
    game::_camera->focal_length = 1.0f;
    game::_camera->focus_distance = 2.0f;
    game::_camera->fStop = 80.0f;
    game::_camera->temperature = 6500.0f;
    game::_fov = (double*)(uintptr_t)(game::_base + 0x2FA7CC8);

    game::_theaterCamera = (theater_camera*)utils::memory::pointer(game::_theater, 0x981A68);

    game::_dollyMarkersCount = (int*)utils::memory::pointer(game::_theater, 0x9845D8);
    game::_dollyMarkers = (dolly_marker*)utils::memory::pointer(game::_theater, 0x983248);

    game::_playerPosList = (uintptr_t)(game::_base + 0x196C3B9C);

    m_addDollyCamMarker = utils::signature::find(_hModule,
        "40 55 48 8B EC 48 83 EC ?? 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 45 F0 83 3D ?? ?? ?? ?? ?? 0F 85 ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ??");
    m_removeDollyCamMarker = utils::signature::find(_hModule,
        "48 83 EC ?? E8 ?? ?? ?? ?? 48 8B 48 18 48 63 01 83 7C 81 64 ?? 0F 85 ?? ?? ?? ?? 83 3D ?? ?? ?? ?? ? 0F 85 ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 84 C0 0F 84 ?? ?? ?? ?? E8 ?? ?? ?? ?? 84 C0 0F 84 ?? ?? ?? ?? E8 ?? ?? ?? ?? 84 C0 0F 85 ?? ?? ?? ?? E8 ?? ?? ?? ?? 84 C0 0F 85 ?? ?? ?? ?? 48 89 5C 24 30");
    m_applyNewDollyCamMarkerPosition = utils::signature::find(_hModule,
        "48 83 EC ?? E8 ?? ?? ?? ?? 48 8B 48 18 48 63 01 83 7C 81 64 ?? 75 ?? 83 3D ?? ?? ?? ?? ?? 75 ?? 48 8B 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 84 C0 74 ?? E8 ?? ?? ?? ?? 84 C0 74 ?? E8 ?? ?? ?? ??");
    
    MH_Initialize();
    MH_CreateHook(m_addDollyCamMarker, add_dolly_cam_marker, m_addDollyCamMarker.address_of());
    MH_CreateHook(m_removeDollyCamMarker, remove_dolly_cam_marker, m_removeDollyCamMarker.address_of());
    MH_CreateHook(m_applyNewDollyCamMarkerPosition, apply_new_dolly_cam_marker_position, m_applyNewDollyCamMarkerPosition.address_of());
    MH_EnableHook(MH_ALL_HOOKS);

    m_initialized = true;
}

void gc::director_camera::update(float _time, float _frameTime)
{
    game::_playerNames.clear();
    uintptr_t _name = utils::memory::pointer(game::_base + 0x51A3500, 0xC764C);
    int _it = NULL;
    while (*(char*)_name != NULL) {
        _it++;
        game::_playerNames.push_back((const char*)_name);
        _name += 0x338;
        if (_it > 0x32)
            break;
    }

    if (!m_cameraManager || !m_initialized)
        return;

    bool _playingDolly = (*game::_theaterMode == theater_mode::dolly);
    if (!_playingDolly && (m_customDolly->size() > NULL) && ((*game::_dollyMarkersCount) == NULL))
        m_customDolly->clear();

    auto _input = mvm::_core->input();
    auto _mouseState = _input->mouse_state();
    if (mvm::_core->focus() && !_playingDolly && !mvm::_core->ui_wrapper()->enabled()) {
        _mouseState->wheel *= 50.0f * (*m_cameraManager->input_scale());
        if (!_input->key_down(VK_SHIFT) && !_input->key_down(VK_CONTROL) && !_input->key_down(VK_MENU)) {
            *game::_fov -= _mouseState->wheel * _frameTime;
            *game::_fov = (double)clamp(*game::_fov, 0.1f, 75.0f);
        }
        if (game::_camera && game::_camera->dof_type > NULL) {
            if (_input->key_down(VK_SHIFT)) {
                game::_camera->focal_length += _mouseState->wheel * _frameTime;
                if (game::_camera->focal_length <= NULL)
                    game::_camera->focal_length = 0.1f;
            }
            if (_input->key_down(VK_CONTROL) && !m_autoFocus) {
                game::_camera->focus_distance += _mouseState->wheel * _frameTime;
                if (game::_camera->focus_distance <= NULL)
                    game::_camera->focus_distance = 0.1f;
            }
        }
    }
    _mouseState->reset();

    if (_playingDolly) {
        custom_dolly_marker _custom = m_customDolly->value(_time).data();

        *game::_fov = _custom._fov;
        game::_camera->focal_length = _custom._focalLength;
        if (!m_autoFocus)
            game::_camera->focus_distance = _custom._focusDistance;
        game::_camera->fStop = _custom._fStop;
    }

    if (m_autoFocus) {
        vector3* _cameraPos = (vector3*)(uintptr_t)(game::_base + 0x347EF08);
        vector3 _targetPos = 0.0f;
        if (gc::game::_joints.empty())
            _targetPos = *(vector3*)(uintptr_t)(game::_playerPosList + (0x1F0 * m_focusTarget));
        else get_tag_pos(get_entity(NULL, m_focusTarget), gc::game::_joints[m_focusJoint].tag, &_targetPos);
        game::_camera->focus_distance = _cameraPos->distance(_targetPos + m_focusOffset);
    }
}

gc::custom_dolly_marker gc::director_camera::create_custom_marker()
{
    custom_dolly_marker _customMarker;
    _customMarker._fov = *game::_fov;
    _customMarker._focalLength = game::_camera->focal_length;
    _customMarker._focusDistance = game::_camera->focus_distance;
    _customMarker._fStop = game::_camera->fStop;
    return _customMarker;
}

__int64 __stdcall gc::director_camera::add_dolly_cam_marker()
{
    float _time = (float)*game::_tick / 1000.0f;
    custom_dolly_marker _customMarker = m_directorCameraInstance->create_custom_marker();
    m_directorCameraInstance->m_customDolly->insert(_time, _customMarker.data());
    return m_addDollyCamMarker.invoke<__int64>();
}

__int64 __stdcall gc::director_camera::remove_dolly_cam_marker()
{
    float _time = (float)*game::_tick / 1000.0f;
    float _existTime = m_directorCameraInstance->m_customDolly->nearest(_time);
    if (_existTime >= 0.0f)
        m_directorCameraInstance->m_customDolly->remove(_existTime);
    return m_removeDollyCamMarker.invoke<__int64>();
}

__int64 __stdcall gc::director_camera::apply_new_dolly_cam_marker_position()
{
    float _time = (float)*game::_tick / 1000.0f;
    custom_dolly_marker _customMarker = m_directorCameraInstance->create_custom_marker();
    float _existTime = m_directorCameraInstance->m_customDolly->nearest(_time);
    if (_existTime >= 0.0f)
        m_directorCameraInstance->m_customDolly->replace(_existTime, mvm::node::value(_customMarker.data(), custom_dolly_marker::size_of()));
    return m_applyNewDollyCamMarkerPosition.invoke<__int64>();
}