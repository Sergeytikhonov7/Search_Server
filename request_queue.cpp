#include "request_queue.h"

constexpr int TRANSITION_TO_THE_NEXT_DAY = 2;

RequestQueue::RequestQueue(const SearchServer& search_server) : server_(search_server) {}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
    return AddFindRequest(raw_query, [status]([[maybe_unused]]int document_id, DocumentStatus document_status,
                                              [[maybe_unused]]int rating) {
        return document_status == status;
    });
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
    return AddFindRequest(raw_query, DocumentStatus::ACTUAL);
}

int RequestQueue::GetNoResultRequests() const {
    return empty_request;
}

void RequestQueue::QueryDeque(const std::vector<Document>& result_query) {
    CleanDeque();
    QueryResult queryResult(result_query, (sec_query));
    requests_.push_back(queryResult);
    if (result_query.empty()) {
        empty_request++;
    }
    sec_query++;
}

void RequestQueue::CleanDeque() {
    if (!requests_.empty()) {
        int d = requests_.back().sec_of_query - requests_.begin()->sec_of_query;
        if (d > sec_in_day_ - TRANSITION_TO_THE_NEXT_DAY) {
            for (auto it = requests_.begin(); it != requests_.end(); ++it) {
                if ((requests_.back().sec_of_query - requests_.front().sec_of_query) >
                    sec_in_day_ - TRANSITION_TO_THE_NEXT_DAY) {
                    requests_.pop_front();
                    if (it->result.empty()) {
                        --empty_request;
                    }
                }
            }
        }
    }
}