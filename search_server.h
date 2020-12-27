#pragma once

#include <string>
#include <map>
#include <cmath>
#include <algorithm>
#include <execution>
#include <atomic>

#include "document.h"
#include "string_processing.h"

constexpr int MAX_RESULT_DOCUMENT_COUNT = 5;
constexpr double EPSILON = 1e-6;

class SearchServer {
public:
    template<typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
            : stop_words_(MakeUniqueNonEmptyStrings<StringContainer>(stop_words))  // Extract non-empty stop words
    {
        if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
            throw std::invalid_argument("Some of stop words are invalid");
        }
    }

    explicit SearchServer(const std::string& stop_words_text);

    explicit SearchServer(std::string_view stop_words_text);

    void
    AddDocument(int document_id, std::string_view document, DocumentStatus status, const std::vector<int>& ratings);

    template<typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentPredicate document_predicate) const {
        return FindTopDocuments(std::execution::seq, raw_query, document_predicate);
    }

    template<typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::execution::sequenced_policy&, std::string_view raw_query,
                                           DocumentPredicate document_predicate) const {
        const auto query = ParseQuery(raw_query);
        std::vector<Document> matched_documents;
        matched_documents = FindAllDocuments(std::execution::seq, query, document_predicate);

        sort(std::execution::seq, matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
            if (std::abs(lhs.relevance - rhs.relevance) < EPSILON) {
                return lhs.rating > rhs.rating;
            } else {
                return lhs.relevance > rhs.relevance;
            }
        });
        if (int(matched_documents.size()) > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }

        return matched_documents;
    }

    template<typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::execution::parallel_policy&, std::string_view raw_query,
                                           DocumentPredicate document_predicate) const {
        const auto query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(std::execution::par, query, document_predicate);
        sort(std::execution::par, matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
            if (std::abs(lhs.relevance - rhs.relevance) < EPSILON) {
                return lhs.rating > rhs.rating;
            } else {
                return lhs.relevance > rhs.relevance;
            }
        });
        if (int(matched_documents.size()) > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentStatus status) const;

    std::vector<Document>
    FindTopDocuments(const std::execution::sequenced_policy&, std::string_view raw_query, DocumentStatus status) const;

    std::vector<Document>
    FindTopDocuments(const std::execution::parallel_policy&, std::string_view raw_query, DocumentStatus status) const;

    std::vector<Document> FindTopDocuments(std::string_view raw_query) const;

    std::vector<Document> FindTopDocuments(const std::execution::sequenced_policy&, std::string_view raw_query) const;

    std::vector<Document> FindTopDocuments(const std::execution::parallel_policy&, std::string_view raw_query) const;

    int GetDocumentCount() const;

    int GetDocumentId(int index) const;

    std::tuple<std::vector<std::string_view>, DocumentStatus>
    MatchDocument(std::string_view raw_query, int document_id) const;

    template<typename ExecutionPolicy>
    std::tuple<std::vector<std::string_view>, DocumentStatus>
    MatchDocument(ExecutionPolicy&& policy, std::string_view raw_query, int document_id) const {
        static auto query = ParseQuery(raw_query);
        std::vector<std::string_view> matched_words;
        std::for_each(policy, query.plus_words.begin(), query.plus_words.end(),
                      [document_id, &matched_words, this](const std::string& word) {
                          if (id_to_words_freqs.at(document_id).count(word)) {
                              matched_words.push_back(word);
                          }
                      });
        for (const std::string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
        return {matched_words, documents_.at(document_id).status};
    }


    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;

    void RemoveDocument(int document_id);

    template<typename ExecutionPolicy>
    void RemoveDocument(ExecutionPolicy&& policy, int document_id) {
        if (id_to_words_freqs.count(document_id) == 1) {
            auto find_id = find(policy, document_ids_.begin(), document_ids_.end(), document_id);
            document_ids_.erase(find_id);
            documents_.erase(document_id);
            std::for_each(policy, word_to_document_freqs_.begin(), word_to_document_freqs_.end(), [document_id]
                    (auto& doc) {
                doc.second.erase(document_id);
            });
            id_to_words_freqs.erase(document_id);
        }
    }

    auto begin() {
        return document_ids_.begin();
    }

    auto end() {
        return document_ids_.end();
    }

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    struct Query {
        std::set<std::string, std::less<>> plus_words;
        std::set<std::string, std::less<>> minus_words;
    };

    const std::set<std::string> stop_words_;
    std::map<std::string, std::map<int, double>> word_to_document_freqs_;
    std::map<int, DocumentData> documents_;
    std::vector<int> document_ids_;
    std::map<int, std::map<std::string, double>> id_to_words_freqs;


    bool IsStopWord(std::string_view word) const;

    static bool IsValidWord(std::string_view word);

    std::vector<std::string_view> SplitIntoWordsNoStop(std::string_view text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    QueryWord ParseQueryWord(std::string_view text) const;

    Query ParseQuery(std::string_view text) const;

    // Existence required
    double ComputeWordInverseDocumentFreq(const std::string& word) const;

    template<typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const {
        return FindAllDocuments(std::execution::seq, query, document_predicate);
    }

    template<typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const std::execution::sequenced_policy&, const Query& query,
                                           DocumentPredicate document_predicate) const {
        std::map<int, double> document_to_relevance;
        for (const std::string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto[document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const auto& document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }
        for (const std::string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto[document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }
        std::vector<Document> matched_documents;
        for (const auto[document_id, relevance] : document_to_relevance) {
            matched_documents.emplace_back(Document(document_id, relevance, documents_.at(document_id).rating));
        }
        return matched_documents;
    }

    template<typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const std::execution::parallel_policy&, const Query& query,
                                           DocumentPredicate document_predicate) const {
        std::map<int, double> document_to_relevance;
        std::for_each(std::execution::par, query.plus_words.begin(), query.plus_words.end(), // да понимаю, что ни самое удачное решение в 3 этажа,
                      // но самое понтятное,прошедшее тест,  на основе последовательного решения,
                      // если разложить по порядку
                      [this, &document_to_relevance, &document_predicate](const std::string& word) {
                          if (word_to_document_freqs_.count(word) != 0) {
                              const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
                              std::for_each(std::execution::par, word_to_document_freqs_.at(word).begin(),
                                            word_to_document_freqs_.at(word).end(),
                                            [this, &document_to_relevance, &document_predicate, &inverse_document_freq]
                                                    (const std::pair<int, double>& docs) {
                                                const auto& document_data = documents_.at(docs.first);
                                                if (document_predicate(docs.first, document_data.status,
                                                                       document_data.rating)) {
                                                    document_to_relevance[docs.first] +=
                                                            docs.second * inverse_document_freq;
                                                }
                                            });
                          };
                      });

        for (const std::string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto[document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }
        std::vector<Document> matched_documents;
        matched_documents.resize(document_to_relevance.size());
        std::atomic_size_t size = 0;
        std::for_each(std::execution::par, document_to_relevance.begin(), document_to_relevance.end(),
                      [this, &matched_documents, &size](const std::pair<int, double>& document) {
                          matched_documents[size++] = Document(document.first, document.second,
                                                               documents_.at(document.first).rating);;
                      });
        return matched_documents;
    }
};