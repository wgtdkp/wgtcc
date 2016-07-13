void foo(void)
{
    int b;
    int a = b;
    for (a = 0; a < 7; a++) {
        if (a == b) {
            continue;
        } else {
            break;
        }
    }
    //a = 10;

    while (b < 10 && b > 0) {

    }

    do {

    } while (!(~b));

    float c;
    switch (b) {
    case 1 + 4:
        break;
    case 2:
    //case b:
    case 3:
        break;
    default:
    //default:
        ;
    }

    //continue;
    //break;
    int* y;
    unsigned* z;
    int* x = y - z;

    float d = a ? b: c;
}
