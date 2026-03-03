#pragma once

// Recursive-descent parser for the tiny-c2ll C++ frontend.
//
// Entry point: Parser::parse() returns a NK_PROGRAM Node* containing all
// top-level declarations (functions, global variables, struct/enum defs).
//
// Design:
//  - All AST nodes and strings are allocated from the supplied Arena.
//  - typedef names are tracked in a std::set<std::string> for disambiguation.
//  - Anonymous struct/union/enum definitions are collected and prepended to
//    the program item list so the IR builder sees types before use sites.
//
// Pure-C backport note: replace class with struct + free functions.
// Replace std::set with a sorted char*[] searched by strcmp.

#include <set>
#include <string>
#include <vector>

#include "arena.hpp"
#include "ast.hpp"
#include "token.hpp"

namespace tinyc2ll::frontend_cxx {

class Parser {
 public:
  // All members public (required by project coding constraints).
  explicit Parser(std::vector<Token> tokens, Arena& arena);

  // Parse the entire token stream and return a NK_PROGRAM node.
  Node* parse();

  // ── public state ──────────────────────────────────────────────────────────
  std::vector<Token> tokens_;
  int                pos_;
  Arena&             arena_;
  std::set<std::string> typedefs_;  // known typedef names
  std::vector<Node*>    struct_defs_;  // collected struct/enum defs (prepended)
  int                anon_counter_;  // counter for anonymous tag names

  // ── token cursor helpers ──────────────────────────────────────────────────
  const Token& cur() const;              // current token
  const Token& peek(int offset = 0) const; // peek at pos_+offset (0=current)
  const Token& consume();                // consume and return current token
  bool at_end() const;
  bool check(TokenKind k) const;         // is current token kind k?
  bool check2(TokenKind k) const;        // is next token kind k?
  bool match(TokenKind k);               // consume if check(k)
  void expect(TokenKind k);              // consume or throw
  void skip_until(TokenKind k);          // skip tokens until k (consume k)

  // ── type parsing helpers ──────────────────────────────────────────────────
  bool is_type_start() const;            // can current token start a type?
  bool is_typedef_name(const std::string& s) const;

  // Parse a complete type specifier (base type + qualifiers).
  // ptr_level and array_size are NOT parsed here (those are in the declarator).
  TypeSpec parse_base_type();

  // Parse declarator suffix: pointer levels, array, etc.
  // Modifies ts in place.  Returns the declared name (into out_name).
  void parse_declarator(TypeSpec& ts, const char** out_name);

  // Parse a full type-name (type + declarator with no name): e.g. for sizeof.
  TypeSpec parse_type_name();

  // Skip __attribute__((...)) sequences (zero or more).
  void skip_attributes();

  // Skip __asm__("...") or asm("...") sequences.
  void skip_asm();

  // Skip a balanced paren group (consuming the closing paren).
  void skip_paren_group();

  // Skip a balanced brace group (consuming the closing brace).
  void skip_brace_group();

  // Parse struct/union body { fields... } or just a tag reference.
  // Returns a NK_STRUCT_DEF node; appends to struct_defs_ if new.
  Node* parse_struct_or_union(bool is_union);

  // Parse enum body { variants... } or just a tag reference.
  // Returns a NK_ENUM_DEF node; appends to struct_defs_ if new.
  Node* parse_enum();

  // ── parameter / field parsing ──────────────────────────────────────────────
  // Parse a single parameter declaration.  Returns NK_DECL node.
  Node* parse_param();

  // ── expression parsing (Pratt) ─────────────────────────────────────────────
  Node* parse_expr();            // comma-level
  Node* parse_assign_expr();     // assignment / ternary
  Node* parse_ternary();         // ternary ?:
  Node* parse_binary(int min_prec); // binary ops (precedence climbing)
  Node* parse_unary();           // prefix unary
  Node* parse_postfix(Node* base); // postfix ops (left-recursive)
  Node* parse_primary();         // primary expression

  // Operator precedence helper.
  static int bin_prec(TokenKind k);

  // ── statement parsing ──────────────────────────────────────────────────────
  Node* parse_stmt();
  Node* parse_block();           // { ... }

  // ── initializer parsing ───────────────────────────────────────────────────
  Node* parse_initializer();     // expr or { list }
  Node* parse_init_list();       // { item, item, ... }

  // ── declaration parsing ───────────────────────────────────────────────────
  // Parse a local declaration (inside function body).  May return a NK_BLOCK
  // if there are multiple declarators.
  Node* parse_local_decl();

  // ── top-level parsing ─────────────────────────────────────────────────────
  // Parse one top-level item. Returns nullptr on EOF.
  Node* parse_top_level();

  // ── node constructors ──────────────────────────────────────────────────────
  Node* make_node(NodeKind k, int line);
  Node* make_int_lit(long long v, int line);
  Node* make_float_lit(double v, const char* raw, int line);
  Node* make_str_lit(const char* raw, int line);
  Node* make_var(const char* name, int line);
  Node* make_binop(const char* op, Node* l, Node* r, int line);
  Node* make_unary(const char* op, Node* operand, int line);
  Node* make_assign(const char* op, Node* lhs, Node* rhs, int line);
  Node* make_block(Node** stmts, int n, int line);
};

}  // namespace tinyc2ll::frontend_cxx
