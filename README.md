# tiny-c2ll

Super tiny C-subset compiler that outputs LLVM IR text without linking LLVM libraries.
Uses Python `match/case`, so run with Python 3.10+ (tested with 3.14).

## Supported syntax

- One function: `int <name>() { ... }`
- Statements:
  - `int x;`
  - `int x = expr;`
  - `x = expr;`
  - `return expr;`
- Expressions:
  - integer literal, variable
  - `+ - * /`
  - parentheses

## Usage

```bash
python3.14 c2ll.py example.c -o out.ll
cat out.ll
```

Run generated IR with clang:

```bash
clang out.ll -o out
./out
echo $?
```

`echo $?` prints the C `main` return value.

## Pipeline

- Lexer: source text -> token stream
- Parser: token stream -> AST
- SemanticAnalyzer: validates declarations, assignments, and uninitialized reads
- IRBuilder: AST -> LLVM IR text

## Next

- Language roadmap and pattern-matching design: `SPEC.md`
