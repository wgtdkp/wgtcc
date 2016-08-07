struct foo {
    int a;
    struct bar {
        int c;
    } b;
} f;

int main(void)
{
    int arr[10];
    int** p = &arr;
    int i;
    int a = (*p)[i];
    return 0;
}
