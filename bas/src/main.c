#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "lexer.h"
#include "parser.h"
#include "codegen.h"
#include "object.h"
#include "utils.h"

int main(int argc, char **argv) {
    Platform windows = { PLATFORM_WINDOWS };
    Arch x86_64 = { ARCH_X86_64 };

    // Parse args
    Platform *platform = &windows;
    Arch *arch = &x86_64;
    char *path = NULL;
    char *outputPath = NULL;
    bool dumpNode = false;
    for (int32_t i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-o")) {
            i++;
            outputPath = argv[i];
            continue;
        }

        if (!strcmp(argv[i], "-d")) {
            dumpNode = true;
            continue;
        }

        if (!strcmp(argv[i], "-os") || !strcmp(argv[i], "-platform")) {
            i++;
            if (!strcmp(argv[i], "windows")) {
                platform = &windows;
            }
            continue;
        }

        if (!strcmp(argv[i], "-arch")) {
            i++;
            if (!strcmp(argv[i], "x86_64")) {
                arch = &x86_64;
            }
            continue;
        }

        if (path == NULL) {
            path = argv[i];
        }
    }
    if (argc == 1 || path == NULL) {
        printf("Bassie Assembler\n");
        return EXIT_SUCCESS;
    }

    // Parse file
    char *text = file_read(path);
    if (text == NULL) return EXIT_FAILURE;

    List *lines = text_to_list(text);
    List *tokens = lexer(arch, path, lines);

    Node *node = parser(arch, path, lines, tokens);
    if (dumpNode) {
        printf("%s", node_to_string(node));
        return EXIT_SUCCESS;
    }

    // Write object
    Object *object = object_new(platform, arch, OBJECT_EXECUTABLE);

    uint8_t textBuffer[0x200];
    Section *textSection = section_new(SECTION_TEXT, textBuffer, 0);
    list_add(object->sections, textSection);

    uint8_t *end = textBuffer;
    node_write(object, textSection, &end, node);
    textSection->size = end - textBuffer;

    object_write(object, outputPath);
    return EXIT_SUCCESS;
}
