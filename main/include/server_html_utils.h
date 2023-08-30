#ifndef WETZEL_SERVER_HTTP_UTILITIES_H_
#define WETZEL_SERVER_HTTP_UTILITIES_H_

namespace Wetzel {

void convert_html_text_to_ascii(char* string);
void convert_html_symbol(char* string, const char* code, char caracter);
void convert_html_spaces(char* string);

};  // namespace Wetzel
#endif