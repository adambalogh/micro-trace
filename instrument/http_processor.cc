#include "http_processor.h"

namespace microtrace {

static PrefixTreeNode BuildTree(const std::vector<std::string>& list) {
    PrefixTreeNode head;
    PrefixTreeNode* curr = &head;

    for (const auto& str : list) {
        for (size_t i = 0; i < str.size(); ++i) {
            auto* n = curr->Get(str[i]);
            if (n == nullptr) {
                curr->Add(str[i]);
            }
            curr = curr->Get(str[i]);
        }
        curr->mark_finish();
    }
    return head;
}

void PrefixTreeNode::Add(const char c) {
    VERIFY(c >= 0 && c <= MAX_CHAR, "char outside range: {}", c);
    children_[c] = std::make_unique<PrefixTreeNode>();
}

PrefixTreeNode* PrefixTreeNode::Get(const char c) const {
    VERIFY(c >= 0 && c <= MAX_CHAR, "char outside range: {}", c);
    if (!children_[c]) {
        return nullptr;
    }
    return children_[c].get();
}

PrefixTree::PrefixTree(const PrefixTreeNode* head) : current_(head) {}

bool PrefixTree::Advance(char c) {
    VERIFY(current_, "Advance called");
    current_ = current_->Get(c);
    return current_;
}

static const std::vector<std::string> METHODS{
    "GET", "POST", "PUT", "DELETE", "HEAD", "OPTIONS", "TRACE"};

static const PrefixTreeNode tree = BuildTree(METHODS);

HttpProcessor::HttpProcessor() : valid_(true), tree_(&tree) {}

bool HttpProcessor::Process(const char* buf, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        if (!valid_) break;

        switch (state_) {
            case State::METHOD:
                if (buf[i] == SPACE) {
                    if (!tree_.is_finish()) {
                        valid_ = false;
                        break;
                    } else {
                        state_ = State::URL;
                    }
                } else {
                    if (!tree_.Advance(buf[i])) {
                        valid_ = false;
                    }
                }
                break;
            case State::URL:
                // Indicates end of URL
                if (buf[i] == SPACE) {
                    // URL cannot be empty
                    if (!url_buffer_.empty()) {
                        has_url_ = true;
                    }
                    valid_ = false;
                } else {
                    url_buffer_ += buf[i];
                }
                break;
        }
    }
    return valid_;
}
}
