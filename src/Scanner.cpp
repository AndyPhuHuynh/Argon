#include <Argon/Scanner.hpp>

#include <sstream>
#include <unordered_map>

Argon::Token::Token(const TokenKind kind) : kind(kind) {
    image = getDefaultImage(kind);
}

Argon::Token::Token(const TokenKind kind, std::string image) : kind(kind), image(std::move(image)) {}

bool Argon::Token::operator==(std::vector<Token>::const_reference token) const {
    return kind == token.kind && image == token.image;
}

std::string Argon::getDefaultImage(const TokenKind kind) {
    static const std::unordered_map<TokenKind, std::string> kindToImage = {
        { LBRACK, "[" },
        { RBRACK, "]" },
        { END, "" }
    };

    return kindToImage.contains(kind) ? kindToImage.at(kind) : "";
}

bool Argon::Scanner::seeTokenKind(const TokenKind kind) {
    recordPosition();
    Token nextToken = getNextToken();
    bool found = nextToken.kind == kind;
    rewind();
    return found;
}

char Argon::Scanner::peekChar() {
    if (m_bufferPos >= m_buffer.size()) {
        return EOF;
    }
    return m_buffer[m_bufferPos];
}

char Argon::Scanner::nextChar() {
    if (m_bufferPos >= m_buffer.size()) {
        return EOF;
    }
    return m_buffer[m_bufferPos++];
}

Argon::Token Argon::Scanner::getNextToken() {
    char ch = nextChar();
    while (true) {
        if (ch == ' ') {
            ch = nextChar();
            continue;
        }
        else if (ch == EOF) {
            return {END};
        } else if (ch == '[') {
            return {LBRACK};
        } else if (ch == ']') {
            return {RBRACK};
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
            return {IDENTIFIER, ss.str()};
        }
    }
}

void Argon::Scanner::recordPosition() {
    m_rewindPos = m_bufferPos;
}

void Argon::Scanner::rewind() {
    m_bufferPos = m_rewindPos;
}
