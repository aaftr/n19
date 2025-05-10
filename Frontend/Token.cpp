/*
* Copyright (c) 2024 Diago Lima
* SPDX-License-Identifier: BSD-3-Clause
*/

#include <Frontend/Lexer.hpp>
#include <IO/Fmt.hpp>
#include <Core/Panic.hpp>
#include <Core/Murmur3.hpp>
#include <algorithm>
#include <new>
BEGIN_NAMESPACE(n19);

auto Token::eof(
  const uint32_t pos, const uint32_t line ) -> Token
{
  Token token;
  token.pos_   = pos;
  token.line_  = line;
  token.cat_   = TokenCategory::NonCategorical;
  token.type_  = TokenType::EndOfFile;
  return token;
}

auto Token::illegal(
  const uint32_t pos,
  const uint32_t length, const uint32_t line ) -> Token
{
  Token token;
  token.len_   = length;
  token.pos_   = pos;
  token.line_  = line;
  token.cat_   = TokenCategory::NonCategorical;
  token.type_  = TokenType::Illegal;
  return token;
}

// Converts a given TokenType's underlying
// Type enumeration to a string.
auto TokenType::to_string() const -> std::string {
  #define X(TYPE, STR) case TokenType::TYPE: return #TYPE;
  switch(value) {
    N19_TOKEN_TYPE_LIST
    default: return "Unknown"; // Failsafe
  }
  #undef X
}

// Converts a TokenType to it's given string
// representation. This is different from converting
// the underlying type, for example:
// - TokenType::LogicalAnd becomes "&&".
// - TokenType::PlusEq becomes "+="
// etc.
auto TokenType::string_repr() const -> std::string {
#define X(TYPE, STR) case TokenType::TYPE: return STR;
  switch(value) {
    N19_TOKEN_TYPE_LIST
    default: return "Unknown"; // Failsafe
  }
#undef X
}

// Converts a given TokenCategory's underlying
// Type enumeration to a string.
auto TokenCategory::to_string() const -> std::string {
  #define X(CAT, UNUSED) if(value & CAT) buff += (std::string(#CAT) + " | ");
   std::string buff;
   N19_TOKEN_CATEGORY_LIST
  #undef X

  if(!buff.empty() && buff[buff.size() - 2] == '|') {
    buff.erase(buff.size() - 3);
  } else {
    buff += "NonCategorical";
  }

  return buff;
}

// Retrieves a TokenCategory value, using a given string
// which represents a keyword. The keyword may or may not exist.
// returns the category if it exists, or Nothing otherwise.
auto TokenCategory::from_keyword(const std::u8string_view& str)
-> Maybe<TokenCategory>
{
  constexpr uint32_t seed = 0xbeef;
  if(str.size() > 15) return Nothing;

  switch(murmur3_x86_32(str, seed)) {
#define KEYWORD_X(NAME, UNUSED, CAT) case u8##NAME##_mm32: return CAT;
    N19_HIR_KEYWORDS; default: break;
#undef KEYWORD_X
  }

  return Nothing;
}

// Gets a given token's "value". This is the exact
// way in which it appears in a source file. For example,
// an identifier of "foo" would be returned as such,
// a string of "foo". For the plus op it would return "+".
auto Token::value(const Lexer& lxr) const -> Maybe<std::string> {
  if(len_ == 0) return Nothing;
  const auto bytes = lxr.get_bytes();
  ASSERT(pos_ < bytes.size());
  ASSERT(pos_ + len_ - 1 < bytes.size());

  std::string ret;
  ret.reserve(len_);

  for(size_t i = pos_, j = 0; j < len_; i++, j++) {
    auto ch = bytes[i];
    ret += (static_cast<char>(ch));
  }
  return ret;
}

// Formats a token into a more readable representation.
// For debugging/testing purposes only.
auto Token::format(const Lexer &lxr) const -> std::string {
  std::string buffer;
  buffer += fmt("{:<12}: ", type_.to_string());
  buffer += fmt("\"{}\" -- ", value(lxr).value_or("N/A"));
  buffer += fmt("LINE={},POS={} -- ", line_, pos_);
  buffer += fmt("{}\n", cat_.to_string());
  return buffer;
}

// Retrieves a TokenType value, using a given string
// which represents a keyword. The keyword may or may not exist.
// returns the type if it exists, or Nothing otherwise.
auto TokenType::from_keyword(const std::u8string_view& keyword)
-> Maybe<TokenType>
{
  constexpr uint32_t seed = 0xbeef;
  if(keyword.size() > 15) return Nothing;

  switch(murmur3_x86_32(keyword, seed)) {
#define KEYWORD_X(NAME, TYPE, UNUSED) case u8##NAME##_mm32: return TYPE;
  N19_HIR_KEYWORDS; default: break;
#undef KEYWORD_X
  }

  return Nothing;
}

// is this token a terminating token? (';', ',')
auto Token::is_terminator() const -> bool {
  return type_ == TokenType::Semicolon || type_ == TokenType::Comma;
}

// TODO
// operator precedence of a given token.
auto Token::prec() const -> uint16_t {
  switch(type_.value) {
  case TokenType::ValueAssignment:
  case TokenType::PlusEq:
  case TokenType::SubEq:
  case TokenType::MulEq:
  case TokenType::DivEq:
  case TokenType::ModEq:
  case TokenType::LshiftEq:
  case TokenType::RshiftEq:
  case TokenType::BitwiseAndEq:
  case TokenType::BitwiseOrEq:
  case TokenType::XorEq:
  case TokenType::LogicalAnd:
  case TokenType::LogicalOr:
  case TokenType::Mul:
  case TokenType::NamespaceOperator:
  case TokenType::Dot:
  case TokenType::Eq:
  case TokenType::Neq:
  case TokenType::Lt:
  case TokenType::Lte:
  case TokenType::Gt:
  case TokenType::Gte:
  case TokenType::Plus:
  case TokenType::Sub:
  case TokenType::Div:
  case TokenType::Mod:
  case TokenType::BitwiseAnd:
  case TokenType::BitwiseOr:
  case TokenType::Xor:
  case TokenType::Lshift:
  case TokenType::Rshift:
  case TokenType::SkinnyArrow:
  default: break;
  }

  PANIC("Token::prec(): default assertion.");
}

END_NAMESPACE(n19);