#include <unistd.h>
#include <zerg/tool/channel.h>
#include <iomanip>
#include <iostream>

using namespace std;
using namespace zerg;

struct MyData {
    int64_t x;
};

int main(int argc, char** argv) {
    uint32_t n = 10;

    ChannelCoordinator coordinator("/tmp/");
    coordinator.createChannelVar("md", (sizeof(MyData) + sizeof(ShmMsgHeader)) * n + 17);
    coordinator.createChannel("cmd", 0, sizeof(MyData), n);
    coordinator.createChannel("recv", 0, sizeof(MyData), n);

}
