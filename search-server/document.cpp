#include "document.h"

using namespace std;

ostream& operator <<(ostream& os, const Document& d) {
    os << "{ document_id = "s << d.id << ", relevance = "s << d.relevance << ", rating = "s << d.rating << " }"s;
    return os;
}