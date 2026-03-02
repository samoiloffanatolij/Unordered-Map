// Solution folder: https://gitlab.manytask.org/mipt-cpp/students-2025-spring/samoilov.av/-/tree/b56d9842981822db1cfe8280b5669bdf1fa0bd6c/unordered_map
// Job link: https://gitlab.manytask.org/mipt-cpp/students-2025-spring/samoilov.av/-/jobs/946009

#pragma once

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

template <class T, class Allocator = std::allocator<T>>
class ForwardList {

    template <class Key, class Value, class Hash, class KeyEqual, class Alloc>
    friend class UnorderedMap;

    struct BaseNode {
        BaseNode* next;
        BaseNode()
            : next(nullptr) {
        }
    };

    struct Node : BaseNode {
        T value;

        Node(T&& value)
            : value(std::move(value)) {
        }

        Node(const T& value)
            : value(value) {
        }

        template <typename... Args>
        Node(Args&&... args)
            : value(std::forward<Args>(args)...) {
        }
    };

    BaseNode fake_node_;

    using AllocType = typename std::allocator_traits<Allocator>::template rebind_alloc<Node>;
    using AllocTraits = std::allocator_traits<AllocType>;

    [[no_unique_address]] AllocType node_alloc_;

    template <bool is_const>
    class BaseIterator {
        template <class Key, class Value, class Hash, class Equal, class Alloc>
        friend class UnorderedMap;
        friend class ForwardList;

        using BaseNodeType = std::conditional_t<is_const, const BaseNode, BaseNode>;
        using NodeType = std::conditional_t<is_const, const Node, Node>;

        BaseNodeType* ptr_;

    public:
        using value_type = std::conditional_t<is_const, const T, T>;
        using reference_type = std::conditional_t<is_const, const value_type&, value_type&>;
        using pointer_type = std::conditional_t<is_const, const value_type*, value_type*>;
        using difference_type = std::ptrdiff_t;

        operator BaseIterator<true>() const noexcept {
            return BaseIterator<true>(ptr_);
        }

        reference_type operator*() const noexcept {
            return static_cast<NodeType*>(ptr_)->value;
        }
        pointer_type operator->() const noexcept {
            return &operator*();
        }
        BaseIterator& operator++() noexcept {
            ptr_ = ptr_->next;
            return *this;
        }
        BaseIterator operator++(int) noexcept {
            BaseIterator temp = *this;
            ptr_ = ptr_->next;
            return temp;
        }

        explicit BaseIterator(BaseNodeType* ptr = nullptr)
            : ptr_(ptr) {
        }
        BaseIterator(const BaseIterator& it)
            : ptr_(it.ptr_) {
        }
        BaseIterator& operator=(const BaseIterator& it) = default;

        template <bool is_const_it>
        bool operator==(const BaseIterator<is_const_it>& it) const noexcept {
            return ptr_ == it.ptr_;
        }
    };

public:
    using iterator = BaseIterator<false>;
    using const_iterator = BaseIterator<true>;

    iterator begin() noexcept {
        return iterator(fake_node_.next);
    }
    const_iterator begin() const noexcept {
        return const_iterator(fake_node_.next);
    }
    iterator end() noexcept {
        return iterator(nullptr);
    }
    const_iterator end() const noexcept {
        return const_iterator(nullptr);
    }
    const_iterator cbegin() const noexcept {
        return begin();
    }
    const_iterator cend() const noexcept {
        return end();
    }

    iterator before_begin() noexcept {
        return iterator(&fake_node_);
    }
    const_iterator cbefore_begin() const noexcept {
        return const_iterator(&fake_node_);
    }

    bool empty() const noexcept {
        return begin() == end();
    }

    void pop_front() {
        if (!empty()) {
            erase_next(cbefore_begin());
        }
    }

    ForwardList(const Allocator& alloc = Allocator())
        : node_alloc_(alloc) {
    }
    ~ForwardList() {
        while (!empty()) {
            pop_front();
        }
    }

    ForwardList(const ForwardList& list)
        : node_alloc_(AllocTraits::select_on_container_copy_construction(list.node_alloc_)) {
        iterator pos = before_begin();
        for (const iterator& val : list) {
            pos = emplace_next(pos, val);
        }
    }

    ForwardList(ForwardList&& list) noexcept
        : fake_node_{},
          node_alloc_(std::move(list.node_alloc_)) {
        fake_node_.next = list.fake_node_.next;
        list.fake_node_.next = nullptr;
    }

    ForwardList& operator=(const ForwardList& list) {
        if (this == &list) {
            return *this;
        }
        ForwardList temp(list);
        swap(temp);
        return *this;
    }

