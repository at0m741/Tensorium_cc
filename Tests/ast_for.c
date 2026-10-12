int sum(int n) {
    int result = 0;

    for (int i = 0; i < n; i++) {
        if (i == 3)
            continue;
        if (i == 10)
            break;
        result = result + i;
    }

    return result;
}
