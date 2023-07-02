#include "search_server.h"

using namespace std;

void SearchServer::RemoveDocument(int document_id) {
    RemoveDocument(execution::seq, document_id);
}

SearchServer::SearchServer(const string_view stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
    {
    }

SearchServer::SearchServer(const string stop_words_text)
        : SearchServer(SplitIntoWords((string_view)stop_words_text))
    {
    }

set<int>::const_iterator SearchServer::begin() const{
    return document_ids_.begin();
}

set<int>::const_iterator SearchServer::end() const{
    return document_ids_.end();
}

const map<string_view, double>& SearchServer::GetWordFrequencies(int document_id) const{
    static map<std::string_view, double> empty;
    if (id_to_word_freqs_.count(document_id)) {
        return id_to_word_freqs_.at(document_id);   
    } else {
        return empty;
    }
}

void SearchServer::AddDocument(int document_id, const string_view document, DocumentStatus status, const vector<int>& ratings) {
        if ((document_id < 0) || (documents_.count(document_id) > 0)) {
            throw invalid_argument("Invalid document_id"s);
        }
        const auto words = SplitIntoWordsNoStop(document);

        const double inv_word_count = 1.0 / words.size();
        for (const string_view word : words) {
            word_to_document_freqs_[(string)word][document_id] += inv_word_count;
            string_view str_in_map = word_to_document_freqs_.find((string)word)->first;
            id_to_word_freqs_[document_id][str_in_map] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
        document_ids_.insert(document_id);
    }

vector<Document> SearchServer::FindTopDocuments(const string_view raw_query, DocumentStatus status) const {
        return FindTopDocuments(
            raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
                return document_status == status;
            });
    }

vector<Document> SearchServer::FindTopDocuments(const string_view raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

int SearchServer::GetDocumentCount() const {
        return documents_.size();
    }

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(const string_view raw_query, int document_id) const {
    return MatchDocument(execution::seq, raw_query, document_id);
}

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(execution::sequenced_policy policy, const string_view raw_query, int document_id) const {
        if (!documents_.count(document_id)) {
            throw out_of_range("Defunct document_id"s);
        }
    
        auto query = ParseQuery(raw_query);
    
        sort(policy, query.plus_words.begin(), query.plus_words.end());
        query.plus_words.erase(unique(policy, query.plus_words.begin(), query.plus_words.end()), query.plus_words.end());
        
        sort(policy, query.minus_words.begin(), query.minus_words.end());
        query.minus_words.erase(unique(policy, query.minus_words.begin(), query.minus_words.end()), query.minus_words.end());
        
        bool found_minus_word = false; 
        for (const string_view word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at((string)word).count(document_id)) {
                found_minus_word = true;
                break;
            }
        }
    
        vector<string_view> matched_words;
        if (!found_minus_word) {
            for (const string_view word : query.plus_words) {
                if (word_to_document_freqs_.count(word) == 0) {
                    continue;
                }
                if (word_to_document_freqs_.at((string)word).count(document_id)) {
                    matched_words.push_back(word);
                }
            }
        }
        
        return {matched_words, documents_.at(document_id).status};
    }

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(execution::parallel_policy policy, const string_view raw_query, int document_id) const {
    if (!documents_.count(document_id)) {
        throw out_of_range("Defunct document_id"s);
    }
    
    auto query = ParseQuery(raw_query);
    
    bool found_minus_word = any_of(policy, query.minus_words.begin(), query.minus_words.end(), [this, &document_id](const string_view word){
        if (word_to_document_freqs_.count(word)) {
            return (bool)word_to_document_freqs_.at((string)word).count(document_id);
        }
        return false;
    });
    
    vector<string_view> matched_words;
    
    if (!found_minus_word) {
        matched_words.resize(query.plus_words.size());
        auto new_end = copy_if (policy, query.plus_words.begin(), query.plus_words.end(), matched_words.begin(), [this, &document_id](const string_view word){
            if (word_to_document_freqs_.count(word)) {
                return (bool)word_to_document_freqs_.at((string)word).count(document_id);   
            }
            return false;
        });
        matched_words.resize(new_end - matched_words.begin());
        sort(policy, matched_words.begin(), matched_words.end());
        matched_words.erase(unique(policy, matched_words.begin(), matched_words.end()), matched_words.end());
    }
    
    return {matched_words, documents_.at(document_id).status};
}

bool SearchServer::IsStopWord(const string_view word) const {
        return stop_words_.count((string)word) > 0;
    }

bool SearchServer::IsValidWord(const string_view word) {
        // A valid word must not contain special characters
        return none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
        });
    }

vector<string_view> SearchServer::SplitIntoWordsNoStop(const string_view text) const {
        vector<string_view> words;
        for (const string_view word : SplitIntoWords(text)) {
            if (!IsValidWord(word)) {
                throw invalid_argument("Word "s + (string)word + " is invalid"s);
            }
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

int SearchServer::ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = 0;
        for (const int rating : ratings) {
            rating_sum += rating;
        }
        return rating_sum / static_cast<int>(ratings.size());
    }

SearchServer::QueryWord SearchServer::ParseQueryWord(const string_view text) const {
        if (all_of(text.begin(), text.end(), [](char c){ return c == ' '; })) {
            throw invalid_argument("Query word is empty"s);
        }
        string_view word = text;
        bool is_minus = false;
        if (word[0] == '-') {
            is_minus = true;
            word = word.substr(1);
        }
        if (all_of(word.begin(), word.end(), [](char c){ return c == ' '; }) || word[0] == '-' || !IsValidWord(word)) {
            throw invalid_argument("Query word "s + (string)text + " is invalid");
        }

        return {word, is_minus, IsStopWord(word)};
    }

SearchServer::Query SearchServer::ParseQuery(const string_view text) const {
        SearchServer::Query result;
        for (const string_view word : SplitIntoWords(text)) {
            const auto query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    result.minus_words.push_back(query_word.data);
                } else {
                    result.plus_words.push_back(query_word.data);
                }
            }
        }
        return result;
    }

double SearchServer::ComputeWordInverseDocumentFreq(const string_view word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at((string)word).size());
    }