    ForwardList& operator=(ForwardList&& list) noexcept {
        if (this == &list) {
            return *this;
        }
        while (!empty()) {
            pop_front();
        }
        swap(list);
        return *this;
    }

    template <typename... Args>
    iterator emplace_next(const_iterator pos, Args&&... args) {
        Node* node = AllocTraits::allocate(node_alloc_, 1);
        try {
            AllocTraits::construct(node_alloc_, node, std::forward<Args>(args)...);
            node->next = pos.ptr_->next;
            pos.ptr_->next = node;
        } catch (...) {
            AllocTraits::deallocate(node_alloc_, node, 1);
            throw;
        }
        return iterator(node);
    }

    iterator erase_next(const_iterator pos) {
        Node* node = static_cast<Node*>(const_cast<BaseNode*>(pos.ptr_)->next);
        const_cast<BaseNode*>(pos.ptr_)->next = node->next;
        AllocTraits::destroy(node_alloc_, node);
        AllocTraits::deallocate(node_alloc_, node, 1);
        return iterator(const_cast<BaseNode*>(pos.ptr_)->next);
    }

    static iterator InsertNodeNext(const_iterator it, Node* node) {
        node->next = it.ptr_->next;
        const_cast<BaseNode*>(it.ptr_)->next = node;
        return iterator(node);
    }

    Node* PopNodeNext(const_iterator it) {
        BaseNode* ptr = const_cast<BaseNode*>(it.ptr_);
        if (!ptr->next) {
            return nullptr;
        }
        Node* res = static_cast<Node*>(ptr->next);
        if (!ptr || !ptr->next) {
            return res;
        }
        BaseNode* next = ptr->next;
        ptr->next = ptr->next->next;
        next->next = nullptr;
        return res;
    }

    void swap(ForwardList& list) noexcept {
        if constexpr (AllocTraits::propagate_on_container_swap::value) {
            std::swap(node_alloc_, list.node_alloc_);
        }
        std::swap(fake_node_.next, list.fake_node_.next);
    }
};

template <class Key, class Value, class Hash = std::hash<Key>, class Equal = std::equal_to<Key>,
          class Allocator = std::allocator<std::pair<const Key, Value>>>
class UnorderedMap {
    using NodeType = std::pair<const Key, Value>;

    struct HashNode {
        NodeType key_val;
        std::size_t hash;

        void SetHash(const Hash& hash_func) {
            hash = hash_func(key_val.first);
        }
    };

    using List = ForwardList<HashNode, Allocator>;
    using ListNode = typename List::Node;

    template <bool is_const>
    using list_iterator = typename List::template BaseIterator<is_const>;

public:
    class MapOutOfRange final : public std::exception {
    public:
        const char* what() const noexcept override {
            return "UnorderedMap::at(): key not found";
        }
    };

    template <bool is_const>
    class BaseIterator {
        friend class UnorderedMap;

        list_iterator<is_const> list_iter_;
        std::size_t GetHash() const {
            return list_iter_->hash;
        }

    public:
        using value_type = NodeType;
        using reference_type = std::conditional_t<is_const, const value_type&, value_type&>;
        using pointer_type = std::conditional_t<is_const, const value_type*, value_type*>;
        using difference_type = std::ptrdiff_t;

        BaseIterator() = default;
        explicit BaseIterator(list_iterator<is_const> it)
            : list_iter_(it) {
        }

        BaseIterator(const BaseIterator&) = default;
        BaseIterator& operator=(const BaseIterator&) = default;

        operator BaseIterator<true>() const {
            return BaseIterator<true>(list_iter_);
        }

        reference_type operator*() const {
            return list_iter_->key_val;
        }
        pointer_type operator->() const {
            return &list_iter_->key_val;
        }
        BaseIterator& operator++() {
            ++list_iter_;
            return *this;
        }
        BaseIterator operator++(int) {
            BaseIterator temp = *this;
            ++*this;
            return temp;
        }

        template <bool is_const_it>
        bool operator==(const BaseIterator<is_const_it>& it) const {
            return list_iter_ == it.list_iter_;
        }
    };

    using iterator = BaseIterator<false>;
    using const_iterator = BaseIterator<true>;

private:
    struct Bucket {
        typename List::iterator prev_it;
        typename List::iterator elem_it;
        Bucket(typename List::iterator prev, typename List::iterator elem)
            : prev_it(prev),
              elem_it(elem) {
        }
    };

