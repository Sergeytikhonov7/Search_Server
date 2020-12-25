#include "process_queries.h"



std::vector<std::vector<Document>> ProcessQueries(const SearchServer& search_server, const std::vector<std::string>& queries) {
    std::vector<std::vector<Document>> result(queries.size());
    std::transform(std::execution::par, queries.cbegin(), queries.cend(),result.begin(),
                   [&search_server](std::string query) { return search_server.FindTopDocuments(query);});
    return result;
}

std::vector<Document> ProcessQueriesJoined(const SearchServer& search_server, const std::vector<std::string>& queries) {
    auto docs = ProcessQueries(search_server,queries);
    int size = std::transform_reduce(std::execution::par, docs.cbegin(), docs.cend(),0, std::plus<>(),
          [](const std::vector<Document>& doc){ return doc.size();});
    std::vector<Document> result (size);
    int id = 0;
    for (auto& v: docs) {
        for (auto& doc: v) {
            result[id++] = std::move(doc);
        }
    }
    /* vector<Document> documents;
    for (const auto& local_documents : ProcessQueries(search_server, queries)) {
        documents.insert(documents.end(), local_documents.begin(), local_documents.end());
    }
    return documents;*/
    return result;
}
