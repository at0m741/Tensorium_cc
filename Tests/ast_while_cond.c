int f(int n) {
    int x = 0;

    while (n > 0) {
        if (n > 5)
            x = x + 2;
        else
            x = x + 1;

        n = n - 1;
    }

    return x;
}
