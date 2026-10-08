#pragma once

#include "core/Types.hpp"
#include <vector>
#include <string>
#include <mutex>

namespace crypto {

class INewsProvider {
public:
    virtual ~INewsProvider() = default;
    virtual std::vector<NewsItem> fetchNews(int limit = 20) = 0;
    virtual std::string getProviderName() const = 0;
};

class NewsManager {
public:
    static NewsManager& instance();

    void init();
    void refreshNewsAsync();

    std::vector<NewsItem> getLatestNews();
    std::vector<NewsItem> getNewsForSymbol(const std::string& symbol);

private:
    NewsManager() = default;
    ~NewsManager() = default;

    void generateFallbackNews();

    mutable std::mutex m_mutex;
    std::vector<NewsItem> m_newsItems;
};

} // namespace crypto
