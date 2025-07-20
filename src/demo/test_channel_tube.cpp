#include <unistd.h>
#include <zerg/tool/channel.h>
#include <iomanip>
#include <iostream>
#include <zerg/time/clock.h>

using namespace std;
using namespace zerg;

struct MyData {
    int64_t x;
};

int main(int argc, char** argv) {
    uint32_t n = 10;

    zerg::Clock clock("/dat/ctp/trading_day.txt");
    int tradingDay = clock.DateToInt();
    ChannelCoordinator coordinator("/tmp/", tradingDay);
    coordinator.createChannelVar("md", (sizeof(MyData) + sizeof(ShmMsgHeader)) * n + 17);
    coordinator.createChannel("cmd", 0, sizeof(MyData), n);
    coordinator.createChannel("recv", 0, sizeof(MyData), n);

}
