#include "../src/published_value.hpp"
#include "../src/backend.hpp"
#include "../src/controls_mailbox.hpp"
#include <cassert>
#include <future>
#include <iostream>
#include <mutex>
#include <thread>
int main(){
    using namespace theater;
    published_value<snapshot> view;
    snapshot first;first.playing=true;first.tick=123;first.cameras.resize(50);view.publish(first);
    std::mutex native_mutex;std::promise<void> entered,release;auto gate=release.get_future();
    std::thread native([&]{std::lock_guard lock(native_mutex);entered.set_value();gate.wait();});
    entered.get_future().wait();
    auto input=std::async(std::launch::async,[&]{for(int i=0;i<10000;++i){auto current=view.read();assert(current.playing && current.tick==123 && current.cameras.size()==50);}});
    // The native camera operation is deliberately blocked. Input still reads
    // complete snapshots without needing its mutex or waiting for it to finish.
    assert(input.wait_for(std::chrono::seconds(2))==std::future_status::ready);
    input.get();release.set_value();native.join();
    std::atomic<bool> finished=false;
    std::thread producer([&]{for(int i=0;i<20000;++i){snapshot next;next.tick=i;next.message=std::to_string(i);next.cameras.resize(i%51);view.publish(std::move(next));}finished=true;});
    while(!finished){auto current=view.read();if(current.playing)continue;assert(current.message==std::to_string(current.tick) && current.cameras.size()==current.tick%51);}
    producer.join();auto last=view.read();assert(last.tick==19999);
    auto detached=view.read();detached.tick=-1;detached.cameras.clear();assert(view.read().tick==19999 && !view.read().cameras.empty());
    controls_mailbox controls;options applied;controls.initialize(applied);
    auto ui=applied;ui.move_speed=.1f;ui.hold_weapons=false;controls.request(ui);
    applied=*controls.take();
    // A new UI edit arrives after the native frame took its first request but
    // before a queued CFG applies. Preserve flags the CFG does not own.
    ui.move_speed=.2f;ui.hold_weapons=true;controls.request(ui);
    applied=controls.merge(applied,[](options& next){next.move_speed=.3f;});
    assert(applied.move_speed==.3f && applied.hold_weapons && controls.read().move_speed==.3f && controls.read().hold_weapons && !controls.take());
    ui=applied;ui.move_speed=.4f;controls.request(ui);applied=*controls.take();
    assert(applied.move_speed==controls.read().move_speed);
    std::cout<<"Published UI state checks passed: reads continue during blocked native work, consistent concurrent snapshots, and detached reader copies.\n";
}
