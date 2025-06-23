#include <unistd.h>
#include <zerg/tool/admin.h>
#include <iomanip>
#include <iostream>
#include <linenoise.hpp>

using namespace std;
using namespace zerg;

void help() {
    std::cout << "Program options:" << std::endl;
    std::cout << "  -h                                    list help" << std::endl;
    std::cout << "  -k                                    key" << std::endl;
    std::cout << "  -c                                    cmd" << std::endl;
    std::cout << "  -x                                    continous mode" << std::endl;
    std::cout << "  -y                                    client mode" << std::endl;
    std::cout << "demo:" << std::endl;
    std::cout << "server: ./demo_test_admin_server -k /tmp/aaron" << std::endl;
    std::cout << "client: ./demo_test_admin_server -k /tmp/aaron -y -c hello" << std::endl;
}

int main(int argc, char** argv) {
    bool server_mode = true;
    string key, cmd;
    bool continous{false};
    int opt;
    while ((opt = getopt(argc, argv, "hxyk:c:")) != -1) {
        switch (opt) {
            case 'k':
                key = std::string(optarg);
                break;
            case 'c':
                cmd = std::string(optarg);
                break;
            case 'x':
                continous = true;
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

    auto* admin = new Admin(key);
    if (server_mode) {
        admin->OpenForCreate();
        while (true) {
            cmd = admin->ReadCmd();
            if (!cmd.empty()) {
                cout << "recv " << cmd << endl;
                admin->WriteReturn(cmd);
            }

            sleep(1);
        }
    } else {
        if (!admin->OpenForRead()) {
            cerr << "Admin failed for key " << key << endl;
            return -1;
        }

        if (continous) {
            linenoise::SetMultiLine(false);
            linenoise::SetHistoryMaxLen(100);
            while (true) {
                std::string line;

                auto quit = linenoise::Readline("> ", line);

                if (quit) {
                    break;
                }

                admin->IssueCmd(line);

                linenoise::AddHistory(line.c_str());
                std::string ret;
                do {
                    sleep(1);
                    ret = admin->ReadReturn();
                } while (ret.empty());
                if (ret == "DIE") {
                    printf("server die\n");
                    fflush(stdout);
                    break;
                }
                printf("Receive return %s\n", ret.c_str());
                fflush(stdout);
            }
        } else {
            admin->IssueCmd(cmd);
            std::string ret;
            do {
                sleep(1);
                ret = admin->ReadReturn();
            } while (ret.empty() && ret != "DIE");
            printf("Receive return %s\n", ret.c_str());
        }
    }

}
