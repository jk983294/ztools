#pragma once

#include <pthread.h>
#include <string>
#include <unordered_map>
#include <zerg/io/shm.h>

using namespace std;

namespace zerg {
struct __attribute__((packed)) ChnlCtrlBlock {
    ShmHeader header;
    uint16_t topic_size; // 0 for variable length msg
    uint16_t warp; // 1 for circular buffer
    uint32_t topic_n; // 0 means variable length
    int64_t curr_idx; // init -1
    // pthread_mutex_t lock; // SPMC
};

struct __attribute__((packed)) ShmMsgHeader {
    uint16_t msg_type;
    uint16_t msg_len; // including header
    int32_t timestamp;
    int64_t seq_num;
};

enum ChannelRole {
    PUBER,
    SUBER,
    TUBER
};

struct Channel {
    bool m_isPuber{false};
    std::string name;
    ChnlCtrlBlock* pcb{nullptr};
    char* pdata{nullptr};
    char* data_start{nullptr};
    char* data_boundary{nullptr};
    
    Channel() = default;
    ~Channel();
    /**
     * 
     */
    explicit Channel(std::string name_, ChnlCtrlBlock* pcb_, ChannelRole role, bool isTubeMode = false);
    bool Publish(const char* data, uint32_t size);
    /**
     * first part is ShmMsgHeader, get whole msg leng from ShmMsgHeader.msg_len
     */
    bool PublishVar(const char* data);
    const char* PollVar(const char* data);
    int64_t GetIndex();
    int32_t GetMaxCount();
};

struct ChannelMgr {
    int m_date{0};
    string m_dir;
    std::unordered_map<std::string, Channel*> m_n2c;
    ChnlCtrlBlock* pCtrlBlock{nullptr};

    explicit ChannelMgr(const std::string& dir);
    ~ChannelMgr();

    Channel* RegisterPublisher(const std::string& name, uint64_t total_size, uint32_t topic_size, uint32_t topic_n);
    Channel* RegisterPublisherVar(const std::string& name, uint64_t total_data_size);
    Channel* RegisterSubscriber(const std::string& name);

};

struct ChannelCoordinator {
    int m_date{0};
    string m_dir;
    std::unordered_map<std::string, Channel*> m_n2c;
    ChnlCtrlBlock* pCtrlBlock{nullptr};

    explicit ChannelCoordinator(const std::string& dir);
    ~ChannelCoordinator();

    /**
     * for tube
     */
    void createChannel(const std::string& name, uint64_t total_size, uint32_t topic_size, uint32_t topic_n);
    void createChannelVar(const std::string& name, uint64_t total_data_size);
    /**
     * for pub / sub
     */
    Channel* linkChannel(const std::string& name, bool isPub);

};
}
