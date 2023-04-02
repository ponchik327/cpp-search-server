#pragma once
#include "document.h"
#include "search_server.h"
#include<vector>
#include<iterator>
#include<ostream>

template <typename Iterator>
class Diaposon {
    public:
        Diaposon(Iterator begin, Iterator end)
            : begin_(begin)
            , end_(end)
        {
        }

        auto begin() const{
            return begin_;
        } 

        auto end() const{
            return end_;
        }

        auto size() {
            return std::distance(begin_, end_);
        }

    private:
        Iterator begin_;
        Iterator end_;
};

template <typename Iterator>
std::ostream& operator <<(std::ostream& os, Diaposon<Iterator> d) {
    auto end = d.end();
    for (auto begin = d.begin(); begin != end; ++begin) {
        os << *begin;    
    }
    return os;
}

template <typename Iterator>
class Paginator {
    public:
        Paginator(Iterator begin, Iterator end, size_t page_size)
        {
            Splitting_into_pages(begin, end, page_size);
        }

        auto begin() const{
            return pages_.begin();
        } 

        auto end() const{
            return pages_.end();
        }

        auto size() {
            return pages_.size();
        }

    private:
        std::vector<Diaposon<Iterator>> pages_;

        void Splitting_into_pages(Iterator begin, Iterator end, size_t page_size) {
            while (begin != end) {
                if (std::distance(begin, end) > page_size) {
                    pages_.push_back(Diaposon<Iterator>(begin, begin + page_size));
                    std::advance(begin, page_size);
                } else {
                    pages_.push_back(Diaposon<Iterator>(begin, end));
                    break;
                }
            }
        }
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}