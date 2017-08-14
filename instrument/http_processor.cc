#include "http_processor.h"

namespace microtrace {

static PrefixTreeNode BuildTree(const std::vector<std::string>& list);

static const std::vector<std::string> METHODS{
    "GET", "POST", "PUT", "DELETE", "HEAD", "OPTIONS", "TRACE"};

const PrefixTreeNode& tree() {
    static const PrefixTreeNode tree_ = BuildTree(METHODS);
    return tree_;
}

static PrefixTreeNode BuildTree(const std::vector<std::string>& list) {
    PrefixTreeNode head;

    for (const auto& str : list) {
        PrefixTreeNode* curr = &head;
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
    if (!(c >= 0 && c < MAX_CHAR)) {
        return nullptr;
    }
    if (!children_[c]) {
        return nullptr;
    }
    return children_[c].get();
}

PrefixTree::PrefixTree(const PrefixTreeNode* head) : current_(head) {}

bool PrefixTree::Advance(const char c) {
    VERIFY(current_, "Advance called");
    current_ = current_->Get(c);
    return current_;
}

HttpProcessor::HttpProcessor()
    : state_(State::METHOD), valid_(true), has_url_(false), tree_(&tree()) {}

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
                    valid_ = tree_.Advance(buf[i]);
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
