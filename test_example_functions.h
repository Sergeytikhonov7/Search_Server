

#pragma once

#include "search_server.h"
#include "process_queries.h"

using std::string_literals::operator""s;

void PrintDocument(const Document& document);

void PrintMatchDocumentResult(int document_id,
                              const std::vector<std::string>& words,
                              DocumentStatus status);

void AddDocument(SearchServer& search_server,
                 int document_id,
                 const std::string& document,
                 DocumentStatus status,
                 const std::vector<int>& ratings);

void FindTopDocuments(const SearchServer& search_server,
                      const std::string& raw_query);

void MatchDocuments(const SearchServer& search_server, const std::string& query);

void TestProcessQueries();

void TestProcessQueriesJoined();

void TestRemoveFunction();

void TestMatchDocument();


template<typename Collection>
std::ostream& Print(std::ostream& out, Collection& container) {
    int size = container.size();
    for (const auto& element : container) {
        if (size > 1) {
            out << element << ", "s;
        } else {
            out << element;
        }
        --size;
    }
    return out;
}

template<typename T>
std::ostream& operator<<(std::ostream& out, const std::set<T>& container) {
    out << "{";
    Print(out, container);
    out << "}";
    return out;
}

template<typename T>
std::ostream& operator<<(std::ostream& out, const std::vector<T>& container) {
    out << "[";
    Print(out, container);
    out << "]";
    return out;
}

template<typename First, typename Second>
std::ostream& operator<<(std::ostream& out, const std::pair<First, Second>& container) {
    return out << container.first << ": " << container.second;
}


template<typename Key, typename Value>
std::ostream& operator<<(std::ostream& out, const std::map<Key, Value>& container) {
    out << "{";
    Print(out, container);
    out << "}";
    return out;
}

template<typename T, typename U>
void
AssertEqualImpl(const T& t, const U& u, const std::string& t_str, const std::string& u_str, const std::string& file,
                const std::string& func, unsigned line, const std::string& hint) {
    if (t != u) {
        std::cerr << std::boolalpha;
        std::cerr << file << "("s << line << "): "s << func << ": "s;
        std::cerr << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        std::cerr << t << " != "s << u << "."s;
        if (!hint.empty()) {
            std::cerr << " Hint: "s << hint;
        }
        std::cerr << std::endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void
AssertImpl(bool value, const std::string& expr_str, const std::string& file, const std::string& func, unsigned line,
           const std::string& hint);

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

template<typename T>
void RunTestImpl(const T& func, const std::string& func_str) {
    func();
    std::cerr << func_str << " OK" << std::endl;
}

#define RUN_TEST(func)  RunTestImpl ((func), #func)