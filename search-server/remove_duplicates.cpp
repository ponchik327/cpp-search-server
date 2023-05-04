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
    map<set<string>, vector<int>> words_for_doc;
    //здесь приведу мой рассчёт сложности, скажите где ошибка если не правильно посчитаю
    for (const int id : search_server) { //сложность n
        set<string> words;
        for(const auto& [word, freq] : search_server.GetWordFrequencies(id)) { //второй for в котором есть функция слоностью log(n) и в худшем случае перебор по w слов
            words.insert(word); //здесь будет амортизированная константная сложность, так как в начале сложности будут log(1), log(2), log(3), ... и только последние случаи будут log(w-2), log(w-1), log(w) 
        }
        words_for_doc[words].push_back(id); //вставка в вектор амортизированная константная сложность, только уже только уже в худших случаях будет n-2, n-1, n
        //и доступ к элементу map или создание нового это тоже амортизированная константная сложность, так как в начале будет log(1), log(2), log(3), ... и только последние случаи будут log(n-2), log(n-1), log(n) 
    }//итоговая будет что-то типа n*(w*log(n)) 

    //я не уверен, что прошёл по сложности, но мне кажеться что она здесь не n^2, а k*(n-k) k - любое чило из диапазона [1..n)
    //если у нас в певром for попало n докуметов, то во втором for будет все векторы равны 1, то есть сложность n*1
    //если в первом for попало k = [1..n-1], то в худшем случае второй for сделает n-k операций 
    for(const auto& [words, ids] : words_for_doc) {
        if (ids.size() != 1) {
            for (auto it = ids.begin() + 1; it != ids.end(); ++it) {
                cout << "Found duplicate document id "s << *it << endl;
                search_server.RemoveDocument(*it);
            }
        }
    }
}