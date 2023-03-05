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
        const auto found_docs = server.FindTopDocuments("cat -city"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc0 = found_docs[0];
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
        server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings);
        vector<string> v = {"cat"s, "city"s}; 
        
        {
            const auto [words, status] = server.MatchDocument("cat city"s, 42);
            ASSERT_EQUAL(words, v);
        }

        {
            const auto [words, status] = server.MatchDocument("cat -home"s, 80);
            ASSERT(words.empty());
        }

        {
            const auto [words, status] = server.MatchDocument("home dog"s, 42);
            ASSERT(words.empty());
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
        for(int i = 0; i < found_docs.size(); ++i) {
            ASSERT_EQUAL(found_docs[i].relevance, found_docs[i].relevance);
        }    
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
        ASSERT_EQUAL(found_docs[0].rating, 2);
        ASSERT_EQUAL(found_docs[1].rating, 0);
        ASSERT_EQUAL(found_docs[2].rating, 0);
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
        ASSERT(abs(found_docs[0].relevance - 0.376019) < 1e-6);
        ASSERT(abs(found_docs[1].relevance - 0.101366) < 1e-6);
        (found_docs[0].relevance == 0);
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
}