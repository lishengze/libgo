#pragma once
#include <atomic>

namespace libgo
{

struct LinkedList;

struct LinkedNode
{
    LinkedNode* prev = nullptr;
    LinkedNode* next = nullptr;

    inline bool is_linked() {
        return prev || next;
    }
};

struct LinkedList
{
public:
    LinkedList() {
        clear();
    }

    void clear()
    {
        dummy_.prev = dummy_.next = nullptr;
        head_ = &dummy_;
        tail_ = &dummy_;
    }

    void push(LinkedNode* node)
    {
        tail_->next = node;
        node->prev = tail_;
        node->next = nullptr;
        tail_ = node;
    }

    LinkedNode* front()
    {
        return head_->next;
    }

    bool unlink(LinkedNode* node)
    {
        if (tail_ == node) {
            tail_ = tail_->prev;
            tail_->next = nullptr;
            node->prev = node->next = nullptr;
            return true;
        }

        bool unlinked = false;
        if (node->prev) {
            node->prev->next = node->next;
            node->prev = nullptr;
            unlinked = true;
        }

        if (node->next) {
            node->next->prev = node->prev;
            node->next = nullptr;
            unlinked = true;
        }
        return unlinked;
    }

private:
    LinkedNode* head_;
    LinkedNode* tail_;
    LinkedNode dummy_;
};

} // namespace libgo
