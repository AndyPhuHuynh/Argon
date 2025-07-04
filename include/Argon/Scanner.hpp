﻿#ifndef ARGON_SCANNER_INCLUDE
#define ARGON_SCANNER_INCLUDE

#include <cstdint>
#include <optional>
#include <ostream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace Argon {
    enum class TokenKind : uint8_t {
        NONE = 0,
        LBRACK,
        RBRACK,
        IDENTIFIER,
        EQUALS,
        END,
    };

    struct Token {
        TokenKind kind = TokenKind::NONE;
        std::string image;
        int position = -1;

        Token() = default;
        explicit Token(TokenKind kind);
        Token(TokenKind kind, std::string image);
        Token(TokenKind kind, int position);
        Token(TokenKind kind, std::string image, int position);

        auto operator==(const Token& token) const -> bool;

        [[nodiscard]] bool isOneOf(const std::initializer_list<TokenKind>& kinds) const;
    };

    std::string getDefaultImage(TokenKind kind);
    
    class Scanner {
    public:
        Scanner() = default;
        explicit Scanner(std::string_view buffer);

        [[nodiscard]] bool seeTokenKind(TokenKind kind) const;
        [[nodiscard]] bool seeTokenKind(const std::initializer_list<TokenKind>& kinds) const;
        [[nodiscard]] std::optional<char> peekChar() const;
        auto nextChar() -> std::optional<char>;
        auto getNextToken() -> Token;
        [[nodiscard]] auto getAllTokens() const -> const std::vector<Token>&;
        [[nodiscard]] Token peekToken() const;
        
        void recordPosition();
        void rewindToPosition();
        auto rewind(uint32_t amount) -> std::span<const Token>;
    private:
        std::vector<Token> m_tokens;
        uint32_t m_tokenPos = 0;
        uint32_t m_rewindPos = 0;

        std::string_view m_buffer;
        uint32_t m_bufferPos = 0;

        void scanNextToken();
        void scanBuffer();

        [[nodiscard]] Token getEndToken() const;
    };
}

// --------------------------------------------- Implementations -------------------------------------------------------

#include <algorithm>
#include <unordered_map>

inline Argon::Token::Token(const TokenKind kind) : kind(kind) {
    image = getDefaultImage(kind);
}

inline Argon::Token::Token(const TokenKind kind, std::string image) : kind(kind), image(std::move(image)) {}

inline Argon::Token::Token(const TokenKind kind, const int position) : kind(kind), position(position) {
    image = getDefaultImage(kind);
}

inline Argon::Token::Token(const TokenKind kind, std::string image, const int position)
    : kind(kind), image(std::move(image)), position(position) {}

inline bool Argon::Token::operator==(const Token& token) const {
    return (kind == token.kind) &&
           (image == token.image) &&
           (position == token.position);
}

inline bool Argon::Token::isOneOf(const std::initializer_list<TokenKind> &kinds) const {
    return std::ranges::contains(kinds, kind);
}

inline std::string Argon::getDefaultImage(const TokenKind kind) {
    static const std::unordered_map<TokenKind, std::string> kindToImage = {
        { TokenKind::LBRACK, "[" },
        { TokenKind::RBRACK, "]" },
        { TokenKind::EQUALS, "=" },
        { TokenKind::END, "" }
    };

    return kindToImage.contains(kind) ? kindToImage.at(kind) : "";
}

inline Argon::Scanner::Scanner(const std::string_view buffer) {
    m_buffer = buffer;
    scanBuffer();
}

inline bool Argon::Scanner::seeTokenKind(const TokenKind kind) const {
    return peekToken().kind == kind;
}

inline bool Argon::Scanner::seeTokenKind(const std::initializer_list<TokenKind>& kinds) const {
    return std::ranges::contains(kinds, peekToken().kind);
}

inline std::optional<char> Argon::Scanner::peekChar() const {
    if (m_bufferPos >= m_buffer.size()) {
        return std::nullopt;
    }
    return m_buffer[m_bufferPos];
}

inline std::optional<char> Argon::Scanner::nextChar() {
    if (m_bufferPos >= m_buffer.size()) {
        return std::nullopt;
    }
    return m_buffer[m_bufferPos++];
}

inline Argon::Token Argon::Scanner::peekToken() const {
    if (m_tokenPos >= m_tokens.size()) {
        return getEndToken();
    } else {
        return m_tokens[m_tokenPos];
    }
}

inline Argon::Token Argon::Scanner::getNextToken() {
    if (m_tokenPos >= m_tokens.size()) {
        return getEndToken();
    } else {
        return m_tokens[m_tokenPos++];
    }
}

inline auto Argon::Scanner::getAllTokens() const -> const std::vector<Token>& {
    return m_tokens;
}

inline void Argon::Scanner::recordPosition() {
    m_rewindPos = m_tokenPos;
}

inline void Argon::Scanner::rewindToPosition() {
    m_tokenPos = m_rewindPos;
}

inline auto Argon::Scanner::rewind(const uint32_t amount) -> std::span<const Token> {
    const uint32_t rewindAmount = std::min(m_tokenPos, amount);
    m_tokenPos -= rewindAmount;
    return {m_tokens.data() + m_tokenPos, rewindAmount};
}

inline void Argon::Scanner::scanNextToken() {
    int position = static_cast<int>(m_bufferPos);
    auto optCh = nextChar();
    while (true) {
        if (!optCh.has_value()) {
            m_tokens.emplace_back(TokenKind::END, position);
            return;
        }
        char ch = optCh.value();

        if (ch == ' ') {
            optCh = nextChar();
            position++;
            continue;
        }
        if (ch == '[') {
            m_tokens.emplace_back(TokenKind::LBRACK, position);
            return;
        }
        if (ch == ']') {
            m_tokens.emplace_back(TokenKind::RBRACK, position);
            return;
        }
        if (ch == '=') {
            m_tokens.emplace_back(TokenKind::EQUALS, position);
            return;
        }

        std::string image;
        image += ch;
        while (true) {
            optCh = peekChar();
            if (!optCh.has_value()) break;

            ch = optCh.value();
            if (ch == ' ' || ch == '[' || ch == ']' || ch == '=') break;

            image += nextChar().value();
        }
        m_tokens.emplace_back(TokenKind::IDENTIFIER, image, position);
        return;
    }
}

inline void Argon::Scanner::scanBuffer() {
    while (m_bufferPos < m_buffer.size()) {
        scanNextToken();
    }
    if (m_tokens.empty() || m_tokens.back().kind != TokenKind::END) {
        m_tokens.emplace_back(TokenKind::END, static_cast<int>(m_buffer.size()));
    }
}

inline Argon::Token Argon::Scanner::getEndToken() const {
    return m_tokens.back();
}

inline auto operator<<(std::ostream& os, const Argon::TokenKind kind) -> std::ostream& {
    switch (kind) {
        case Argon::TokenKind::NONE:        return os << "NONE";
        case Argon::TokenKind::LBRACK:      return os << "LBRACK";
        case Argon::TokenKind::RBRACK:      return os << "RBRACK";
        case Argon::TokenKind::IDENTIFIER:  return os << "IDENTIFIER";
        case Argon::TokenKind::EQUALS:      return os << "EQUALS";
        case Argon::TokenKind::END:         return os << "END";
    }
    return os << "UNKNOWN";
}

inline auto operator<<(std::ostream& os, const Argon::Token& token) -> std::ostream& {
    os  << "Token("
        << "kind=" << token.kind
        << ", image=\"" << token.image << "\""
        << ", position=" << token.position
        << ")";
    return os;
}

#endif // ARGON_SCANNER_INCLUDE