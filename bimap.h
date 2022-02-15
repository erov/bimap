#pragma once

#include "tree.h"
#include <functional>
#include <utility>

template <typename Left, typename Right,
          typename CompareLeft = std::less<Left>,
          typename CompareRight = std::less<Right>>
struct bimap {
private:
  using left_t = Left;
  using right_t = Right;
  using left_tree_t = bimap_impl::tree<Left, CompareLeft, bimap_impl::left_tag>;
  using right_tree_t = bimap_impl::tree<Right, CompareRight, bimap_impl::right_tag>;
  using bimap_node_t = bimap_impl::bimap_node<Left, Right>;
  using left_node_t = bimap_impl::tree_node<Left, bimap_impl::left_tag>;
  using right_node_t = bimap_impl::tree_node<Right, bimap_impl::right_tag>;
  using node_base_t = bimap_impl::tree_base_node;

  template <typename Value, typename CompareValue, typename Tag,
            typename FlipValue, typename FlipCompareValue, typename FlipTag>
  struct base_iterator;

public:
  using left_iterator = base_iterator<Left, CompareLeft, bimap_impl::left_tag, Right, CompareRight, bimap_impl::right_tag>;
  using right_iterator = base_iterator<Right, CompareRight, bimap_impl::right_tag, Left, CompareLeft, bimap_impl::left_tag>;

  bimap(CompareLeft compare_left = CompareLeft(),
        CompareRight compare_right = CompareRight())
      : left_tree(std::move(compare_left)),
        right_tree(std::move(compare_right)) {
    left_tree.connect(right_tree);
  }

  bimap(bimap const& other)
      : bimap(other.left_tree.get_comparator(), other.right_tree.get_comparator()) {
    for (left_iterator iter = other.begin_left(); iter != other.end_left(); ++iter) {
      insert(*iter, *iter.flip());
    }
  }

  bimap(bimap&& other) noexcept : bimap() {
    other.swap(*this);
  }

  bimap& operator=(bimap const& other) {
    if (this != &other) {
      bimap(other).swap(*this);
    }
    return *this;
  }

  bimap& operator=(bimap&& other) noexcept {
    if (this != &other) {
      bimap(std::move(other)).swap(*this);
    }
    return *this;
  }

  ~bimap() {
    erase_left(begin_left(), end_left());
  }


  left_iterator insert(left_t const& left, right_t const& right) {
    return insert_impl(left, right);
  }

  left_iterator insert(left_t const& left, right_t&& right) {
    return insert_impl(left, std::move(right));
  }

  left_iterator insert(left_t&& left, right_t const& right) {
    return insert_impl(std::move(left), right);
  }

  left_iterator insert(left_t&& left, right_t&& right) {
    return insert_impl(std::move(left), std::move(right));
  }


  left_iterator erase_left(left_iterator it) {
    return remove(static_cast<bimap_node_t*>(
                      static_cast<left_node_t*>(it.src_node))).first;
  }

  bool erase_left(left_t const& left) {
    left_node_t* left_node = left_tree.find(left);
    if (left_node != nullptr) {
      remove(static_cast<bimap_node_t*>(left_node));
      return true;
    }
    return false;
  }

  right_iterator erase_right(right_iterator it) {
    return remove(static_cast<bimap_node_t*>(
                      static_cast<right_node_t*>(it.src_node))).second;
  }

  bool erase_right(right_t const& right) {
    right_node_t* right_node = right_tree.find(right);
    if (right_node != nullptr) {
      remove(static_cast<bimap_node_t*>(right_node));
      return true;
    }
    return false;
  }

  left_iterator erase_left(left_iterator first, left_iterator last) {
    left_iterator iter(first);
    while (first != last) {
      ++first;
      erase_left(iter++);
    }
    return last;
  }

  right_iterator erase_right(right_iterator first, right_iterator last) {
    right_iterator iter(first);
    while (first != last) {
      ++first;
      erase_right(iter++);
    }
    return last;
  }


  left_iterator find_left(left_t const& left) const {
    left_node_t* left_node = left_tree.find(left);
    if (left_node == nullptr) {
      return end_left();
    }
    return left_iterator(static_cast<node_base_t*>(left_node));
  }

  right_iterator find_right(right_t const& right) const {
    right_node_t* right_node = right_tree.find(right);
    if (right_node == nullptr) {
      return end_right();
    }
    return right_iterator(static_cast<node_base_t*>(right_node));
  }


  right_t const& at_left(left_t const& key) const {
    left_node_t* left_node = left_tree.find(key);
    if (left_node == nullptr) {
      throw std::out_of_range("no entry exists");
    }
    return switch_node(left_node)->value();
  }

  left_t const& at_right(right_t const& key) const {
    right_node_t* right_node = right_tree.find(key);
    if (right_node == nullptr) {
      throw std::out_of_range("no entry exists");
    }
    return switch_node(right_node)->value();
  }


  template <typename U = right_t>
  std::enable_if_t<std::is_default_constructible_v<U>, U const&> at_left_or_default(left_t const& left) {
    left_node_t* left_node = left_tree.find(left);
    if (left_node == nullptr) {
      right_t right = right_t();
      right_node_t* right_node = right_tree.find(right);
      if (right_node != nullptr) {
        erase_right(right_iterator(static_cast<node_base_t*>(right_node)));
      }
      return *insert(left, right).flip();
    }
    return switch_node(left_node)->value();
  }

