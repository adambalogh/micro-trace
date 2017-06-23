#pragma once

#include <algorithm>
#include <mutex>

#include "common.h"

namespace microtrace {

class SocketInterface;

class SocketMap {
   public:
    typedef std::unique_ptr<SocketInterface> value_type;

    // Should be power of 2
    const int DEFAULT_SIZE = 1024;

    SocketMap() : size_(DEFAULT_SIZE), map_(new value_type[DEFAULT_SIZE]) {}

    ~SocketMap() { delete[] map_; }

    SocketInterface* Get(const int sockfd) {
        std::unique_lock<std::mutex> l(mu_);
        VERIFY(in_range(sockfd), "Invalid sockfd");
        return map_[sockfd].get();
    }

    void Set(const int sockfd, value_type val) {
        std::unique_lock<std::mutex> l(mu_);
        if (!in_range(sockfd)) {
            Resize(sockfd);
        }
        LOG_ERROR_IF(map_[sockfd] == false, "Socket created twice");
        map_[sockfd] = std::move(val);
    }

    void Delete(const int sockfd) {
        std::unique_lock<std::mutex> l(mu_);
        VERIFY(in_range(sockfd), "Invalid sockfd");
        map_[sockfd].reset();
    }

   private:
    inline bool in_range(int sockfd) const { return sockfd < size_; }

    void Resize(const int min_size) {
        const int old_size = size_;
        while (!in_range(min_size)) {
            size_ *= 2;
        }

        value_type* new_map = new value_type[size_];
        for (int i = 0; i < old_size; ++i) {
            new_map[i] = std::move(map_[i]);
        }
        delete[] map_;

        map_ = new_map;
    }

    // Guards public methods
    std::mutex mu_;

    int size_;
    value_type* map_;
};
}
