unsigned long strlen(char *str) {
    char *start = str;
    while (*str != '\0') str += 1;
    return str - start;
}

extern char strcmp(char *s1, char *s2);
