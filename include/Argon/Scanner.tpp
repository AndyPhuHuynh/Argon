#pragma once
#include <Argon/Scanner.hpp>

#include <algorithm>
#include <sstream>
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

inline bool Argon::Token::operator==(std::vector<Token>::const_reference token) const {
    return kind == token.kind && image == token.image;
}

inline std::string Argon::getDefaultImage(const TokenKind kind) {
    static const std::unordered_map<TokenKind, std::string> kindToImage = {
        { TokenKind::LBRACK, "[" },
        { TokenKind::RBRACK, "]" },
        { TokenKind::END, "" }
    };

    return kindToImage.contains(kind) ? kindToImage.at(kind) : "";
}

inline bool Argon::isOneOf(const Token& token, const std::initializer_list<TokenKind> &kinds) {
    return std::ranges::contains(kinds, token.kind);
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

inline char Argon::Scanner::peekChar() const {
    if (m_bufferPos >= m_buffer.size()) {
        return EOF;
    }
    return m_buffer[m_bufferPos];
}

inline char Argon::Scanner::nextChar() {
    if (m_bufferPos >= m_buffer.size()) {
        return EOF;
    }
    return m_buffer[m_bufferPos++];
}

inline Argon::Token Argon::Scanner::peekToken() const {
    if (m_tokenPos >= m_tokens.size()) {
        return {TokenKind::END, static_cast<int>(m_buffer.size())};
    } else {
        return m_tokens[m_tokenPos];
    }
}

inline Argon::Token Argon::Scanner::getNextToken() {
    if (m_tokenPos >= m_tokens.size()) {
        return {TokenKind::END, static_cast<int>(m_buffer.size())};
    } else {
        return m_tokens[m_tokenPos++];
    }
}

inline void Argon::Scanner::scanUntilSee(const std::initializer_list<TokenKind>& kinds) {
    while (!seeTokenKind(kinds) && !seeTokenKind(TokenKind::END)) {
        getNextToken();
    }
}

inline Argon::Token Argon::Scanner::scanUntilGet(const std::initializer_list<TokenKind>& kinds) {
    scanUntilSee(kinds);
    return getNextToken();
}


inline void Argon::Scanner::recordPosition() {
    m_rewindPos = m_tokenPos;
}

inline void Argon::Scanner::rewind() {
    m_tokenPos = m_rewindPos;
}

inline void Argon::Scanner::scanNextToken() {
    int position = static_cast<int>(m_bufferPos);
    char ch = nextChar();
    while (true) {
        if (ch == ' ') {
            ch = nextChar();
            continue;
        }
        else if (ch == EOF) {
            m_tokens.emplace_back(TokenKind::END, position);
            return;
        } else if (ch == '[') {
            m_tokens.emplace_back(TokenKind::LBRACK, position);
            return;
        } else if (ch == ']') {
            m_tokens.emplace_back(TokenKind::RBRACK, position);
            return;
        }
        // Token is identifier
        else {
            std::stringstream ss;
            ss << ch;
            ch = peekChar();
            while (ch != ' ' && ch != EOF && ch != '[' && ch != ']') {
                ch = nextChar();
                ss << ch;
                ch = peekChar();
            }
            m_tokens.emplace_back(TokenKind::IDENTIFIER, ss.str(), position);
            return;
        }
    }
}

inline void Argon::Scanner::scanBuffer() {
    while (m_bufferPos < m_buffer.size() ||  m_tokens.back().kind != TokenKind::END) {
        scanNextToken();
    }
}
