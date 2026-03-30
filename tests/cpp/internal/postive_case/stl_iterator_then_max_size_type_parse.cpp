// Reduced from libstdc++ ranges bring-up: parsing <bits/max_size_type.h> only
// fails once the earlier <bits/stl_iterator.h> concepts path has been seen.
// RUN: %c4cll --parse-only %s

#include <bits/stl_iterator.h>
#include <bits/max_size_type.h>

int main() {
  return 0;
}
