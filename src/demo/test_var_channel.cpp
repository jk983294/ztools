#include <unistd.h>
#include <zerg/tool/channel.h>
#include <iomanip>
#include <iostream>
#include <zerg/log.h>
#include <zerg/time/clock.h>

using namespace std;
using namespace zerg;

void help() {
    std::cout << "Program options:" << std::endl;
    std::cout << "  -h                                    list help" << std::endl;
    std::cout << "  -k                                    key" << std::endl;
    std::cout << "  -y                                    client mode" << std::endl; 
    std::cout << "  -r                                    role (pub/sub)" << std::endl;
    std::cout << "demo:" << std::endl;
    std::cout << "server: ./demo_test_var_channel -k key1" << std::endl;
    std::cout << "client: ./demo_test_var_channel -k key1 -y" << std::endl;
}


struct MyData {
    int64_t x;
};

void visit(const MyData& d) {
    printf("visit d=%ld\n", d.x);
}

int main(int argc, char** argv) {
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
    zerg::Clock clock("/dat/ctp/trading_day.txt");
    int tradingDay = clock.DateToInt();

    ChannelMgr mgr("/tmp/", tradingDay);
    ChannelCoordinator coordinator("/tmp/", tradingDay);

    if (role == "publisher") {
        Channel* publisher{nullptr};
        if (mode == "tube") {
            publisher = coordinator.linkChannel(key, true);
        } else if (mode == "primaryPub") {
            publisher = mgr.RegisterPublisherVar(key, (sizeof(MyData) + sizeof(ShmMsgHeader)) * n + 17);
        
        } else {
            ZLOG_THROW("invalid mode %s", mode.c_str());
        }
        char buffer[sizeof(ShmMsgHeader) + sizeof(MyData)];
        ShmMsgHeader* h = reinterpret_cast<ShmMsgHeader*>(buffer);
        MyData* d = reinterpret_cast<MyData*>(h + 1);
        h->msg_len = sizeof(ShmMsgHeader) + sizeof(MyData);
        h->seq_num = 0;
        d->x = 0;
        while(true) {
            printf("produce %ld\n", d->x);
            publisher->PublishVar(buffer);
            d->x++;
            h->seq_num++;
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
        const char* data = nullptr;
        while (true) {
            int64_t curIndex = subscriber->GetIndex();
            for (int64_t i = doneIndex + 1; i <= curIndex; ++i) {
                const char* pd = subscriber->PollVar(data);
                if (pd == nullptr) {
                    throw std::runtime_error("should not happen");
                }
                auto* h = reinterpret_cast<const ShmMsgHeader*>(pd);
                if(h->seq_num != i) {
                    i = h->seq_num;
                    ZLOG("caused by channel warp, set to idx=%ld,%ld,%ld\n", i, doneIndex, curIndex);
                }
                visit(*reinterpret_cast<const MyData*>(h + 1));
                data = pd + h->msg_len;
            }

            if (curIndex < doneIndex) {
                ZLOG("%ld < %ld, channel %s maybe cleared\n", curIndex, doneIndex, subscriber->name.c_str());
                data = nullptr;
            }
            doneIndex = curIndex;
            usleep(100);
        }
    } else {
        ZLOG_THROW("invalid role %s", role.c_str());
    }

}
