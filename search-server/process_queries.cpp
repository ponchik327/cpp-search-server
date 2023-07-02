#include "process_queries.h"

using namespace std;

vector<vector<Document>> ProcessQueries(const SearchServer& search_server, const vector<string>& queries) {
    vector<vector<Document>> documents_lists(queries.size());
    transform(
    execution::par,
    queries.begin(), queries.end(),
    documents_lists.begin(),
    [&search_server](const string& query){
        return search_server.FindTopDocuments(query);
    });
    return documents_lists;
}

vector<Document> ProcessQueriesJoined(const SearchServer& search_server, const vector<string>& queries) {
    vector<Document> documents_lists;
    for (auto vec_doc : ProcessQueries(search_server, queries)) {
        documents_lists.insert(documents_lists.end(), vec_doc.begin(), vec_doc.end());
    }
    return documents_lists;
}