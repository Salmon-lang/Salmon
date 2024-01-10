#include "scanner.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

typedef struct Scanner {
  const char *start;
  const char *current;
  size_t line;
} Scanner;

Scanner scanner;

void init_scanner(const char *source) {
  scanner.start = source;
  scanner.current = source;
  scanner.line = 1;
}

static bool is_digit(char c) { return c >= '0' && c <= '9'; }

static bool is_alpha(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool is_at_end() { return *scanner.current == '\0'; }

static char advance() {
  scanner.current++;
  return scanner.current[-1];
}

static char peek() { return *scanner.current; }

static char peek_next() {
  if (is_at_end()) {
    return '\0';
  }
  return scanner.current[1];
}

static bool match(char expected) {
  if (is_at_end()) {
    return false;
  }
  if (*scanner.current != expected) {
    return false;
  }
  scanner.current++;
  return true;
}

static Token make_token(TokenType type) {
  Token token;
  token.type = type;
  token.start = scanner.start;
  token.length = (size_t)(scanner.current - scanner.start);
  token.line = scanner.line;
  return token;
}

static Token error_token(const char *message) {
  Token token;
  token.type = TOKEN_ERROR;
  token.start = message;
  token.length = (size_t)strlen(message);
  token.line = scanner.line;
  return token;
}

static bool is_whitespace() {
  char c = peek();
  switch (c) {
  case ' ':
  case '\t':
  case '\n':
  case '\r':
    return true;
  default:
    return false;
  }
}

static void skip_whitespace() {
  for (;;) {
    char c = peek();
    switch (c) {
    case ' ':
    case '\r':
    case '\t':
      advance();
      break;
    case '\n':
      scanner.line++;
      advance();
      break;
    case '/':
      if (peek_next() == '/') {
        while (peek() != '\n' && !is_at_end()) {
          advance();
        }
      } else {
        return;
      }
      break;
    default:
      return;
    }
  }
}

static TokenType check_keyword(size_t start, size_t length, const char *rest,
                               TokenType type) {
  if (scanner.current - scanner.start == start + length &&
      memcmp(scanner.start + start, rest, length) == 0) {
    return type;
  }
  return TOKEN_IDENTIFIER;
}

static TokenType identifier_type() {
  switch (scanner.start[0]) {
  case 'c':
    return check_keyword(1, 4, "lass", TOKEN_CLASS);
  case 'e':
    return check_keyword(1, 3, "lse", TOKEN_ELSE);
  case 'i':
    return check_keyword(1, 1, "f", TOKEN_IF);
  case 'n':
    return check_keyword(1, 2, "il", TOKEN_NIL);
  case 'r':
    return check_keyword(1, 5, "eturn", TOKEN_RETURN);
  case 's':
    return check_keyword(1, 4, "uper", TOKEN_SUPER);
  case 'v':
    return check_keyword(1, 2, "ar", TOKEN_VAR);
  case 'w':
    return check_keyword(1, 4, "hile", TOKEN_WHILE);
  case '_':
    return check_keyword(1, 11, "___path____", TOKEN_PATH);
  case 'f':
    if (scanner.current - scanner.start > 1) {
      switch (scanner.start[1]) {
      case 'a':
        return check_keyword(2, 3, "lse", TOKEN_FALSE);
      case 'o':
        return check_keyword(2, 1, "r", TOKEN_FOR);
      case 'u':
        return check_keyword(2, 6, "nction", TOKEN_FUN);
      }
    }
    break;
  case 't':
    if (scanner.current - scanner.start > 1) {
      switch (scanner.start[1]) {
      case 'h':
        return check_keyword(2, 2, "is", TOKEN_THIS);
      case 'r':
        return check_keyword(2, 2, "ue", TOKEN_TRUE);
      }
    }
    break;
  }

  return TOKEN_IDENTIFIER;
}

static Token identifier() {
  while (is_alpha(peek()) || is_digit(peek())) {
    advance();
  }
  return make_token(identifier_type());
}

static Token number() {
  while (is_digit(peek())) {
    advance();
  }

  if (peek() == '.' && is_digit(peek_next())) {
    advance();
    while (is_digit(peek())) {
      advance();
    }
  }
  return make_token(TOKEN_NUMBER);
}

static Token file_path() {
  while (!is_whitespace() && !is_at_end()) {
    advance();
  }
  return make_token(TOKEN_FILE_PATH);
}

static Token string() {
  while ((peek() != '"') && !is_at_end()) {
    if (peek() == '\n') {
      scanner.line++;
    }
    if (peek() == '\\') {
      switch (peek_next()) {
      case '\\':
      case '\"':
      case '\r':
      case '\n':
      case '\t':
        advance();
        break;
      }
    }
    advance();
  }

  if (is_at_end()) {
    return error_token("Unterminated string.");
  }

  advance();
  return make_token(TOKEN_STRING);
}

Token scan_token() {
  skip_whitespace();
  scanner.start = scanner.current;

  if (is_at_end()) {
    return make_token(TOKEN_EOF);
  }

  char c = advance();
  if (is_digit(c)) {
    return number();
  }
  if (is_alpha(c) || c == '#') {
    return identifier();
  }

  switch (c) {
  case '(':
    return make_token(TOKEN_LEFT_PAREN);
  case ')':
    return make_token(TOKEN_RIGHT_PAREN);
  case '{':
    return make_token(TOKEN_LEFT_BRACE);
  case '}':
    return make_token(TOKEN_RIGHT_BRACE);
  case '[':
    return make_token(TOKEN_LEFT_BRACKET);
  case ']':
    return make_token(TOKEN_RIGHT_BRACKET);
  case ';':
    return make_token(TOKEN_SEMICOLON);
  case ',':
    return make_token(TOKEN_COMMA);
  case '.':
    return make_token(TOKEN_DOT);
  case '=':
    return make_token(TOKEN_EQUAL_EQUAL);
  case '|':
    return make_token(TOKEN_OR);
  case '&':
    return make_token(TOKEN_AND);
  case '+':
    return make_token(match('=') ? TOKEN_PLUS_EQUAL : TOKEN_PLUS);
  case '-':
    return make_token(match('=') ? TOKEN_MINUS_EQUAL : TOKEN_MINUS);
  case '*':
    return make_token(match('=') ? TOKEN_STAR_EQUAL : TOKEN_STAR);
  case '/':
    return make_token(match('=') ? TOKEN_SLASH_EQUAL : TOKEN_SLASH);
#ifdef __unix__
  case '~':
    return file_path();
#else
  case 'C':
    if (match(':')) {
      return file_path();
    }
    break;
#endif /* ifdef __UNIX__ */
  case ':': {
    if (match('=')) {
      return make_token(TOKEN_EQUAL);
    }
    break;
  }
  case '!':
    return make_token(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
  case '<':
    return make_token(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
  case '>':
    return make_token(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
  case '"':
    return string();
  }

  return error_token("Unexpected character.");
}
