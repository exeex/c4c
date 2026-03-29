struct BadRecordStaticAssert {
    static_assert(false, "record member static_assert should fail");
};

int main() {
    return 0;
}
