#include <fcntl.h>
#include <zerg/io/file.h>
#include <pwd.h>
#include <unistd.h>
#include <zerg/log.h>
#include <zerg/time/time.h>
#include <zerg/tool/channel.h>
#include <zerg/unix.h>
#include <iostream>
#include <zerg/time/clock.h>

using namespace std;

namespace zerg {
constexpr int32_t ChannelMagic = 'C' * 42 + 'n' * 41 + 'l' * 37;

bool Channel::Publish(const char* data, uint32_t size) {
    if (pcb->topic_size != size) {
        ZLOG_THROW("%s publish size incorrect %u %u", name.c_str(), pcb->topic_size, size);
    }
    if (pdata >= data_boundary) {
        ZLOG_THROW("%s publish full %u", name.c_str(), pcb->topic_n);
    }
    memcpy(pdata, data, size);
    pdata += size;
    if (pdata >= data_boundary) {
        if (pcb->warp == 1) {
            pdata = data_start;
            ZLOG("warn! %s warp to data start!", name.c_str());
        }
    }
    __sync_add_and_fetch(&pcb->curr_idx, 1);
    return true;
}
bool Channel::PublishVar(const char* data) {
    if (pdata >= data_boundary) {
        ZLOG_THROW("%s publish full %u", name.c_str(), pcb->topic_n);
    }
    const ShmMsgHeader* h = reinterpret_cast<const ShmMsgHeader*>(data);
    uint32_t data_size = h->msg_len;
    if (pdata + data_size <= data_boundary) {
        memcpy(pdata, data, data_size);
        pdata += data_size;
        if (pdata >= data_boundary) {
            if (pcb->warp == 1) {
                pdata = data_start;
                ZLOG("warn! %s warp to data start!", name.c_str());
            }
        }
    } else {
        if (pcb->warp == 1) {
            if (data_boundary - pdata > 0) {
                memset(pdata, 0, data_boundary - pdata);
            }
            ZLOG("warn! %s warp to data start!", name.c_str());
            pdata = data_start;
            memcpy(pdata, data, data_size);
            pdata += data_size;
        } else {
            ZLOG("error! %s publish full! %u", name.c_str(), data_size);
            return false;
        }
    }
    __sync_add_and_fetch(&pcb->curr_idx, 1);
    return true;
}

const char* Channel::PollVar(const char* data) {
    if (data == nullptr) data = data_start;
    if (data + sizeof(ShmMsgHeader) >= data_boundary) {
        if (pcb->warp == 0) {
            ZLOG("error! %s PollVar full!", name.c_str());
            return nullptr;
        }
        data = data_start;
    } else {
        auto* h = reinterpret_cast<const ShmMsgHeader*>(data);
        if (h->msg_len == 0 || data + h->msg_len > data_boundary) {
            if (pcb->warp == 0) {
                ZLOG("error! %s PollVar full!", name.c_str());
                return nullptr;
            }
            data = data_start;
        }
    }
    return data;
}

Channel::Channel(std::string name_, ChnlCtrlBlock* pcb_, ChannelRole role, bool isTubeMode) {
    name = name_;
    pcb = pcb_;
    data_start = (char*)(pcb + 1);
    data_boundary = ((char*)pcb) + pcb->header.size;
    if ((!isTubeMode && role == PUBER) || (isTubeMode && role ==TUBER)) {
        pcb->curr_idx = -1;
        pcb->warp = 1;
    }

    if (pcb->topic_size != 0) {
        pdata = data_start + ((pcb->curr_idx + 1) % pcb->topic_n) * pcb->topic_size;
    } else if (pcb->curr_idx != -1) {
        throw std::runtime_error("not support yet!");
    } else {
        pdata = data_start;
    }
    ZLOG("%s init idx=%ld, warp=%u, topic_n=%u, topic_size=%u, pid=%d, d=%d, t=%s", name.c_str(),
        pcb->curr_idx, (uint32_t)pcb->warp, pcb->topic_n, (uint32_t)pcb->topic_size, pcb->header.pid,
        pcb->header.date, ntime2string(pcb->header.creation_time).c_str());
}

Channel::~Channel() {
    ReleaseShm((char*)pcb);
}
int64_t Channel::GetIndex() { return pcb->curr_idx; }
int32_t Channel::GetMaxCount() { return pcb->topic_n; }

ChannelMgr::ChannelMgr(const std::string& dir, int32_t tradingDay) {
    m_dir = dir;
    m_date = tradingDay;
}

ChannelMgr::~ChannelMgr() {
    for (auto& item : m_n2c) {
        delete item.second;
    }
}

Channel* ChannelMgr::RegisterPublisherVar(const std::string& name, uint64_t total_data_size) {
    auto itr = m_n2c.find(name);
    if (itr != m_n2c.end()) {
        ZLOG_THROW("RegisterPublisherVar twice for %s", name.c_str());
    }
    uint64_t total_size = total_data_size + sizeof(ChnlCtrlBlock);
    auto* pcb = (ChnlCtrlBlock*)CreateShm(path_join(m_dir, name), total_size, ChannelMagic, m_date);
    pcb->topic_size = 0;
    pcb->topic_n = 0; // indicate it is variable length version
    Channel* c = new Channel(name, pcb, PUBER, false);
    m_n2c[name] = c;
    return c;
}

Channel* ChannelMgr::RegisterPublisher(const std::string& name, uint64_t total_size, uint32_t topic_size, uint32_t topic_n) {
    if (total_size == 0) {
        total_size = ((uint64_t)topic_n) * topic_size + sizeof(ChnlCtrlBlock);
    }
    auto itr = m_n2c.find(name);
    if (itr != m_n2c.end()) {
        ZLOG_THROW("RegisterPublisher twice for %s", name.c_str());
    }
    auto* pcb = (ChnlCtrlBlock*)CreateShm(path_join(m_dir, name), total_size, ChannelMagic, m_date);
    pcb->topic_size = topic_size;
    pcb->topic_n = topic_n;
    Channel* c = new Channel(name, pcb, PUBER, false);
    m_n2c[name] = c;
    return c;
}
Channel* ChannelMgr::RegisterSubscriber(const std::string& name) {
    auto itr = m_n2c.find(name);
    if (itr != m_n2c.end()) {
        return itr->second;
    }
    auto* pcb = (ChnlCtrlBlock*)LinkShm(path_join(m_dir, name), ChannelMagic, m_date, true);
    Channel* c = new Channel(name, pcb, SUBER, false);
    m_n2c[name] = c;
    return c;
}


ChannelCoordinator::ChannelCoordinator(const std::string& dir, int32_t tradingDay) {
    m_dir = dir;
    m_date = tradingDay;
}

ChannelCoordinator::~ChannelCoordinator() {
    for (auto& item : m_n2c) {
        delete item.second;
    }
}

void ChannelCoordinator::createChannelVar(const std::string& name, uint64_t total_data_size) {
    auto itr = m_n2c.find(name);
    if (itr != m_n2c.end()) {
        ZLOG_THROW("RegisterPublisherVar twice for %s", name.c_str());
    }
    uint64_t total_size = total_data_size + sizeof(ChnlCtrlBlock);
    auto* pcb = (ChnlCtrlBlock*)CreateShm(path_join(m_dir, name), total_size, ChannelMagic, m_date);
    pcb->topic_size = 0;
    pcb->topic_n = 0; // indicate it is variable length version
    Channel* c = new Channel(name, pcb, TUBER, true);
    m_n2c[name] = c;
}

void ChannelCoordinator::createChannel(const std::string& name, uint64_t total_size, uint32_t topic_size, uint32_t topic_n) {
    if (total_size == 0) {
        total_size = ((uint64_t)topic_n) * topic_size + sizeof(ChnlCtrlBlock);
    }
    auto itr = m_n2c.find(name);
    if (itr != m_n2c.end()) {
        ZLOG_THROW("RegisterPublisher twice for %s", name.c_str());
    }
    auto* pcb = (ChnlCtrlBlock*)CreateShm(path_join(m_dir, name), total_size, ChannelMagic, m_date);
    pcb->topic_size = topic_size;
    pcb->topic_n = topic_n;
    Channel* c = new Channel(name, pcb, TUBER, true);
    m_n2c[name] = c;
}

Channel* ChannelCoordinator::linkChannel(const std::string& name, bool isPub) {
    auto itr = m_n2c.find(name);
    if (itr != m_n2c.end()) {
        return itr->second;
    }
    auto* pcb = (ChnlCtrlBlock*)LinkShm(path_join(m_dir, name), ChannelMagic, m_date, !isPub);
    Channel* c = new Channel(name, pcb, isPub? PUBER:SUBER, true);
    m_n2c[name] = c;
    return c;
}

}