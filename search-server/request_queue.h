#pragma once
#include "document.h"
#include "search_server.h"
#include<string>
#include<vector>
#include<deque>

class RequestQueue {
public:
    explicit RequestQueue(SearchServer& search_server);
    // сделаем "обёртки" для всех методов поиска, чтобы сохранять результаты для нашей статистики
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) ;
    
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status) ;
    
    std::vector<Document> AddFindRequest(const std::string& raw_query) ;
    
    int GetNoResultRequests() const ;
    
private:
    struct QueryResult {
        // определите, что должно быть в структуре
        std::vector<Document> result_search;
        bool answer_is_empty; 
    };
    
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    int mins_passed_ = 0;
    int number_empty_requests = 0;
    SearchServer& server_;

    void collecting_statistics(const std::vector<Document>& answer) ;
    
};

template <typename DocumentPredicate>
    std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
        // напишите реализацию
        auto answer = server_.FindTopDocuments(raw_query, document_predicate);
        collecting_statistics(answer);
        return answer;
    }