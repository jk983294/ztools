#include <unistd.h>
#include <zerg/tool/channel.h>
#include <iomanip>
#include <iostream>

using namespace std;
using namespace zerg;

void help() {
    std::cout << "Program options:" << std::endl;
    std::cout << "  -h                                    list help" << std::endl;
    std::cout << "  -k                                    key" << std::endl;
    std::cout << "  -y                                    client mode" << std::endl;
    std::cout << "demo:" << std::endl;
    std::cout << "server: ./demo_test_channel -k key1" << std::endl;
    std::cout << "client: ./demo_test_channel -k key1 -y" << std::endl;
}

struct MyData {
    int64_t x;
};

void visit(const MyData& d) {
    printf("visit d=%ld\n", d.x);
}

int main(int argc, char** argv) {
    bool server_mode = true;
    string key, cmd;
    int opt;
    while ((opt = getopt(argc, argv, "hyk:")) != -1) {
        switch (opt) {
            case 'k':
                key = std::string(optarg);
                break;
            case 'y':
                server_mode = false;
                break;
            case 'h':
            default:
                help();
                return 1;
        }
    }

    if (key.empty()) {
        help();
        return 1;
    }

    uint32_t n = 10;

    ChannelMgr mgr("/tmp/");
    if (server_mode) {
        auto* publisher = mgr.RegisterPublisher(key, 0, sizeof(MyData), n);
        MyData d;
        d.x = 0;
        while(true) {
            printf("produce %ld\n", d.x);
            publisher->Publish((char*)&d, sizeof(MyData));
            d.x++;
            sleep(1);
        }
    } else {
        auto* subscriber = mgr.RegisterSubscriber(key);
        int64_t doneIndex = -1;
        int maxCount = subscriber->GetMaxCount();
        MyData* array = reinterpret_cast<MyData*>(subscriber->data_start);
        while (true) {
            int64_t curIndex = subscriber->GetIndex();
            for (int64_t i = doneIndex + 1; i <= curIndex; ++i) {
                visit(array[i % maxCount]);
            }

            if (curIndex < doneIndex) {
                printf("%ld < %ld, channel %s maybe cleared\n", curIndex, doneIndex, subscriber->name.c_str());
            }
            doneIndex = curIndex;
            usleep(100);
        }
    }

}
