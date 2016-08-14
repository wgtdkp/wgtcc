
enum EE {
    AA,
    BB,
    CC,
} ee;

enum TT {
    EE,
    FF,
    GG,
};

void test(void)
{
    enum TT tt = AA;
    enum BB bb;
    ee = CC;
}