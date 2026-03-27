// Parse-only: parenthesized pointer/member-pointer declarators should keep the
// direct-name path and the nested recursive path stable while the inner branch
// is extracted into focused helpers.
// RUN: %c4cll --parse-only %s

struct Box {
    int read(int value) const { return value; }
};

int (Box::*choose_reader(int which))(int) const;
int (Box::*(*choose_reader_factory(int which))(long))(int) const;

int main() {
    int (Box::*reader)(int) const = choose_reader(0);
    int (Box::*(*factory)(long))(int) const = choose_reader_factory(1);
    return reader == nullptr || factory == nullptr;
}
