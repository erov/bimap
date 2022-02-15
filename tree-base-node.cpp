#include <algorithm>
#include "tree.h"

namespace bimap_impl {

size_t tree_base_node::get_height(tree_base_node* point) noexcept {
  return point != nullptr ? point->height : 0;
}

ptrdiff_t tree_base_node::get_balance() const noexcept {
  return get_height(right) - get_height(left);
}

void tree_base_node::upd_height() noexcept {
  height = std::max(get_height(left), get_height(right)) + 1;
}

void tree_base_node::upd_left() noexcept {
  if (left != nullptr) {
    left->parent = this;
  }
}

void tree_base_node::upd_kids() noexcept {
  upd_left();
  if (right != nullptr) {
    right->parent = this;
  }
}

tree_base_node* tree_base_node::rotate_left() noexcept {
  tree_base_node* p = right;
  right = p->left;
  upd_kids();
  p->left = this;
  p->upd_kids();
  upd_height();
  p->upd_height();
  return p;
}

tree_base_node* tree_base_node::rotate_right() noexcept {
  tree_base_node* v = left;
  left = v->right;
  upd_kids();
  v->right = this;
  v->upd_kids();
  upd_height();
  v->upd_height();
  return v;
}

tree_base_node* tree_base_node::balance() noexcept {
  upd_height();
  if (get_balance() == 2) {
    if (right->get_balance() < 0) {
      right = right->rotate_right();
      upd_kids();
    }
    return rotate_left();
  }
  if (get_balance() == -2) {
    if (left->get_balance() > 0) {
      left = left->rotate_left();
      upd_kids();
    }
    return rotate_right();
  }
  return this;
}

tree_base_node* tree_base_node::get_min() const noexcept {
  tree_base_node* result = const_cast<tree_base_node*>(this);
  while (result->left != nullptr) {
    result = result->left;
  }
  return result;
}

tree_base_node* tree_base_node::remove_min(tree_base_node* point) noexcept {
  if (point->left == nullptr) {
    return point->right;
  }
  point->left = remove_min(point->left);
  point->upd_kids();
  return point->balance();
}

tree_base_node* tree_base_node::get_max() const noexcept {
  tree_base_node* result = const_cast<tree_base_node*>(this);
  while (result->right != nullptr) {
    result = result->right;
  }
  return result;
}

bool tree_base_node::is_end() const noexcept {
  return parent == nullptr;
}

tree_base_node* tree_base_node::get_right() const noexcept {
  return right;
}

}
