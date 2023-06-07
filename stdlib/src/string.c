unsigned long strlen(char *str) {
    char *start = str;
    while (*str != '\0') str += 1;
    return str - start;
}

char strcmp(char *s1, char *s2) {
    while (*s1 != '\0' && (*s1 == *s2)) {
        s1 += 1;
        s2 += 1;
    }
    return *s1 - *s2;
}
