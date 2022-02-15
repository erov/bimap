#pragma once

#include <cassert>
#include <cstddef>
#include <memory>
#include <optional>

namespace bimap_impl {
  struct left_tag;
  struct right_tag;

  template <typename T, typename Compare, typename Tag>
  struct tree;


  struct tree_base_node {
    static size_t get_height(tree_base_node* point) noexcept;

    ptrdiff_t get_balance() const noexcept;

    void upd_height() noexcept;

    void upd_left() noexcept;

    void upd_kids() noexcept;

    tree_base_node* rotate_left() noexcept;

    tree_base_node* rotate_right() noexcept;

    tree_base_node* balance() noexcept;

    tree_base_node* get_min() const noexcept;

    static tree_base_node* remove_min(tree_base_node* point) noexcept;

    tree_base_node* get_max() const noexcept;

    bool is_end() const noexcept;

    tree_base_node* get_right() const noexcept;

    template <typename T_, typename Compare_, typename Tag_>
    friend struct tree;

  private:
    tree_base_node* left{nullptr};
    tree_base_node* right{nullptr};
    tree_base_node* parent{nullptr};
    size_t height{1};
  };


  template <typename T, typename Tag>
  struct tree_node : tree_base_node {
    template <typename Arg>
    tree_node(Arg&& value_)
        : value_(std::forward<Arg>(value_)) {}

    T const& value() noexcept {
      return value_;
    }

  private:
    T value_;
  };


  template <typename Left, typename Right>
  struct bimap_node : tree_node<Left, left_tag>, tree_node<Right, right_tag> {

    template <typename ArgLeft, typename ArgRight>
    bimap_node(ArgLeft&& left, ArgRight&& right)
        : tree_node<Left, left_tag>(std::forward<ArgLeft>(left)),
          tree_node<Right, right_tag>(std::forward<ArgRight>(right)) {}
  };


  template <typename T, typename Compare, typename Tag>
  struct tree {
  private:
    using node_t = tree_node<T, Tag>;
    using node_base_t = tree_base_node;

  public:
    tree() = default;

    tree(Compare&& compare) noexcept
        : compare(std::move(compare)) {}

    node_t* find(T const& key) const {
      node_t* point = static_cast<node_t*>(fake.left);
      while (point != nullptr) {
        if (compare(key, point->value())) {
          point = static_cast<node_t*>(point->left);
          continue;
        }
        if (compare(point->value(), key)) {
          point = static_cast<node_t*>(point->right);
          continue;
        }
        break;
      }
      return point;
    }

    node_t* insert(node_t* node) {
      fake.left = static_cast<node_base_t*>(insert_impl(static_cast<node_t*>(fake.left), node));
      fake.upd_left();
      check_invariant(static_cast<node_t*>(fake.left));
      return node;
    }

    node_base_t* remove(node_t* src) {
      node_base_t* src_next = next(static_cast<node_base_t*>(src));
      fake.left = static_cast<node_base_t*>(remove_impl(static_cast<node_t*>(fake.left), src));
      fake.upd_left();
      check_invariant(static_cast<node_t*>(fake.left));
      assert(lower_bound(src->value()) == src_next);
      return src_next;
    }

    node_base_t* get_begin() const noexcept {
      return fake.get_min();
    }

    node_base_t* get_end() const noexcept {
      return const_cast<node_base_t*>(&fake);
    }

    Compare const& get_comparator() const noexcept {
      return compare;
    }

    static node_base_t* next(node_base_t* point) noexcept {
      if (point->right != nullptr) {
        return point->right->get_min();
      }
      node_base_t* parent = point->parent;
      while (parent != nullptr && point == parent->right) {
        point = parent;
        parent = parent->parent;
      }
      return parent;
    }

    static node_base_t* prev(node_base_t* point) noexcept {
      if (point->left != nullptr) {
        return point->left->get_max();
      }
      node_base_t* parent = point->parent;
      while (parent != nullptr && point == parent->left) {
        point = parent;
        parent = parent->parent;
      }
      return parent;
    }

    node_base_t* lower_bound(T const& key) const {
      node_base_t* result = const_cast<node_base_t*>(&fake);
      node_base_t* point = fake.left;
      while (point != nullptr) {
        if (!compare(static_cast<node_t*>(point)->value(), key)) {
          result = point;
          point = point->left;
        } else {
          point = point->right;
        }
      }
      return result;
    }

    node_base_t* upper_bound(T const& key) const {
      node_base_t* point = lower_bound(key);
      return point != &fake && !compare(key, static_cast<node_t*>(point)->value()) ? next(point) : point;
    }

    bool compare_equal(T const& lhs, T const& rhs) const {
      return !compare(lhs, rhs) && !compare(rhs, lhs);
    }

    void swap(tree& other) noexcept {
      std::swap(fake.left, other.fake.left);
      fake.upd_left();
      other.fake.upd_left();
      std::swap(compare, other.compare);
    }

    template <typename T_, typename Compare_, typename Tag_>
    void connect(tree<T_, Compare_, Tag_>& other) noexcept {
      get_end()->right = other.get_end();
      other.get_end()->right = get_end();
    }

  private:
    node_t* insert_impl(node_t* point, node_t* node) {
      if (point == nullptr) {
        return node;
      }
      if (compare(node->value(), point->value())) {
        point->left = static_cast<node_base_t*>(insert_impl(static_cast<node_t*>(point->left), node));
        point->upd_kids();
        return static_cast<node_t*>(point->balance());
      }
      if (compare(point->value(), node->value())) {
        point->right = static_cast<node_base_t*>(insert_impl(static_cast<node_t*>(point->right), node));
        point->upd_kids();
      }
      return static_cast<node_t*>(point->balance());
    }

    node_t* remove_impl(node_t* point, node_t* node) {
      if (point == nullptr) {
        return nullptr;
      }
      if (compare(node->value(), point->value())) {
        point->left = static_cast<node_base_t*>(remove_impl(static_cast<node_t*>(point->left), node));
        point->upd_kids();
        return static_cast<node_t*>(point->balance());
      }
      if (compare(point->value(), node->value())) {
        point->right = static_cast<node_base_t*>(remove_impl(static_cast<node_t*>(point->right), node));
        point->upd_kids();
        return static_cast<node_t*>(point->balance());
      }

      node_t* left = static_cast<node_t*>(point->left);
      node_t* right = static_cast<node_t*>(point->right);
      if (right == nullptr) {
        return left;
      }
      node_t* minimal = static_cast<node_t*>(right->get_min());
      minimal->right = node_base_t::remove_min(right);
      minimal->left = static_cast<node_base_t*>(left);
      minimal->parent = point->parent;
      minimal->upd_kids();
      return static_cast<node_t*>(minimal->balance());
    }

    void check_invariant(node_t* point) {
      #ifdef DEBUG
        if (point == nullptr) {
          return;
        }

        if (point->left != nullptr) {
          assert(static_cast<node_t*>(point->left->parent) == point);
          assert(compare(static_cast<node_t*>(point->left)->value(), point->value()));
          check_invariant(static_cast<node_t*>(point->left));
        }

        if (point->right != nullptr) {
          assert(static_cast<node_t*>(point->right->parent) == point);
          assert(compare(point->value(), static_cast<node_t*>(point->right)->value()));
          check_invariant(static_cast<node_t*>(point->right));
        }

        assert(-1 <= point->get_balance());
        assert(point->get_balance() <= 1);
      #endif
    }

    node_base_t fake;
    [[no_unique_address]] Compare compare;
  };
}
