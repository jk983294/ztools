#include <unistd.h>
#include <zerg/tool/channel.h>
#include <iomanip>
#include <iostream>
#include <zerg/log.h>

using namespace std;
using namespace zerg;

void help() {
    std::cout << "Program options:" << std::endl;
    std::cout << "  -h                                    list help" << std::endl;
    std::cout << "  -k                                    key" << std::endl;
    std::cout << "  -y                                    primary pub or tube mode" << std::endl;
    std::cout << "  -r                                    role (pub/sub)" << std::endl;
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
    //tube or primaryPub
    string mode = "tube";
    string role = "publisher";
    string key, cmd;
    int opt;
    while ((opt = getopt(argc, argv, "hy:k:r:")) != -1) {
        switch (opt) {
            case 'k':
                key = std::string(optarg);
                break;
            case 'y':
                mode = std::string(optarg);
                break;
            case 'r':
                role = std::string(optarg);
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
    ChannelCoordinator coordinator("/tmp/");

    if (role == "publisher") {
        Channel* publisher{nullptr};
        if (mode == "tube") {
            publisher = coordinator.linkChannel(key, true);
        } else if (mode == "primaryPub") {
            mgr.RegisterPublisher(key, 0, sizeof(MyData), n);
        } else {
            ZLOG_THROW("invalid mode %s", mode.c_str());
        }
        MyData d;
        d.x = 0;
        while(true) {
            ZLOG("produce %ld\n", d.x);
            publisher->Publish((char*)&d, sizeof(MyData));
            d.x++;
            sleep(1);
        }

    } else if (role == "subscriber") {
         Channel* subscriber{nullptr};
        if (mode == "tube") {
            subscriber = coordinator.linkChannel(key, false);
        } else if (mode == "primaryPub") {
           subscriber = mgr.RegisterSubscriber(key);
        } else {
            ZLOG_THROW("invalid mode %s", mode.c_str());
        }
        int64_t doneIndex = -1;
        int maxCount = subscriber->GetMaxCount();
        MyData* array = reinterpret_cast<MyData*>(subscriber->data_start);
        while (true) {
            int64_t curIndex = subscriber->GetIndex();
            for (int64_t i = doneIndex + 1; i <= curIndex; ++i) {
                visit(array[i % maxCount]);
            }

            if (curIndex < doneIndex) {
                ZLOG("%ld < %ld, channel %s maybe cleared\n", curIndex, doneIndex, subscriber->name.c_str());
            }
            doneIndex = curIndex;
            usleep(100);
        }
    } else {
        ZLOG_THROW("invalid role %s", role.c_str());
    }

}
