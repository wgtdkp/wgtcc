struct S {
    int a;
};
struct S bar();
void foo() {
    &bar().a;
}
int main() {
    struct S {
        unsigned a: 3;
        signed b: 3;
        int c: 3;
    };
    return 0;
}
