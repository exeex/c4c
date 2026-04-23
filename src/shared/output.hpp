#pragma once

#include <iostream>
#include <ostream>
#include <streambuf>

namespace c4c::output {

class NullBuffer : public std::streambuf {
 protected:
  int overflow(int ch) override { return traits_type::not_eof(ch); }
  int sync() override { return 0; }
};

class Sink {
 public:
  explicit Sink(std::ostream* stream) : stream_(stream) {}

  template <typename T>
  Sink& operator<<(const T& value) {
    (*stream_) << value;
    return *this;
  }

  Sink& operator<<(std::ostream& (*manip)(std::ostream&)) {
    (*stream_) << manip;
    return *this;
  }

 private:
  std::ostream* stream_;
};

inline Sink normal_sink() {
  return Sink(&std::cout);
}

inline Sink error_sink() {
  return Sink(&std::cerr);
}

inline Sink null_sink() {
  static NullBuffer buffer;
  static std::ostream stream(&buffer);
  return Sink(&stream);
}

}  // namespace c4c::output
