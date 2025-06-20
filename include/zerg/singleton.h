#pragma once

#include <sys/stat.h>
#include <array>
#include <memory>
#include <string>

namespace zerg {

template <typename T>
class Singleton {
public:
    static T &GetInstance() {
        static T m_instance;
        return m_instance;
    }

protected:
    Singleton() {}

    ~Singleton() {}

private:
    Singleton(const Singleton &rhs) {}

    Singleton operator=(const Singleton &rhs) {}
};

}
