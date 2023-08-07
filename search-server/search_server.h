#pragma once
#include <string>
#include <string_view>
#include <functional>
#include <vector>
#include <set>
#include <map>
#include <tuple>
#include <algorithm>
#include <numeric>
#include <execution>
#include <cmath>
#include <stdexcept>
#include <type_traits>
#include "document.h"
#include "string_processing.h"
#include "concurrent_map.h"

const int MAX_RESULT_DOCUMENT_COUNT = 5;

class SearchServer {
public:
    
    std::set<int>::const_iterator begin() const;
    
    std::set<int>::const_iterator end() const;
    
    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;
    
    void RemoveDocument(int document_id);
    
    template <typename ExecutionPolicy>
    void RemoveDocument(ExecutionPolicy policy, int document_id);
    
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);

    SearchServer(const std::string_view stop_words_text);

    SearchServer(const std::string stop_words_text);
    
    void AddDocument(int document_id, const std::string_view document, DocumentStatus status, const std::vector<int>& ratings);

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentPredicate document_predicate) const ;
    
    template <typename DocumentPredicate, typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy policy, const std::string_view raw_query, DocumentPredicate document_predicate) const ;

    std::vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentStatus status) const ;
    
    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy policy, const std::string_view raw_query, DocumentStatus status) const ;
    
    std::vector<Document> FindTopDocuments(const std::string_view raw_query) const ;
    
    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy policy, const std::string_view raw_query) const ;

    int GetDocumentCount() const ;

    using MatchWords = std::vector<std::string_view>;

    std::tuple<MatchWords, DocumentStatus> MatchDocument(const std::string_view raw_query, int document_id) const ;
    
    std::tuple<MatchWords, DocumentStatus> MatchDocument(std::execution::sequenced_policy policy, const std::string_view raw_query, int document_id) const ;
    
    std::tuple<MatchWords, DocumentStatus> MatchDocument(std::execution::parallel_policy policy, const std::string_view raw_query, int document_id) const ;

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    
    const std::set<std::string> stop_words_;
    
    std::map<std::string, std::map<int, double>, std::less<>> word_to_document_freqs_;
    
    std::map<int, std::map<std::string_view, double>> id_to_word_freqs_;
    
    std::map<int, DocumentData> documents_;
    
    std::set<int> document_ids_;

    static bool IsValidWord(const std::string_view word);
    
    bool IsStopWord(const std::string_view word) const ;
    
    std::vector<std::string_view> SplitIntoWordsNoStop(const std::string_view text) const ;

    static int ComputeAverageRating(const std::vector<int>& ratings) ;

    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(const std::string_view text) const ;

    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };

    Query ParseQuery(const std::string_view text, bool need_sort) const ;

    // Existence required
    double ComputeWordInverseDocumentFreq(const std::string_view word) const ;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(Query& query, DocumentPredicate document_predicate) const ;

    template <typename DocumentPredicate, typename ExecutionPolicy>
    std::vector<Document> FindAllDocuments(ExecutionPolicy policy, Query& query, DocumentPredicate document_predicate) const ;
};

template <typename ExecutionPolicy>
    void SearchServer::RemoveDocument(ExecutionPolicy policy, int document_id){
        std::vector<std::string_view> words;
        words.reserve(id_to_word_freqs_.size());
        for_each(policy, id_to_word_freqs_[document_id].begin(), id_to_word_freqs_[document_id].end(), [&words](const auto& elem){
            words.push_back(elem.first);
        });
        for_each(policy, words.begin(), words.end(), [this, &document_id](const std::string_view word){
            word_to_document_freqs_[(std::string)word].erase(document_id);
        });
        id_to_word_freqs_.erase(document_id);
        documents_.erase(document_id);
        auto iter = find(policy, document_ids_.begin(), document_ids_.end(), document_id);
        document_ids_.erase(iter);
    }

template <typename StringContainer>
    SearchServer::SearchServer(const StringContainer& stop_words)
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words))  // Extract non-empty stop words
    {
        if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
            throw std::invalid_argument("Some of stop words are invalid");
        }
    }

template <typename DocumentPredicate, typename ExecutionPolicy>
    std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy policy, const std::string_view raw_query, DocumentPredicate document_predicate) const {
        auto query = ParseQuery(raw_query, true);

        auto matched_documents = FindAllDocuments(policy, query, document_predicate);

        std::sort(policy, matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
            if (std::abs(lhs.relevance - rhs.relevance) < 1e-6) {
                return lhs.rating > rhs.rating;
            } else {
                return lhs.relevance > rhs.relevance;
            }
        });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }

        return matched_documents; 
    }

template <typename DocumentPredicate>
    std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentPredicate document_predicate) const {
        return FindTopDocuments(std::execution::seq, raw_query, document_predicate);
    }

template <typename ExecutionPolicy>
    std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy policy, const std::string_view raw_query, DocumentStatus status) const {
        return FindTopDocuments(policy, raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
    }

template <typename ExecutionPolicy>
    std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy policy, const std::string_view raw_query) const {
        return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
    }

template <typename DocumentPredicate>
    std::vector<Document> SearchServer::FindAllDocuments(SearchServer::Query& query, DocumentPredicate document_predicate) const {
        std::map<int, double> document_to_relevance;

        for (const std::string_view word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at((std::string)word)) {
                const auto& document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const std::string_view word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at((std::string)word)) {
                document_to_relevance.erase(document_id);
            }
        }

        std::vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back({document_id, relevance, documents_.at(document_id).rating});
        }
        return matched_documents;
    }

    template <typename DocumentPredicate, typename ExecutionPolicy>
    std::vector<Document> SearchServer::FindAllDocuments(ExecutionPolicy policy, SearchServer::Query& query, DocumentPredicate document_predicate) const {
        if (std::is_same_v<ExecutionPolicy, std::execution::parallel_policy>) {
            ConcurrentMap<int, double> document_to_relevance_par(100);

            for_each(policy, query.plus_words.begin(), query.plus_words.end(), [&](const auto& word){
                if (word_to_document_freqs_.count(word)) {
                    const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
                    const auto& document_freqs_ = word_to_document_freqs_.at((std::string)word);
                    for_each(policy, document_freqs_.begin(), document_freqs_.end(), [&](const auto& id_and_freq){
                        const auto& document_data = documents_.at(id_and_freq.first);
                        if (document_predicate(id_and_freq.first, document_data.status, document_data.rating)) {
                            document_to_relevance_par[id_and_freq.first].ref_to_value += id_and_freq.second * inverse_document_freq;
                        }
                    });
                }
            });

            for (const std::string_view word : query.minus_words) {
                if (word_to_document_freqs_.count(word) == 0) {
                    continue;
                }
                for (const auto [document_id, _] : word_to_document_freqs_.at((std::string)word)) {
                    document_to_relevance_par.Erase(document_id);
                }
            }

            auto document_to_relevance = document_to_relevance_par.BuildOrdinaryMap();
            
            std::vector<Document> matched_documents;
            for (const auto [document_id, relevance] : document_to_relevance) {
                matched_documents.push_back({document_id, relevance, documents_.at(document_id).rating});
            }
            return matched_documents;
        } else {
            return FindAllDocuments(query, document_predicate);
        }
    }