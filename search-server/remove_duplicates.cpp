#include "remove_duplicates.h"

using namespace std;

#include <sstream>
template<typename T>
ostream& operator<<(ostream& out, const set<T> s) {
    cout << "{"s;
    for (auto e : s) {
        out << e << ", "s; 
    }
    cout << "}"s << endl;
    return out;
}

void RemoveDuplicates(SearchServer& search_server) {
    map<int , set<string>> doc_for_words;
    for (const int id : search_server) {
        for (auto [word, freq] : search_server.GetWordFrequencies(id)) {
            doc_for_words[id].insert(word);
        }
    }
    for(auto it = search_server.begin(); it < search_server.end(); ++it) {
        for (auto it1 = it + 1; it1 < search_server.end(); ++it1) {
            if (doc_for_words.at(*it) == doc_for_words.at(*it1)) {
                cout << "Found duplicate document id "s << *it1 << endl;
                search_server.RemoveDocument(*it1);
                --it1;
            }
        }
    }
}