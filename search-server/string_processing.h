#pragma once
#include <vector>
#include <string>
#include <string_view>
#include <set>
#include <algorithm>

std::vector<std::string_view> SplitIntoWords(const std::string_view text) ;

template <typename StringContainer>
std::set<std::string> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    std::set<std::string> non_empty_strings;
    for (const std::string_view str_view : strings) {
        if (!std::all_of(str_view.begin(), str_view.end(), [](char c){ return c == ' '; })) {
            non_empty_strings.insert((std::string)str_view);
        }
    }
    return non_empty_strings;
}