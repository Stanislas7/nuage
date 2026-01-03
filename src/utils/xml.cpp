#include "utils/xml.hpp"
#include <cctype>
#include <cstring>
#include <fstream>
#include <sstream>
#include <sstream>

namespace nuage {
namespace {

std::string trim(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) {
        ++start;
    }
    size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) {
        --end;
    }
    return s.substr(start, end - start);
}

void skipWhitespace(const std::string& content, size_t& i) {
    while (i < content.size() && std::isspace(static_cast<unsigned char>(content[i]))) {
        ++i;
    }
}

bool startsWith(const std::string& content, size_t i, const char* token) {
    size_t len = std::strlen(token);
    return content.compare(i, len, token) == 0;
}

bool consumeUntil(const std::string& content, size_t& i, const char* token) {
    size_t pos = content.find(token, i);
    if (pos == std::string::npos) {
        return false;
    }
    i = pos + std::strlen(token);
    return true;
}

std::optional<std::string> parseName(const std::string& content, size_t& i) {
    skipWhitespace(content, i);
    if (i >= content.size()) {
        return std::nullopt;
    }
    size_t start = i;
    while (i < content.size()) {
        char c = content[i];
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == ':') {
            ++i;
        } else {
            break;
        }
    }
    if (start == i) {
        return std::nullopt;
    }
    return content.substr(start, i - start);
}

std::optional<std::string> parseAttributeValue(const std::string& content, size_t& i) {
    skipWhitespace(content, i);
    if (i >= content.size() || content[i] != '=') {
        return std::nullopt;
    }
    ++i;
    skipWhitespace(content, i);
    if (i >= content.size()) {
        return std::nullopt;
    }
    char quote = content[i];
    if (quote != '"' && quote != '\'') {
        return std::nullopt;
    }
    ++i;
    size_t start = i;
    size_t end = content.find(quote, start);
    if (end == std::string::npos) {
        return std::nullopt;
    }
    i = end + 1;
    return content.substr(start, end - start);
}

std::optional<XmlNode> parseElement(const std::string& content, size_t& i) {
    if (i >= content.size() || content[i] != '<') {
        return std::nullopt;
    }
    ++i;
    if (i < content.size() && content[i] == '/') {
        return std::nullopt;
    }
    auto nameOpt = parseName(content, i);
    if (!nameOpt) {
        return std::nullopt;
    }
    XmlNode node;
    node.name = *nameOpt;

    while (i < content.size()) {
        skipWhitespace(content, i);
        if (i >= content.size()) {
            return std::nullopt;
        }
        if (content[i] == '/' || content[i] == '>') {
            break;
        }
        auto attrNameOpt = parseName(content, i);
        if (!attrNameOpt) {
            return std::nullopt;
        }
        auto valueOpt = parseAttributeValue(content, i);
        if (!valueOpt) {
            return std::nullopt;
        }
        node.attributes[*attrNameOpt] = *valueOpt;
    }

    skipWhitespace(content, i);
    if (i >= content.size()) {
        return std::nullopt;
    }
    if (content[i] == '/') {
        ++i;
        if (i >= content.size() || content[i] != '>') {
            return std::nullopt;
        }
        ++i;
        return node;
    }
    if (content[i] != '>') {
        return std::nullopt;
    }
    ++i;

    std::string textAccum;
    while (i < content.size()) {
        if (content[i] == '<') {
            if (startsWith(content, i, "</")) {
                i += 2;
                auto endNameOpt = parseName(content, i);
                if (!endNameOpt || *endNameOpt != node.name) {
                    return std::nullopt;
                }
                skipWhitespace(content, i);
                if (i >= content.size() || content[i] != '>') {
                    return std::nullopt;
                }
                ++i;
                node.text = trim(textAccum);
                return node;
            }
            auto childOpt = parseElement(content, i);
            if (!childOpt) {
                return std::nullopt;
            }
            node.children.push_back(std::move(*childOpt));
        } else {
            textAccum.push_back(content[i]);
            ++i;
        }
    }
    return std::nullopt;
}

std::string stripXmlPrologAndComments(const std::string& content) {
    std::string out;
    out.reserve(content.size());
    size_t i = 0;
    while (i < content.size()) {
        if (startsWith(content, i, "<?")) {
            i += 2;
            if (!consumeUntil(content, i, "?>")) {
                break;
            }
            continue;
        }
        if (startsWith(content, i, "<!--")) {
            i += 4;
            if (!consumeUntil(content, i, "-->")) {
                break;
            }
            continue;
        }
        out.push_back(content[i]);
        ++i;
    }
    return out;
}

} // namespace

std::optional<XmlNode> parseXml(const std::string& content) {
    std::string cleaned = stripXmlPrologAndComments(content);
    size_t i = 0;
    skipWhitespace(cleaned, i);
    if (i >= cleaned.size()) {
        return std::nullopt;
    }
    return parseElement(cleaned, i);
}

std::optional<XmlNode> loadXmlFile(const std::string& path) {
    std::ifstream in(path);
    if (!in.is_open()) {
        return std::nullopt;
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return parseXml(buffer.str());
}

} // namespace nuage
