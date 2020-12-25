#pragma once

#include <deque>

#include "search_server.h"

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);

    // сделаем "обёртки" для всех методов поиска, чтобы сохранять результаты для нашей статистики
    template<typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
        std::vector<Document> result_query = server_.FindTopDocuments(raw_query, document_predicate);
        QueryDeque(result_query);
        return result_query;
    }

    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);

    std::vector<Document> AddFindRequest(const std::string& raw_query);

    int GetNoResultRequests() const;

private:
    struct QueryResult {
        QueryResult(const std::vector<Document> document, const int sec) {
            result = document;
            sec_of_query = sec;
        }

        std::vector<Document> result;
        int sec_of_query = 0;
    };

    std::deque<QueryResult> requests_;
    const static int sec_in_day_ = 1440;
    int sec_query = 0;
    int empty_request = 0;
    const SearchServer& server_;

    void QueryDeque(const std::vector<Document>& result_query);

    void CleanDeque();
};