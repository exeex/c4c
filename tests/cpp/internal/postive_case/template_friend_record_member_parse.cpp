template<typename T>
struct Box {
  template<typename U>
  friend class Box;

  Box operator++(int) {
    Box tmp = *this;
    return tmp;
  }
};
