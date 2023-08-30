#include "server_html_utils.h"

#include <string.h>
#include <stdio.h>

namespace Wetzel {

void convert_html_text_to_ascii(char* string) {
    convert_html_symbol(string, "\%2C", ',');
    convert_html_symbol(string, "\%3B", ';');
    convert_html_symbol(string, "\%20", ';');
    convert_html_symbol(string, "\%7B", '{');
    convert_html_symbol(string, "\%22", '\"');
    convert_html_symbol(string, "\%3A", ':');
    convert_html_symbol(string, "\%7D", '}');
    convert_html_spaces(string);
}

void convert_html_symbol(char* string, const char* code, char caracter) {
    char* code_pos = strstr(string, code);
    if (code_pos == NULL) {
        return;
    }
    int pos = code_pos - string;
    string[pos] = caracter;
    for (int i = pos + 3; i < strlen(string) + 1; i++) {
        string[i - 2] = string[i];
    }
    convert_html_symbol(string, code, caracter);
}

void convert_html_spaces(char* string) {
    const int char_to_replace = '+';
    char* current_pos = strchr(string, char_to_replace);
    while (current_pos) {
        *current_pos = ' ';
        current_pos = strchr(current_pos, char_to_replace);
    }
    return;
}
}  // namespace Wetzel