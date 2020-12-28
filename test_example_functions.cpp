#include "test_example_functions.h"

using namespace std::string_literals;


void PrintDocument(const Document& document) {
    std::cout << "{ "s
              << "document_id = "s << document.id << ", "s
              << "relevance = "s << document.relevance << ", "s
              << "rating = "s << document.rating << " }"s << std::endl;
}

void PrintMatchDocumentResult(int document_id,
                              const std::vector<std::string_view>& words,
                              DocumentStatus status) {
    std::cout << "{ "s
              << "document_id = "s << document_id << ", "s
              << "status = "s << static_cast<int>(status) << ", "s
              << "words ="s;
    for (const std::string_view word : words) {
        std::cout << ' ' << word;
    }
    std::cout << "}"s << std::endl;
}

void AddDocument(SearchServer& search_server,
                 int document_id,
                 const std::string& document,
                 DocumentStatus status,
                 const std::vector<int>& ratings) {
    try {
        search_server.AddDocument(document_id, document, status, ratings);
    } catch (const std::exception& e) {
        std::cout << "Ошибка добавления документа "s << document_id << ": "s
                  << e.what() << std::endl;
    }
}

void FindTopDocuments(const SearchServer& search_server,
                      const std::string& raw_query) {
    std::cout << "Результаты поиска по запросу: "s << raw_query << std::endl;
    try {
        for (const Document& document : search_server.FindTopDocuments(raw_query)) {
            PrintDocument(document);
        }
    } catch (const std::exception& e) {
        std::cout << "Ошибка поиска: "s << e.what() << std::endl;
    }
}

void MatchDocuments(const SearchServer& search_server, const std::string& query) {
    try {
        std::cout << "Матчинг документов по запросу: "s << query << std::endl;
        const int document_count = search_server.GetDocumentCount();
        for (int index = 0; index < document_count; ++index) {
            const int document_id = search_server.GetDocumentId(index);
            const auto[words, status] = search_server.MatchDocument(query, document_id);
            PrintMatchDocumentResult(document_id, words, status);
        }
    } catch (const std::exception& e) {
        std::cout << "Ошибка матчинга документов на запрос "s << query << ": "s
                  << e.what() << std::endl;
    }
}

void TestProcessQueries() {
    SearchServer search_server("and with"s);

    search_server.AddDocument(1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, {1, 2});
    search_server.AddDocument(2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2});
    search_server.AddDocument(3, "funny pet and not very nasty rat"s, DocumentStatus::ACTUAL, {1, 2});
    search_server.AddDocument(4, "pet with rat and rat and rat"s, DocumentStatus::ACTUAL, {1, 2});
    search_server.AddDocument(5, "nasty rat with curly hair"s, DocumentStatus::ACTUAL, {1, 2});


    const std::vector<std::string> queries = {
            "nasty rat -not"s,
            "not very funny nasty pet"s,
            "curly hair"s
    };
    int id = 0;
    for (const auto& documents : ProcessQueries(search_server, queries)) {
        std::cout << documents.size() << " documents for query ["s << queries[id++] << "]"s << std::endl;
    }
}
/*3 documents for query [nasty rat -not]
5 documents for query [not very funny nasty pet]
2 documents for query [curly hair]*/

void TestProcessQueriesJoined() {
    SearchServer search_server("and with"s);

    search_server.AddDocument(1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, {1, 2});
    search_server.AddDocument(2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2});
    search_server.AddDocument(3, "funny pet and not very nasty rat"s, DocumentStatus::ACTUAL, {1, 2});
    search_server.AddDocument(4, "pet with rat and rat and rat"s, DocumentStatus::ACTUAL, {1, 2});
    search_server.AddDocument(5, "nasty rat with curly hair"s, DocumentStatus::ACTUAL, {1, 2});

    const std::vector<std::string> queries = {
            "nasty rat -not"s,
            "not very funny nasty pet"s,
            "curly hair"s
    };
    for (const Document& document : ProcessQueriesJoined(search_server, queries)) {
        std::cout << "Document "s << document.id << " matched with relevance "s << document.relevance << std::endl;
    }
}
/*Document 1 matched with relevance 0.183492
Document 5 matched with relevance 0.183492
Document 4 matched with relevance 0.167358
Document 3 matched with relevance 0.743945
Document 1 matched with relevance 0.311199
Document 2 matched with relevance 0.183492
Document 5 matched with relevance 0.127706
Document 4 matched with relevance 0.0557859
Document 2 matched with relevance 0.458145
Document 5 matched with relevance 0.458145*/

void TestRemoveFunction() {
    SearchServer search_server("and with"s);

    search_server.AddDocument(1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, {1, 2});
    search_server.AddDocument(2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2});
    search_server.AddDocument(3, "funny pet and not very nasty rat"s, DocumentStatus::ACTUAL, {1, 2});
    search_server.AddDocument(4, "pet with rat and rat and rat"s, DocumentStatus::ACTUAL, {1, 2});
    search_server.AddDocument(5, "nasty rat with curly hair"s, DocumentStatus::ACTUAL, {1, 2});

    const std::string query = "curly and funny"s;

    auto report = [&search_server, &query] {
        std::cout << search_server.GetDocumentCount() << " documents total, "s
                  << search_server.FindTopDocuments(query).size() << " documents for query ["s << query << "]"s << std::endl;
    };

    report();
    // однопоточная версия
    search_server.RemoveDocument(5);
    report();
    // однопоточная версия
    search_server.RemoveDocument(std::execution::seq, 1);
    report();
    // многопоточная версия
    search_server.RemoveDocument(std::execution::par, 2);
    report();
}
/*5 documents total, 4 documents for query [curly and funny]
4 documents total, 3 documents for query [curly and funny]
3 documents total, 2 documents for query [curly and funny]
2 documents total, 1 documents for query [curly and funny]*/

void TestMatchDocument() {
    SearchServer search_server("and with"s);

    search_server.AddDocument(1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, {1, 2});
    search_server.AddDocument(2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2});
    search_server.AddDocument(3, "funny pet and not very nasty rat"s, DocumentStatus::ACTUAL, {1, 2});
    search_server.AddDocument(4, "pet with rat and rat and rat"s, DocumentStatus::ACTUAL, {1, 2});
    search_server.AddDocument(5, "nasty rat with curly hair"s, DocumentStatus::ACTUAL, {1, 2});

    const std::string query = "curly and funny -not"s;

    {
        const auto [words, status] = search_server.MatchDocument(query, 1);
        std::cout << words.size() << " words for document 1"s << std::endl;
        // 1 words for document 1
    }

    {
        const auto [words, status] = search_server.MatchDocument(std::execution::seq, query, 2);
        std::cout << words.size() << " words for document 2"s << std::endl;
        // 2 words for document 2
    }

    {
        const auto [words, status] = search_server.MatchDocument(std::execution::par, query, 3);
        std::cout << words.size() << " words for document 3"s << std::endl;
        // 0 words for document 3
    }
}
/*1 words for document 1
2 words for document 2
0 words for document 3*/

void
AssertImpl(bool value, const std::string& expr_str, const std::string& file, const std::string& func, unsigned line,
           const std::string& hint) {
    if (!value) {
        std::cerr << file << "("s << line << "): "s << func << ": "s;
        std::cerr << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            std::cerr << " Hint: "s << hint;
        }
        std::cerr << std::endl;
        abort();
    }
}