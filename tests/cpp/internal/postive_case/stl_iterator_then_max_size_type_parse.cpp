// Reduced from libstdc++ ranges bring-up: parsing <bits/max_size_type.h> only
// fails once the earlier <bits/iterator_concepts.h> path has been seen.
// RUN: %c4cll --parse-only %s

#include <bits/iterator_concepts.h>
#include <bits/max_size_type.h>

int main() {
  return 0;
}
