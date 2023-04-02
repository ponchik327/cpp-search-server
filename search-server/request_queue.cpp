#include "request_queue.h"

using namespace std;

RequestQueue::RequestQueue(SearchServer& search_server) 
        : server_(search_server)
    {
        // напишите реализацию
    }

vector<Document> RequestQueue::AddFindRequest(const string& raw_query, DocumentStatus status) {
        // напишите реализацию
        auto answer = server_.FindTopDocuments(raw_query, status);
        collecting_statistics(answer);
        return answer;
    }

vector<Document> RequestQueue::AddFindRequest(const string& raw_query) {
        // напишите реализацию
        auto answer = server_.FindTopDocuments(raw_query);
        collecting_statistics(answer);
        return answer;
    }

int RequestQueue::GetNoResultRequests() const {
        // напишите реализацию
        return number_empty_requests;
    }

void RequestQueue::collecting_statistics(const vector<Document>& answer) {
        QueryResult request;
        request.result_search = answer;
        if (request.result_search.empty()) {
            request.answer_is_empty = true;
            ++number_empty_requests;
        } else {
            request.answer_is_empty = false;
        }
        requests_.push_front(request);

        mins_passed_++;
        if (mins_passed_ > min_in_day_) {
            if (requests_.back().answer_is_empty) {
                --number_empty_requests;
            }
            requests_.pop_back();
        }
    }