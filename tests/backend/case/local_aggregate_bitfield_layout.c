struct NodeCommon {
  void *chain;
  void *type;
  unsigned code : 8;
};

struct TreeNode {
  struct NodeCommon common;
};

int read_code(struct TreeNode *node) {
  return node->common.code;
}

int main(void) {
  struct TreeNode node = {0};
  node.common.code = 7;
  return read_code(&node);
}
