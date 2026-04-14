// Parse-only regression: conversion operators targeting template-id types
// should accept the type spelling before trailing ref-qualifiers.
// RUN: %c4cll --parse-only %s

template <class A, class B>
struct pair_like {};

struct holder {
  operator pair_like<int, int>() && { return {}; }
};

int main() {
  return 0;
}
