#include <numeric>

void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
     
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT(found_docs.size() == 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT(server.FindTopDocuments("in"s).empty());
    }
}

void TestMinusWords() {
    const int doc_id_1 = 42;
    const int doc_id_2 = 80;
    const string content_1 = "cat in the city"s;
    const string content_2 = "cat in the home"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings);
        auto found_docs = server.FindTopDocuments("cat -city"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        Document doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id_2);
        found_docs = server.FindTopDocuments("home"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id_2);
    }
}

void TestMatchDocument() {
    const int doc_id_1 = 42;
    const int doc_id_2 = 80;
    const string content_1 = "cat in the city"s;
    const string content_2 = "cat in the home"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id_2, content_2, DocumentStatus::BANNED, ratings);
        vector<string> v = {"cat"s, "city"s}; 
        
        {
            const auto [words, status] = server.MatchDocument("cat city"s, doc_id_1);
            ASSERT_EQUAL(words, v);
            ASSERT(status == DocumentStatus::ACTUAL);
        }

        {
            const auto [words, status] = server.MatchDocument("cat -home"s, doc_id_2);
            ASSERT(words.empty());
            ASSERT(status == DocumentStatus::BANNED);
        }

        {
            const auto [words, status] = server.MatchDocument("home dog"s, doc_id_1);
            ASSERT(words.empty());
            ASSERT(status == DocumentStatus::ACTUAL);
        }
        
        {
            const auto [words, status] = server.MatchDocument("in the"s, doc_id_1);
            ASSERT(words.empty());
            ASSERT(status == DocumentStatus::ACTUAL);
        }
    }
}

void TestSortRelevance() {
    const int doc_id_1 = 42;
    const int doc_id_2 = 80;
    const int doc_id_3 = 10;
    const string content_1 = "cat in the city"s;
    const string content_2 = "cat in the home"s;
    const string content_3 = "dog in the home"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id_3, content_3, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("cat city in"s);
        ASSERT_EQUAL(found_docs.size(), 3);
        ASSERT(found_docs[0].relevance > found_docs[1].relevance);
        ASSERT(found_docs[1].relevance > found_docs[2].relevance);   
    }
}

void TestAverageRating() {
    const int doc_id_1 = 42;
    const int doc_id_2 = 80;
    const int doc_id_3 = 10;
    const string content_1 = "cat in the city"s;
    const string content_2 = "cat in the home"s;
    const string content_3 = "dog in the home"s;
    const vector<int> ratings_1 = {1, 2, 3};
    const vector<int> ratings_2 = {1, -2, 3};
    const vector<int> ratings_3 = {};
    {
        SearchServer server;
        server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
        server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
        server.AddDocument(doc_id_3, content_3, DocumentStatus::ACTUAL, ratings_3);
        const auto found_docs = server.FindTopDocuments("cat city in"s);
        const int average_rating_1 = accumulate(ratings_1.begin(), ratings_1.end(), 0) / ratings_1.size();
        const int average_rating_2 = 0;
        const int average_rating_3 = 0;
        ASSERT_EQUAL(found_docs[0].rating, average_rating_1);
        ASSERT_EQUAL(found_docs[1].rating, average_rating_2);
        ASSERT_EQUAL(found_docs[2].rating, average_rating_3);
    }
}

