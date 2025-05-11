#include <Argon/Scanner.hpp>

#include <sstream>
#include <unordered_map>

Argon::Token::Token(const TokenKind kind) : kind(kind) {
    image = getDefaultImage(kind);
}

Argon::Token::Token(const TokenKind kind, std::string image) : kind(kind), image(std::move(image)) {}

Argon::Token::Token(const TokenKind kind, const int position) : kind(kind), position(position) {
    image = getDefaultImage(kind);
}

Argon::Token::Token(const TokenKind kind, std::string image, const int position)
    : kind(kind), image(std::move(image)), position(position) {}

bool Argon::Token::operator==(std::vector<Token>::const_reference token) const {
    return kind == token.kind && image == token.image;
}

std::string Argon::getDefaultImage(const TokenKind kind) {
    static const std::unordered_map<TokenKind, std::string> kindToImage = {
        { TokenKind::LBRACK, "[" },
        { TokenKind::RBRACK, "]" },
        { TokenKind::END, "" }
    };

    return kindToImage.contains(kind) ? kindToImage.at(kind) : "";
}

bool Argon::Scanner::seeTokenKind(const TokenKind kind) {
    uint32_t currentPos = m_bufferPos;
    Token nextToken = getNextToken();
    bool found = nextToken.kind == kind;
    m_bufferPos = currentPos;
    return found;
}

char Argon::Scanner::peekChar() const {
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
    int position = static_cast<int>(m_bufferPos);
    char ch = nextChar();
    while (true) {
        if (ch == ' ') {
            ch = nextChar();
            continue;
        }
        else if (ch == EOF) {
            return {TokenKind::END, -1};
        } else if (ch == '[') {
            return {TokenKind::LBRACK, position};
        } else if (ch == ']') {
            return {TokenKind::RBRACK, position};
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
            return {TokenKind::IDENTIFIER, ss.str(), position};
        }
    }
}

void Argon::Scanner::scanUntilSee(const std::initializer_list<TokenKind>& kinds) {
    Token token;
    while (true) {
        bool found = false;
        for (auto kind : kinds) {
            if (seeTokenKind(kind)) {
                found = true;
                break;
            }
        }
        
        if (found || seeTokenKind(TokenKind::END)) {
            return;
        }
        getNextToken();
    }
}

Argon::Token Argon::Scanner::scanUntilGet(const std::initializer_list<TokenKind>& kinds) {
    scanUntilSee(kinds);
    return getNextToken();
}


void Argon::Scanner::recordPosition() {
    m_rewindPos = m_bufferPos;
}

void Argon::Scanner::rewind() {
    m_bufferPos = m_rewindPos;
}
