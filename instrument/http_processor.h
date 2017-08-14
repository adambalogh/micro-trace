#pragma once

#include <array>
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

    bool is_finish() const {
        if (!current_) return false;
        return current_->is_finish();
    }

   private:
    const PrefixTreeNode* current_;
};

class HttpProcessor {
   public:
    HttpProcessor();

    bool Process(const char* buf, size_t len);

    bool has_url() const { return has_url_; }

    const std::string& url() const {
        VERIFY(has_url_, "url called when not set");
        return url_buffer_;
    }

   private:
    static const char SPACE = ' ';
    enum class State { METHOD, URL };

    State state_;
    bool valid_;

    std::string url_buffer_;
    bool has_url_;

    PrefixTree tree_;
};
}
