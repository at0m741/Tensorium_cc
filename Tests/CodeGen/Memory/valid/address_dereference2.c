int main(void) {
    int x = 10;
    int *p = &x;
    int **pp = &p;

    **pp = 42;

    if (x != 42)
        return 1;

    *p += 8;

    if (**pp != 50)
        return 2;

    int y = 100;
    *pp = &y;

    **pp = 200;

    if (y != 200)
        return 3;

    if (x != 50)
        return 4;

    return 0;
}
