#ifndef ARGON_SCANNER_INCLUDE
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
        DOUBLE_DASH,
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

        [[nodiscard]] auto seeTokenKind(TokenKind kind) const -> bool;
        [[nodiscard]] auto seeTokenKind(const std::initializer_list<TokenKind>& kinds) const -> bool;
        [[nodiscard]] auto peekChar() const -> std::optional<char>;
        auto nextChar() -> std::optional<char>;
        auto getNextToken() -> Token;
        [[nodiscard]] auto getAllTokens() const -> const std::vector<Token>&;
        [[nodiscard]] auto peekToken() const -> Token;
        auto setCurrentToken(const Token& token) -> void;
        
        auto recordPosition() -> void;
        auto rewindToPosition() -> void;
        auto rewind(uint32_t amount) -> std::span<const Token>;
    private:
        std::vector<Token> m_tokens;
        uint32_t m_tokenPos = 0;
        uint32_t m_rewindPos = 0;

        std::string_view m_buffer;
        uint32_t m_bufferPos = 0;
        uint32_t m_bufferRewindPos = 0;

        auto recordBufferPosition() -> void;
        auto rewindToBufferPosition() -> void;

        auto scanNextToken() -> void;
        auto scanBuffer() -> void;

        [[nodiscard]] Token getEndToken() const;
    };
}

// --------------------------------------------- Implementations -------------------------------------------------------

#include <algorithm>

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
    switch (kind) {
        case TokenKind::LBRACK:
            return "[";
        case TokenKind::RBRACK:
            return "]";
        case TokenKind::EQUALS:
            return "=";
        case TokenKind::DOUBLE_DASH:
            return "--";
        case TokenKind::NONE:
        case TokenKind::END:
        case TokenKind::IDENTIFIER:
            return "";
    }
    return "";
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

inline auto Argon::Scanner::setCurrentToken(const Token& token) -> void {
    if (m_tokenPos >= m_tokens.size()) return;
    m_tokens[m_tokenPos] = token;
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

inline auto Argon::Scanner::recordBufferPosition() -> void {
    m_bufferRewindPos = m_bufferPos;
}

inline auto Argon::Scanner::rewindToBufferPosition() -> void {
    m_bufferPos = m_bufferRewindPos;
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
        if (ch == '-') {
            recordBufferPosition();
            const auto secondDash = nextChar();
            if (const auto space = nextChar();
                secondDash.has_value() && secondDash.value() == '-' &&
                ((space.has_value() && space.value() == ' ') || !space.has_value())) {
                m_tokens.emplace_back(TokenKind::DOUBLE_DASH, position);
                return;
            }
            rewindToBufferPosition();
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
        case Argon::TokenKind::DOUBLE_DASH: return os << "DOUBLE_DASH";
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