// Test: deferred method calls on template structs inside template function bodies
// Phase 5 slice 20

template<typename T>
struct Box {
  T item;

  T get() const {
    return item;
  }

  T add(T x) {
    return item + x;
  }
};

// Template function calling method on template struct (dot operator)
template<typename T>
T extract(Box<T> b) {
  return b.get();
}

// Template function calling method on template struct pointer (arrow operator)
template<typename T>
T extract_ptr(Box<T>* bp) {
  return bp->get();
}

// Template function calling method with argument
template<typename T>
T add_to_box(Box<T> b, T x) {
  return b.add(x);
}

int main() {
  // Deferred dot method calls (explicit template args)
  Box<int> bi{42};
  if (extract<int>(bi) != 42) return 1;
  if (add_to_box<int>(bi, 8) != 50) return 2;

  // Deferred arrow method calls
  if (extract_ptr<int>(&bi) != 42) return 3;

  // Different type instantiation
  Box<long> bl{100};
  if (extract<long>(bl) != 100) return 4;
  if (add_to_box<long>(bl, 200L) != 300) return 5;
  if (extract_ptr<long>(&bl) != 100) return 6;

  return 0;
}
