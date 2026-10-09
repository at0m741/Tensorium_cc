int f(int n) {
    int result = 0;

    while (n > 0) {
        n = n - 1;

        if (n == 3)
            continue;

        if (n == 1)
            break;

        result = result + 1;
    }

    return result;
}
