#include "../src/preferences.hpp"
#include <cassert>
#include <fstream>
int main(){
    using namespace theater;
    auto path=std::filesystem::path("build/tests/preferences/settings.json");
    options value;value.move_speed=.003f;value.fast_multiplier=8;value.slow_multiplier=.02f;
    assert(preferences::save(value,path));auto loaded=preferences::load(path);
    assert(loaded.move_speed==value.move_speed && loaded.fast_multiplier==8 && loaded.slow_multiplier==.02f);
    value.release_weapons=true;value.weapon_release_delay=1.2f;value.native_weapon_drops=true;
    assert(preferences::save(value,path));loaded=preferences::load(path);
    assert(loaded.hold_weapons && loaded.release_weapons && !loaded.native_weapon_drops && loaded.weapon_release_delay==1.2f);
    value.move_speed=.7f;assert(preferences::save(value,path));assert(preferences::load(path).move_speed==.7f);
    std::ofstream(path)<<"{\"moveSpeed\":0.01}";loaded=preferences::load(path);
    assert(loaded.move_speed==.01f && loaded.fast_multiplier==2 && loaded.slow_multiplier==.25f);
    std::ofstream(path)<<"not json";assert(preferences::load(path).move_speed==1);
    value.move_speed=std::numeric_limits<float>::quiet_NaN();value.fast_multiplier=-1;value.slow_multiplier=10;
    auto safe=preferences::sanitize(value);assert(safe.move_speed==1 && safe.fast_multiplier==1 && safe.slow_multiplier==1);
}
