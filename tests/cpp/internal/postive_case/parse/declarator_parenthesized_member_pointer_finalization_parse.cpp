// Parse-only: parenthesized member-pointer declarators should keep the shared
// post-')' function-suffix and array-suffix finalization stable for both the
// direct-name and nested-recursive paths.
// RUN: %c4cll --parse-only %s

struct Box {
    int cells[2];
    int lookup(long index) { return cells[index != 0]; }
};

int (Box::*pick_member(int which))[2];
int (Box::*(*pick_member_factory(int which))(long))[2];

int main() {
    int (Box::*direct)[2] = pick_member(0);
    int (Box::*(*nested)(long))[2] = pick_member_factory(1);
    return direct == nullptr || nested == nullptr;
}