    static constexpr std::size_t kDefaultBucketCount = 1;
    static constexpr float kDefaultMaxLoadFactor = 1;

    using BucketVector =
        std::vector<Bucket,
                    typename std::allocator_traits<Allocator>::template rebind_alloc<Bucket>>;

    List list_;
    BucketVector buckets_;

    std::size_t size_ = 0;
    float max_load_factor_ = kDefaultMaxLoadFactor;

    [[no_unique_address]] Hash hasher_;
    [[no_unique_address]] Equal equal_;
    [[no_unique_address]] Allocator alloc_;

    std::size_t bucket_id(std::size_t hash) const {
        return buckets_.empty() ? 0 : hash % buckets_.size();
    }

    void DestroyNode(ListNode* node) {
        using KeyValAllocator =
            typename std::allocator_traits<Allocator>::template rebind_alloc<NodeType>;
        using KeyValTraits = std::allocator_traits<KeyValAllocator>;

        KeyValAllocator kv_alloc(alloc_);
        KeyValTraits::destroy(kv_alloc, &node->value.key_val);

        using AllocType =
            typename std::allocator_traits<Allocator>::template rebind_alloc<ListNode>;
        using NodeTraits = std::allocator_traits<AllocType>;

        AllocType node_alloc(alloc_);
        NodeTraits::deallocate(node_alloc, node, 1);
    }

    iterator EmplaceNode(ListNode* node) {
        const std::size_t hash_val = node->value.hash;
        if (size_ + 1 > max_load_factor_ * bucket_count() || bucket_count() == 0) {
            rehash(bucket_count() == 0 ? kDefaultBucketCount : bucket_count() * 2);
        }

        std::size_t bucket_idx = bucket_id(hash_val);
        auto& bucket = buckets_[bucket_idx];

        auto insert_pos = bucket.prev_it;
        auto new_node_it = list_.InsertNodeNext(insert_pos, node);

        if (bucket.elem_it == list_.end() || bucket_id(bucket.elem_it->hash) != bucket_idx ||
            insert_pos == bucket.prev_it) {
            bucket.elem_it = new_node_it;
        }

        auto next_it = std::next(new_node_it);
        if (next_it != list_.end()) {
            std::size_t next_hash_idx = bucket_id(next_it->hash);
            if (next_hash_idx != bucket_idx) {
                buckets_[next_hash_idx].prev_it = new_node_it;
            }
        }

        ++size_;
        return iterator(new_node_it);
    }

public:
    UnorderedMap()
        : UnorderedMap(kDefaultBucketCount) {
    }
    explicit UnorderedMap(std::size_t bucket_count, const Hash& hash = Hash(),
                          const Equal& equal = Equal(), const Allocator& alloc = Allocator())
        : list_(alloc),
          buckets_(bucket_count > 0 ? bucket_count : kDefaultBucketCount,
                   Bucket(list_.before_begin(), list_.end()), alloc),
          hasher_(hash),
          equal_(equal),
          alloc_(alloc) {
    }

    template <class InputIter>
    UnorderedMap(InputIter first, InputIter last, std::size_t bucket_count = kDefaultBucketCount,
                 const Hash& hash = Hash(), const Equal& equal = Equal(),
                 const Allocator& alloc = Allocator())
        : UnorderedMap(bucket_count, hash, equal, alloc) {
        insert(first, last);
    }

    UnorderedMap(const UnorderedMap& map)
        : UnorderedMap(
              map.bucket_count(), map.hasher_, map.equal_,
              std::allocator_traits<Allocator>::select_on_container_copy_construction(map.alloc_)) {
        max_load_factor_ = map.max_load_factor_;
        insert(map.begin(), map.end());
    }

    UnorderedMap(UnorderedMap&& map) noexcept
        : list_(std::move(map.list_)),
          buckets_(std::move(map.buckets_)),
          size_(map.size_),
          max_load_factor_(map.max_load_factor_),
          hasher_(std::move(map.hasher_)),
          equal_(std::move(map.equal_)),
          alloc_(std::move(map.alloc_)) {
        auto* old_fake_node_ptr = &map.list_.fake_node_;
        for (Bucket& bucket : buckets_) {
            if (bucket.prev_it.ptr_ == old_fake_node_ptr) {
                bucket.prev_it = list_.before_begin();
            }
        }

        map.size_ = 0;
        map.buckets_.assign(kDefaultBucketCount, Bucket(map.list_.before_begin(), map.list_.end()));
    }

    UnorderedMap& operator=(const UnorderedMap& map) {
        if (this == &map) {
            return *this;
        }
        UnorderedMap temp(map);
        swap(temp);
        return *this;
    }

