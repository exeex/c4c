// Parse-only: declarator pointer/member-pointer/ref qualifier staging should
// stay stable across grouped and parenthesized declarators.
// RUN: %c4cll --parse-only %s

struct Widget {
    int value;
    int method() const { return value; }
};

int target(int value) {
    return value + 1;
}

int accept(int (*const& cb)(int), int (Widget::*const pm)() const) {
    Widget widget{4};
    return cb((widget.*pm)());
}

int main() {
    int (*const grouped_fp)(int) = target;
    int (*const& grouped_fp_ref)(int) = grouped_fp;
    int (Widget::*const member_method)() const = &Widget::method;
    return accept(grouped_fp_ref, member_method);
}