void TestPredicatFunction() {
    const int doc_id_1 = 42;
    const int doc_id_2 = 81;
    const int doc_id_3 = 10;
    const string content_1 = "cat in the city"s;
    const string content_2 = "cat in the home"s;
    const string content_3 = "dog in the home"s;
    const vector<int> ratings_1 = {1, 2, 3};
    const vector<int> ratings_2 = {-1, 2, 3};
    const vector<int> ratings_3 = {};
    {
        SearchServer server;
        server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
        server.AddDocument(doc_id_2, content_2, DocumentStatus::BANNED, ratings_2);
        server.AddDocument(doc_id_3, content_3, DocumentStatus::ACTUAL, ratings_3);
        
        {
            const auto found_docs = server.FindTopDocuments("cat city in"s, DocumentStatus::BANNED);
            ASSERT_EQUAL(found_docs.size(), 1);
            ASSERT_EQUAL(found_docs[0].id, doc_id_2);
        }

        {
            const auto found_docs = server.FindTopDocuments("cat city in"s, DocumentStatus::ACTUAL);
            ASSERT_EQUAL(found_docs.size(), 2);
            ASSERT_EQUAL(found_docs[0].id, doc_id_1);
            ASSERT_EQUAL(found_docs[1].id , doc_id_3);
        }

        {
            const auto found_docs = server.FindTopDocuments("cat city in"s, [](int document_id, DocumentStatus status, int rating){
                return status == DocumentStatus::BANNED;
            });
            ASSERT_EQUAL(found_docs.size(), 1);
            ASSERT_EQUAL(found_docs[0].id, doc_id_2);
        }

        {
            const auto found_docs = server.FindTopDocuments("cat city in"s, [](int document_id, DocumentStatus status, int rating){
                return document_id % 2 == 1;
            });
            ASSERT_EQUAL(found_docs.size(), 1); 
            ASSERT_EQUAL(found_docs[0].id, doc_id_2);   
        }

        {
            const auto found_docs = server.FindTopDocuments("cat city in"s, [](int document_id, DocumentStatus status, int rating){
                return rating == 0;
            });
            ASSERT_EQUAL(found_docs.size(), 1); 
            ASSERT_EQUAL(found_docs[0].id, doc_id_3);
        }
    }
}

void TestCorrectComputeRelevance() {
    const int doc_id_1 = 42;
    const int doc_id_2 = 81;
    const int doc_id_3 = 10;
    const string content_1 = "cat in the city"s;
    const string content_2 = "cat in the home"s;
    const string content_3 = "dog in the home"s;
    const vector<int> ratings_1 = {1, 2, 3};
    const vector<int> ratings_2 = {-1, 2, 3};
    const vector<int> ratings_3 = {};
    {
       SearchServer server;
        server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
        server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
        server.AddDocument(doc_id_3, content_3, DocumentStatus::ACTUAL, ratings_3);
        const auto found_docs = server.FindTopDocuments("cat city in"s);
        const double EPSILON = 1e-6;
        const double relevance_1 = 0.376019;
        const double relevance_2 = 0.101366;
        const double relevance_3 = 0;
        ASSERT(abs(found_docs[0].relevance - relevance_1) < EPSILON);
        ASSERT(abs(found_docs[1].relevance - relevance_2) < EPSILON);
        ASSERT(abs(found_docs[2].relevance - relevance_3) < EPSILON);
    }
}

void TestGetDocumentCount() {
    const int doc_id_1 = 42;
    const int doc_id_2 = 81;
    const int doc_id_3 = 10;
    const string content_1 = "cat in the city"s;
    const string content_2 = "cat in the home"s;
    const string content_3 = "dog in the home"s;
    const vector<int> ratings_1 = {1, 2, 3};
    const vector<int> ratings_2 = {-1, 2, 3};
    const vector<int> ratings_3 = {};
    {
        SearchServer server; 
        server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
        server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
        server.AddDocument(doc_id_3, content_3, DocumentStatus::ACTUAL, ratings_3);
        const int count_doc = 3;
        ASSERT_EQUAL(server.GetDocumentCount(), count_doc);
    }
}

void TestAddDocument() {
    const int doc_id = 42; 
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server; 
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const vector<Document> doc = server.FindTopDocuments("cat city in");
        ASSERT_EQUAL(doc[0].id, doc_id); 
        const int average_rating = accumulate(ratings.begin(), ratings.end(), 0) / ratings.size();
        ASSERT_EQUAL(doc[0].rating, average_rating);
    } 
}

void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestMinusWords);
    RUN_TEST(TestMatchDocument);
    RUN_TEST(TestSortRelevance);
    RUN_TEST(TestAverageRating);
    RUN_TEST(TestPredicatFunction);
    RUN_TEST(TestCorrectComputeRelevance);
    RUN_TEST(TestGetDocumentCount);
    RUN_TEST(TestAddDocument);
}