    UnorderedMap& operator=(UnorderedMap&& map) noexcept {
        if (this == &map) {
            return *this;
        }
        clear();

        list_ = std::move(map.list_);
        buckets_ = std::move(map.buckets_);
        hasher_ = std::move(map.hasher_);
        equal_ = std::move(map.equal_);
        size_ = map.size_;
        max_load_factor_ = map.max_load_factor_;

        if constexpr (std::allocator_traits<
                          Allocator>::propagate_on_container_move_assignment::value) {
            alloc_ = std::move(map.alloc_);
        }

        auto* old_fake_node = &map.list_.fake_node_;
        for (auto& bucket : buckets_) {
            if (bucket.prev_it.ptr_ == old_fake_node) {
                bucket.prev_it = list_.before_begin();
            }
        }

        map.size_ = 0;
        map.buckets_.clear();
        map.buckets_.assign(kDefaultBucketCount, Bucket(map.list_.before_begin(), map.list_.end()));
        return *this;
    }

    ~UnorderedMap() {
        clear();
    }

    iterator begin() {
        return iterator(list_.begin());
    }
    const_iterator begin() const {
        return const_iterator(list_.cbegin());
    }
    iterator end() {
        return iterator(list_.end());
    }
    const_iterator end() const {
        return const_iterator(list_.cend());
    }
    const_iterator cbegin() const {
        return const_iterator(list_.cbegin());
    }
    const_iterator cend() const {
        return const_iterator(list_.cend());
    }

    std::size_t size() const noexcept {
        return size_;
    }
    bool empty() const noexcept {
        return size_ == 0;
    }

    void clear() {
        while (!list_.empty()) {
            list_.pop_front();
        }
        std::fill(buckets_.begin(), buckets_.end(), Bucket(list_.before_begin(), list_.end()));
        size_ = 0;
    }

    template <typename... Args>
    ListNode* CreateNode(Args&&... args) {
        using KeyValAllocator =
            typename std::allocator_traits<Allocator>::template rebind_alloc<NodeType>;
        using KeyValTraits = std::allocator_traits<KeyValAllocator>;
        using AllocType =
            typename std::allocator_traits<Allocator>::template rebind_alloc<ListNode>;
        using NodeTraits = std::allocator_traits<AllocType>;

        AllocType node_alloc(alloc_);
        ListNode* node = NodeTraits::allocate(node_alloc, 1);
        try {
            KeyValAllocator kv_alloc(alloc_);
            KeyValTraits::construct(kv_alloc, &node->value.key_val, std::forward<Args>(args)...);
            node->value.SetHash(hasher_);
        } catch (...) {
            NodeTraits::deallocate(node_alloc, node, 1);
            throw;
        }
        return node;
    }

    template <typename... Args>
    std::pair<iterator, bool> emplace(Args&&... args) {
        ListNode* node = CreateNode(std::forward<Args>(args)...);
        auto found_it = find(node->value.key_val.first);
        if (found_it != end()) {
            DestroyNode(node);
            return {found_it, false};
        }
        return {EmplaceNode(node), true};
    }

    std::pair<iterator, bool> insert(const NodeType& value) {
        return emplace(value);
    }
    std::pair<iterator, bool> insert(NodeType&& value) {
        return emplace(std::move(value));
    }
    template <class InputIter>
    void insert(InputIter first, InputIter last) {
        for (InputIter it = first; it != last; ++it) {
            emplace(*it);
        }
    }
    template <class Type, class = std::enable_if_t<std::is_constructible_v<NodeType, Type&&>>>
    std::pair<iterator, bool> insert(Type&& value) {
        return emplace(std::forward<Type>(value));
    }

    iterator erase(const_iterator pos) {
        const std::size_t hash_val = pos.GetHash();
        std::size_t bucket_idx = bucket_id(hash_val);
        auto& bucket = buckets_[bucket_idx];

        auto it_before_erased = bucket.prev_it;
        if (pos.list_iter_ != list_iterator<true>(bucket.elem_it)) {
            for (it_before_erased = bucket.elem_it; std::next(it_before_erased) != pos.list_iter_;
                 ++it_before_erased) {
            }
        }

        auto it_next_erased = list_.erase_next(it_before_erased);

        if (pos.list_iter_ == list_iterator<true>(bucket.elem_it)) {
            if (it_next_erased == list_.end() || bucket_id(it_next_erased->hash) != bucket_idx) {
                bucket.elem_it = list_.end();
            } else {
                bucket.elem_it = it_next_erased;
            }
        }

        if (it_next_erased != list_.end()) {
            std::size_t next_bucket_idx = bucket_id(it_next_erased->hash);
            if (next_bucket_idx != bucket_idx) {
                if (buckets_[next_bucket_idx].prev_it == pos.list_iter_) {
                    buckets_[next_bucket_idx].prev_it = it_before_erased;
                }
            }
        }

        --size_;
        return iterator(it_next_erased);
    }

