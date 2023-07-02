#include "string_processing.h"

using namespace std;

vector<string_view> SplitIntoWords(const string_view text) {
    vector<string_view> result;
    int64_t pos = text.find_first_not_of(' ');
    const int64_t pos_end = text.npos;
    while (pos != pos_end) {
        int64_t space = text.find(' ', pos);
        result.push_back(space == pos_end ? text.substr(pos) : text.substr(pos, space - pos));
        pos = text.find_first_not_of(' ', space);
    }
    return result;
}