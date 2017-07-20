#pragma once

#include <algorithm>
#include <mutex>
#include <shared_mutex>

#include "common.h"
#include "socket_interface.h"

namespace microtrace {

/*
 * SocketMap associates file descriptors with SocketInterfaces.
 *
 * The public methods never throw an exception or do abort if an invalid fd is
 * used, instead they return null or do nothing.
 */
class SocketMap {
   public:
    typedef std::unique_ptr<SocketInterface> value_type;

    // Should be power of 2
    const static int DEFAULT_SIZE = 1024;

    SocketMap() : size_(DEFAULT_SIZE), map_(new value_type[DEFAULT_SIZE]) {}
    ~SocketMap() { delete[] map_; }

    SocketMap(const SocketMap&) = delete;
    SocketMap(SocketMap&&) = delete;

    SocketInterface* Get(const int sockfd) const {
        std::shared_lock<mutex_type> l(mu_);
        if (!in_range(sockfd)) {
            return nullptr;
        }
        return map_[sockfd].get();
    }

    void Set(const int sockfd, value_type val) {
        std::unique_lock<mutex_type> l(mu_);
        if (!in_range(sockfd)) {
            Resize(sockfd);
        }
        LOG_ERROR_IF(static_cast<bool>(map_[sockfd]) == true,
                     "Socket created twice: {}", sockfd);
        map_[sockfd] = std::move(val);
    }

    void Delete(const int sockfd) {
        std::unique_lock<mutex_type> l(mu_);
        if (in_range(sockfd)) {
            map_[sockfd].reset();
        }
    }

   private:
    // Note: there is no shared_mutex in GCC 5
    typedef std::shared_timed_mutex mutex_type;

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
    mutable mutex_type mu_;

    int size_;
    value_type* map_;
};
}
