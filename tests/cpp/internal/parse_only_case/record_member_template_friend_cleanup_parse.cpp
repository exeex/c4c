// Parse-only regression: template-member prelude cleanup must not leak
// synthesized typedef visibility or member-template scope after a friend
// member path returns early.
// RUN: %c4cll --parse-only %s

template <typename T>
struct FriendTemplateCleanup {
    template <typename U = FriendTemplateCleanup>
    friend U* make_friend(U* value);

    using Self = FriendTemplateCleanup;
    Self* next;
    int kept;
};

template <typename T>
struct NestedFriendTemplateCleanup {
    template <typename U = NestedFriendTemplateCleanup>
    friend struct NestedFriend;

    using Self = NestedFriendTemplateCleanup;
    Self* tail;
    int kept;
};

int main() {
    return 0;
}
