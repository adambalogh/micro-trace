#pragma once

#include <array>
#include <functional>
#include <string>

#include "common.h"

namespace microtrace {

class PrefixTreeNode {
   public:
    // Adds a char to the tree
    void Add(const char c);

    // Returns the node assoc. to c, nullptr if empty.
    PrefixTreeNode* Get(const char c) const;

    void mark_finish() { finish_ = true; }
    bool is_finish() const { return finish_; }

   private:
    static const int MAX_CHAR = 256;

    bool finish_;

    std::array<std::unique_ptr<PrefixTreeNode>, MAX_CHAR> children_;
};

class PrefixTree {
   public:
    PrefixTree(const PrefixTreeNode* head);

    bool Advance(char c);

    bool is_finish() const { return current_->is_finish(); }

   private:
    const PrefixTreeNode* current_;
};

class HttpProcessor {
   public:
    HttpProcessor(std::function<void(std::string)> url_cb);

    bool Process(const char* buf, size_t len);

   private:
    static const char SPACE = ' ';
    enum class State { METHOD, URL };

    State state_ = State::METHOD;
    bool valid_;

    std::function<void(std::string)> url_cb_;
    std::string url_buffer_;

    PrefixTree tree_;
};
}
