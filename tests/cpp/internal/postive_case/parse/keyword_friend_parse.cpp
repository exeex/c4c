// Parse-only regression: reserve `friend` as a keyword token without breaking
// the existing record-member prelude path for friend declarations and inline
// friend definitions.
// RUN: %c4cll --parse-only %s

struct FriendKeyword;

struct keyword_friend_box {
    int value;

    friend struct FriendKeyword;
    friend bool operator==(const keyword_friend_box& a,
                           const keyword_friend_box& b) noexcept {
        return a.value == b.value;
    }
};

struct FriendKeyword {
    static int read(const keyword_friend_box& box) { return box.value; }
};

int main() {
    keyword_friend_box a{7};
    keyword_friend_box b{7};
    return (a == b) ? (FriendKeyword::read(a) - 7) : 1;
}
