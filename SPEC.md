# C+PM Language Spec v0 (Python POC)

This spec defines a minimal language for proving feasibility of:
- C-like core syntax
- C++-style pattern matching semantics (subset)
- Lowering to simple LLVM IR without importing LLVM libraries

The scope is intentionally small for rapid iteration and self-hosting preparation.

## 1. Goals

- Build a working compiler in Python first.
- Keep runtime model C-like and predictable.
- Add pattern matching as a compile-time language feature.
- Lower pattern matching to plain control flow (`if`/`switch`-style CFG).
- Avoid dependence on table-driven generators for this phase.

## 2. Non-goals (v0)

- Full C standard compliance.
- Full C++ support (templates, exceptions, RTTI, overloading, etc.).
- Exhaustiveness proofs over open-world types.
- Optimization-heavy backend.

## 3. Source Model

Program is a list of function definitions.

Function shape in v0:
- `int name() { ... }`

Future v1 may add parameters and user-defined tagged unions.

## 4. Lexical Tokens

Required token classes:
- Keywords: `int`, `return`, `match`, `case`, `default`, `if`, `else`
- Identifiers
- Integer literals
- Symbols: `{ } ( ) ; , : = + - * / _`

`_` is reserved as wildcard pattern.

## 5. AST Model

Core nodes:
- `Program(functions)`
- `Function(name, body)`
- Statements:
  - `Decl(name, init?)`
  - `Assign(name, expr)`
  - `Return(expr)`
  - `Match(scrutinee, arms, default?)`
- Expressions:
  - `IntLit(value)`
  - `Var(name)`
  - `BinOp(op, lhs, rhs)`

Pattern nodes:
- `PWildcard` => `_`
- `PInt(value)` => integer literal pattern
- `PBind(name)` => name binding
- `PTag(tag, subpatterns...)` (reserved; optional in v0 parser)

## 6. Types (v0)

- Only `int` values.
- All expression types must be `int`.
- `match` scrutinee in v0 must be `int`.

## 7. Semantics

### 7.1 Declarations and Assignment

- Variables must be declared before use.
- Read-before-initialization is an error.
- Redeclaration in the same scope is an error.

### 7.2 Pattern Matching

Syntax (v0):

```c
match (expr) {
  case PATTERN: stmt
  case PATTERN: stmt
  default: stmt
}
```

Rules:
- Arms are checked top-to-bottom (first match wins).
- `PInt(k)` matches when `scrutinee == k`.
- `_` always matches.
- `PBind(x)` always matches and binds `x = scrutinee` for that arm scope.
- `default` is optional but recommended.
- If no arm matches and no `default`, compile-time error in v0.

Scope:
- Bindings created in one arm do not leak outside that arm.

## 8. Lowering Strategy to LLVM IR

`match` lowers into explicit basic blocks:

1. Evaluate scrutinee once into temp `%s`.
2. For each arm:
   - generate condition block
   - compare `%s` with constant for `PInt`
   - unconditional true for `_` or `PBind`
3. Branch to first matching arm block.
4. If no match:
   - go to `default` block, or
   - emit compile-time error in v0.

`PBind(x)` lowering:
- allocate local slot `%x`
- `store %s -> %x` at arm entry

No PHI requirement in v0 if each arm ends in `return`.
If arm fallthrough is allowed later, introduce join block + PHI or stack slot.

## 9. Diagnostics

Every error should include:
- phase (`lex`, `parse`, `sema`, `codegen`)
- line/column when available
- actionable message

Examples:
- `sema 6:10: use of uninitialized variable 'a'`
- `parse 12:5: expected ':' after case pattern`

## 10. Python POC Milestones

M0 (done now):
- lexer/parser/sema/codegen for simple C subset without `match`

M1:
- add `match/case/default` tokens and AST nodes
- support patterns: `PInt`, `_`, `PBind`
- lower to LLVM IR blocks

M2:
- arm-local scope and binding lifetime checks
- better diagnostics and parser recovery

M3:
- function parameters + multiple functions
- optional `PTag(...)` pattern to model tablegen-like rules

## 11. Tablegen Replacement Direction (Design Intent)

Long-term intent:
- encode rule selection in pattern matching arms
- move static data tables into `.h`-style declarations
- keep matching semantics in language, not external generator files

This spec does not yet claim one-to-one compatibility with existing `.td/.tb`.

## 12. DAG Core Model (New)

Pattern matching for instruction selection operates on a dedicated typed DAG IR, not on arbitrary host-language objects.

### 12.1 Core Types

- `dag<T>`: a DAG value whose root result type is `T`
- `node_id`: opaque identity of a DAG node
- `opcode`: symbolic operation code (e.g., `ADD`, `MUL`, `CONST`)
- `attr`: typed metadata (e.g., immediates, flags)

### 12.2 Built-in Node Schema

Each DAG node has a fixed schema:
- `id: node_id`
- `op: opcode`
- `results: [type]`
- `operands: [node_id/result_index]`
- `attrs: map<string, typed-literal>`
- `effects: pure | load | store | call | barrier`

No target is allowed to redefine this schema; target-specific information must fit in `opcode`/`attrs` under verifier rules.

## 13. DAG Declarations and Construction (New)

The language introduces explicit DAG declaration and constructor forms.

Example:

```c
dag<i32> n0 = dag.const<i32>(42);
dag<i32> n1 = dag.add(n0, dag.const<i32>(1));
```

Proposed minimal declarations:
- `dag<T> name = dag.<op>(args...)`
- `dag.match(root) { ... }`
- `dag.rewrite(...)`

## 14. Pattern/Rewrite Syntax (New)

Pattern matching for DAG uses a dedicated construct:

```c
dag.match(root) {
  case ADD(x, MUL(y, CONST(4))) where is_power_of_two(4) cost 1:
    rewrite SHL(y, CONST(2));
  case ADD(x, y) cost 10:
    rewrite ADD(x, y);
  default:
    rewrite root;
}
```

Semantics:
- Top-to-bottom first-match wins, then cost tie-break for same structural class if enabled.
- `x`, `y` are pattern binders to matched DAG subnodes.
- `where` adds boolean guard predicates.
- `rewrite` constructs replacement DAG.

Verifier requirements:
- Type-consistent rewrite output.
- Effect-safe rewrite (cannot reorder side-effecting nodes unless marked legal).
- All referenced binders must be bound in the matched pattern.

## 15. Template-like Surface Syntax (New)

Template syntax is treated as pure sugar over DAG rewrite rules.

Example surface form:

```c
template<dag<i32> A, dag<i32> B>
rule add_mul_to_shl
  : ADD(A, MUL(B, CONST(4)))
  where is_legal_shl(B)
  cost 1
{
  emit SHL(B, CONST(2));
}
```

Lowered internal rule form (conceptual):

```text
Rule {
  name: "add_mul_to_shl",
  pattern: ADD($A, MUL($B, CONST(4))),
  guard: is_legal_shl($B),
  cost: 1,
  rewrite: SHL($B, CONST(2))
}
```

Design rule:
- Template parameters become typed pattern binders.
- `emit` lowers to `rewrite`.
- No Turing-complete template instantiation in matcher core.

## 16. Lowering Pipeline Extension (New)

Extended pipeline:
- Source parse
- AST semantic check
- DAG region extraction/build
- Pattern normalization
- Template-sugar desugaring to `Rule` IR
- Rule verification (type/effect/overlap diagnostics)
- Matcher generation or direct interpreter
- DAG rewrite execution
- Final LLVM IR emission

This keeps front-end ergonomics while preserving a strict, analyzable DAG rewrite core.