    iterator erase(const_iterator first, const_iterator last) {
        while (first != last) {
            first = erase(first);
        }
        return iterator(
            typename List::iterator(const_cast<typename List::BaseNode*>(first.list_iter_.ptr_)));
    }

    void swap(UnorderedMap& map) noexcept {
        std::swap(hasher_, map.hasher_);
        std::swap(equal_, map.equal_);
        std::swap(max_load_factor_, map.max_load_factor_);
        std::swap(size_, map.size_);

        list_.swap(map.list_);
        buckets_.swap(map.buckets_);

        if constexpr (std::allocator_traits<Allocator>::propagate_on_container_swap::value) {
            std::swap(alloc_, map.alloc_);
        }
    }

    Value& operator[](const Key& key) {
        return try_emplace(key).first->second;
    }
    Value& operator[](Key&& key) {
        return try_emplace(std::move(key)).first->second;
    }

    template <typename... Args>
    std::pair<iterator, bool> try_emplace(const Key& k, Args&&... args) {
        auto it = find(k);
        if (it != end()) {
            return {it, false};
        }
        return emplace(std::piecewise_construct, std::forward_as_tuple(k),
                       std::forward_as_tuple(std::forward<Args>(args)...));
    }
    template <typename... Args>
    std::pair<iterator, bool> try_emplace(Key&& k, Args&&... args) {
        auto it = find(k);
        if (it != end()) {
            return {it, false};
        }
        return emplace(std::piecewise_construct, std::forward_as_tuple(std::move(k)),
                       std::forward_as_tuple(std::forward<Args>(args)...));
    }

    iterator find(const Key& key) {
        if (empty()) {
            return end();
        }
        std::size_t hash_val = hasher_(key);
        std::size_t b_idx = bucket_id(hash_val);

        for (auto it = buckets_[b_idx].elem_it; it != list_.end() && bucket_id(it->hash) == b_idx;
             ++it) {
            if (it->hash == hash_val && equal_(it->key_val.first, key)) {
                return iterator(it);
            }
        }
        return end();
    }

    const_iterator find(const Key& key) const {
        if (empty()) {
            return cend();
        }
        std::size_t hash_val = hasher_(key);
        std::size_t bucket_idx = bucket_id(hash_val);

        for (auto it = const_iterator(buckets_[bucket_idx].elem_it);
             it != cend() && bucket_id(it.GetHash()) == bucket_idx; ++it) {
            if (it.GetHash() == hash_val && equal_(it->first, key)) {
                return it;
            }
        }
        return cend();
    }

    Value& at(const Key& key) {
        auto it = find(key);
        if (it == end()) {
            throw MapOutOfRange();
        }
        return it->second;
    }
    const Value& at(const Key& key) const {
        auto it = find(key);
        if (it == end()) {
            throw MapOutOfRange();
        }
        return it->second;
    }

    std::size_t bucket_count() const {
        return buckets_.size();
    }
    float load_factor() const {
        return bucket_count() > 0 ? static_cast<float>(size_) / bucket_count() : 0.0f;
    }
    float max_load_factor() const {
        return max_load_factor_;
    }

    void max_load_factor(const float max_load_factor) {
        max_load_factor_ = max_load_factor;
        if (bucket_count() > 0 && load_factor() > max_load_factor_) {
            rehash(0);
        }
    }

    void rehash(const std::size_t count) {
        std::size_t new_bucket_count = std::max(
            count,
            static_cast<std::size_t>(std::ceil(static_cast<double>(size()) / max_load_factor_)));
        if (new_bucket_count == 0) {
            new_bucket_count = 1;
        }
        if (new_bucket_count <= bucket_count()) {
            return;
        }

        List old_list;
        old_list.swap(list_);

        size_ = 0;
        buckets_.assign(new_bucket_count, Bucket(list_.before_begin(), list_.end()));

        while (!old_list.empty()) {
            ListNode* node = old_list.PopNodeNext(old_list.cbefore_begin());
            EmplaceNode(node);
        }
    }

    void reserve(const std::size_t count) {
        rehash(static_cast<std::size_t>(std::ceil(static_cast<double>(count) / max_load_factor_)));
    }
};
