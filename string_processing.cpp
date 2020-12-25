#include "string_processing.h"

std::vector<std::string_view> SplitIntoWords(std::string_view text) {
    std::string_view str = text;
    std::vector<std::string_view> words;
    while (true) {
        size_t space = str.find(' ');
        words.push_back(str.substr(0, space));
        if (space == str.npos) {
            break;
        } else {
            str.remove_prefix(space + 1);
        }
    }
    return words;
}