  template <typename U = left_t>
  std::enable_if_t<std::is_default_constructible_v<U>, U const&> at_right_or_default(right_t const& right) {
    right_node_t* right_node = right_tree.find(right);
    if (right_node == nullptr) {
      left_t left = left_t();
      left_node_t* left_node = left_tree.find(left);
      if (left_node != nullptr) {
        erase_left(left_iterator(static_cast<node_base_t*>(left_node)));
      }
      return *insert(left, right);
    }
    return switch_node(right_node)->value();
  }


  left_iterator lower_bound_left(const left_t& left) const {
    return left_iterator(left_tree.lower_bound(left));
  }

  left_iterator upper_bound_left(const left_t& left) const {
    return left_iterator(left_tree.upper_bound(left));
  }


  right_iterator lower_bound_right(const right_t& right) const {
    return right_iterator(right_tree.lower_bound(right));
  }

  right_iterator upper_bound_right(const right_t& right) const {
    return right_iterator(right_tree.upper_bound(right));
  }


  left_iterator begin_left() const {
    return left_iterator(left_tree.get_begin());
  }

  left_iterator end_left() const {
    return left_iterator(left_tree.get_end());
  }


  right_iterator begin_right() const {
    return right_iterator(right_tree.get_begin());
  }

  right_iterator end_right() const {
    return right_iterator(right_tree.get_end());
  }


  bool empty() const {
    return tree_size == 0;
  }

  size_t size() const {
    return tree_size;
  }


  friend bool operator==(bimap const& a, bimap const& b) {
    return a.compare_equal(b);
  }

  friend bool operator!=(bimap const& a, bimap const& b) {
    return !a.compare_equal(b);
  }

  void swap(bimap& other) noexcept {
    left_tree.swap(other.left_tree);
    right_tree.swap(other.right_tree);
    std::swap(tree_size, other.tree_size);
  }

private:
  static left_node_t* switch_node(right_node_t* node) noexcept {
    return static_cast<left_node_t*>(static_cast<bimap_node_t*>(node));
  }

  static right_node_t* switch_node(left_node_t* node) noexcept {
    return static_cast<right_node_t*>(static_cast<bimap_node_t*>(node));
  }

  bool compare_equal(bimap const& other) const {
    if (size() != other.size()) {
      return false;
    }
    for (left_iterator a_iter = begin_left(), b_iter = other.begin_left();
         a_iter != end_left() && b_iter != other.end_left(); ++a_iter, ++b_iter) {
      if (!left_tree.compare_equal(*a_iter, *b_iter) ||
          !right_tree.compare_equal(*a_iter.flip(), *b_iter.flip())) {
        return false;
      }
    }
    return true;
  }

  template <typename ArgLeft, typename ArgRight>
  left_iterator insert_impl(ArgLeft&& left, ArgRight&& right) {
    if (left_tree.find(left) != nullptr || right_tree.find(right) != nullptr) {
      return end_left();
    }
    bimap_node_t* bimap_node = new bimap_node_t(
        std::forward<ArgLeft>(left), std::forward<ArgRight>(right));
    right_tree.insert(static_cast<right_node_t*>(bimap_node));
    left_node_t* left_node = left_tree.insert(static_cast<left_node_t*>(bimap_node));
    ++tree_size;
    return left_iterator(static_cast<node_base_t*>(left_node));
  }

  std::pair<left_iterator, right_iterator> remove(bimap_node_t* bimap_node) {
    node_base_t* left_node = left_tree.remove(static_cast<left_node_t*>(bimap_node));
    node_base_t* right_node = right_tree.remove(static_cast<right_node_t*>(bimap_node));
    --tree_size;
    delete bimap_node;
    return {left_iterator(left_node), right_iterator(right_node)};
  }

  template <typename Value, typename Compare, typename Tag,
            typename FlipValue, typename FlipCompare, typename FlipTag>
  struct base_iterator {
  private:
    using value_t = Value;
    using node_t = bimap_impl::tree_node<Value, Tag>;
    using tree_t = bimap_impl::tree<Value, Compare, Tag>;
    using node_base_t = bimap_impl::tree_base_node;

  public:
    base_iterator() = default;

    value_t const& operator*() const {
      return static_cast<node_t*>(src_node)->value();
    }

    value_t const* operator->() const {
      return &static_cast<node_t*>(src_node)->value();
    }


    base_iterator& operator++() {
      src_node = tree_t::next(src_node);
      return *this;
    }

    base_iterator operator++(int) {
      base_iterator old(*this);
      ++(*this);
      return old;
    }


    base_iterator& operator--() {
      src_node = tree_t::prev(src_node);
      return *this;
    }

    base_iterator operator--(int) {
      base_iterator old(*this);
      --(*this);
      return old;
    }


    base_iterator<FlipValue, FlipCompare, FlipTag, Value, Compare, Tag> flip() const {
      if (src_node->is_end()) {
        return base_iterator<FlipValue, FlipCompare, FlipTag, Value, Compare, Tag>(
            src_node->get_right());
      }
      return base_iterator<FlipValue, FlipCompare, FlipTag, Value, Compare, Tag>(
          static_cast<node_base_t*>(switch_node(static_cast<node_t*>(src_node))));
    }


    friend bool operator==(base_iterator const& lhs, base_iterator const& rhs) {
      return lhs.src_node == rhs.src_node;
    }

    friend bool operator!=(base_iterator const& lhs, base_iterator const& rhs) {
      return lhs.src_node != rhs.src_node;
    }


    template <typename Left_, typename Right_,
              typename CompareLeft_, typename CompareRight_>
    friend struct bimap;

  private:
    explicit base_iterator(node_base_t* src_node) noexcept : src_node(src_node) {}

    node_base_t* src_node{nullptr};
  };

  left_tree_t left_tree;
  right_tree_t right_tree;
  size_t tree_size{0};
};
