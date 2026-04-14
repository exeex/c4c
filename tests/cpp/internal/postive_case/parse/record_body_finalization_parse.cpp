// Parse-only regression: record post-body finalization should keep trailing
// attributes, member typedef storage, and namespace-qualified self-type
// registration stable while `parse_struct_or_union()` lifts teardown into
// focused helpers.
// RUN: %c4cll --parse-only %s

namespace finalization {

struct BodyFinalization {
    using Alias = BodyFinalization;
    typedef int Count;

    Alias echo(Alias other) { return other; }
    Count first, second;
} __attribute__((aligned(32)));

struct PackedBody {
    using Self = PackedBody;

    Self identity(Self other) { return other; }
    int payload;
} __attribute__((packed));

}  // namespace finalization

int main() {
    return 0;
}
