# Lexer Subsystem

The lexer transforms preprocessed source text into a flat `std::vector<Token>`.
It operates in a single left-to-right pass over a `std::string`, producing
tokens with line/column locations that downstream parser and diagnostic code can
use directly.

This lexer primarily targets C-family input after preprocessing. It also knows
about a small profile-dependent C++ subset and a few synthetic pragma tokens
that originate from preserved preprocessor directives.

---

## Table of Contents

1. [Module Organization](#module-organization)
2. [Token Model](#token-model)
3. [The Lexer Class](#the-lexer-class)
4. [Scanning Loop](#scanning-loop)
5. [Whitespace, Comments, and Line Markers](#whitespace-comments-and-line-markers)
6. [Keyword and Profile Handling](#keyword-and-profile-handling)
7. [Literal Scanning](#literal-scanning)
8. [Pragmas as Synthetic Tokens](#pragmas-as-synthetic-tokens)
9. [Operator and Punctuation Scanning](#operator-and-punctuation-scanning)
10. [Invariants](#invariants)

---

## Module Organization

```txt
lexer/
  token.hpp  -- TokenKind enum, Token struct, keyword lookup declarations
  token.cpp  -- keyword classification and token-kind debug names
  lexer.hpp  -- Lexer class declaration
  lexer.cpp  -- scanning logic
```

`token.*` owns the token vocabulary. `lexer.*` owns the state machine that turns
source text into those tokens.

## Token Model

The token representation is intentionally simple:

```cpp
struct Token {
  TokenKind kind;
  int line;
  int column;
  TextId text_id;
  FileId file_id;
};
```

Every token carries:

- `kind`: the classified token category
- `line`, `column`: source position of the token start
- `text_id`: an append-only dictionary id for the token spelling
- `file_id`: an append-only dictionary id for the logical source file

`TokenKind` covers:

- literals (`IntLit`, `FloatLit`, `StrLit`, `CharLit`)
- identifiers
- C keywords
- C11 keywords
- GCC / GNU-style extension keywords
- a small C++-subset keyword set (`template`, `constexpr`, `consteval`)
- punctuation and operators
- synthetic pragma tokens consumed later by the parser
- `EndOfFile` and `Invalid`

## The Lexer Class

`c4c::Lexer` in `lexer.hpp` holds:

- `source_`: the full preprocessed source buffer
- `lex_profile_`: keyword/profile selection derived from `SourceProfile`
- `index_`: byte offset into `source_`
- `line_`, `column_`: current location counters
- pending pragma-token slots used when directive-like lines are converted into
  synthetic tokens during whitespace/comment skipping

The public surface is deliberately small:

- `Lexer(std::string source, LexProfile profile)`
- `std::vector<Token> scan_all()`

Everything else is an internal scanning helper.

## Scanning Loop

`scan_all()` repeatedly:

1. skips whitespace/comments/line markers
2. flushes any pending synthetic pragma token
3. emits `EndOfFile` when the input is exhausted
4. dispatches by current character:
   - digit or `.digit` -> number
   - identifier start -> identifier / keyword
   - `"` -> string literal
   - `'` -> character literal
   - otherwise -> punctuation/operator

This keeps the outer loop easy to reason about and pushes all tricky logic into
specialized scanners.

## Whitespace, Comments, and Line Markers

`skip_whitespace_and_comments()` does more than its name suggests. It consumes:

- ASCII whitespace
- `//` line comments
- `/* ... */` block comments
- preprocessor line-marker lines such as `# 123 "file.h"`

These line markers are expected in preprocessed text and should not become
ordinary `#` tokens. The lexer recognizes them only at logical start-of-line.

This same path also intercepts certain preserved pragma lines and turns them
into pending synthetic tokens instead of leaving them as raw punctuation.

## Keyword and Profile Handling

Identifier scanning is handled by `scan_identifier_or_keyword()`.

After collecting the raw spelling, the lexer calls `keyword_from_string(...)`,
which classifies the identifier according to:

- the raw spelling
- GNU-extension keyword support
- the active `LexProfile`

This is how the lexer can treat `template` / `constexpr` / `consteval` as
keywords only in profiles that allow that surface, while keeping plain C mode
more conservative.

The same code path also handles literal prefixes such as `L"..."`, `u"..."`,
`U"..."`, and `u8"..."` by folding the prefix into the subsequent string/char
literal token.

## Literal Scanning

### Numbers

`scan_number()` preserves the raw spelling in the shared text table and classifies the token only at the
high level of integer-vs-float:

- decimal integers and floats
- hex integers and hex floats
- octal forms
- binary integer literals as a GNU extension
- leading-dot floats such as `.5`
- integer and float suffixes
- optional imaginary suffix forms

The lexer does not fully normalize numeric values at this stage. It keeps the
original spelling so later phases can re-interpret the literal with richer type
context if needed.

### Strings and Characters

`scan_string()` and `scan_char()` preserve the original spelling, including
escape sequences. Unterminated literals become `Invalid`.

## Pragmas as Synthetic Tokens

The lexer has a small but important bridge role between preprocessor and parser.

When preserved preprocessor output contains these directive lines at
start-of-line:

- `#pragma pack(...)`
- `#pragma weak symbol`
- `#pragma GCC visibility ...`

the lexer converts them into structured token kinds:

- `PragmaPack`
- `PragmaWeak`
- `PragmaGccVisibility`

The directive payload is interned in the shared text table and referenced by
`Token::text_id`, for example:

- `"push,2"`
- `"pop"`
- `"hidden"`

This keeps pragma state out of the ordinary expression grammar while still
letting the parser react to it in a typed way.

## Operator and Punctuation Scanning

`scan_punct()` implements maximal-munch behavior for punctuation and operators.
That includes:

- single-character punctuation like `(`, `)`, `{`, `}`, `,`, `;`
- arithmetic and bitwise operators
- comparison operators
- compound assignment operators
- `...`, `->`, `<<=`, `>>=`, `&&`, `||`, and similar multi-character forms

The lexer does not emit generic "operator text" tokens. Every supported
punctuation/operator spelling maps directly to a dedicated `TokenKind`.

## Invariants

- The lexer consumes preprocessed text, not raw source with active macros.
- Token locations are tracked incrementally from the text buffer; callers should
  not rewrite the buffer underneath the lexer.
- Synthetic pragma handling only applies at logical start-of-line.
- `scan_all()` always appends one final `EndOfFile` token.
- Literal tokens preserve raw spellings in the shared text table rather than eagerly canonicalizing values.
