# Lightweight Error Recovery Plan

## Goal

We want a minimal error-recovery strategy for `c4cll` that is good enough to:

- keep parsing after common syntax/sema failures,
- avoid turning one real error into a flood of useless cascade errors,
- support a practical subset of Clang-style negative tests,
- stay small and maintainable instead of growing into a full Clang-like recovery engine.

This plan intentionally targets the lightweight path:

- panic-mode parser recovery,
- a small number of `invalid` placeholder nodes/types,
- sema short-circuiting on invalid inputs,
- test-runner support for `expected-error`,
- no attempt to fully emulate `lit`, `-verify`, or `FileCheck`.


## Non-Goals

This work does **not** try to:

- fully match Clang diagnostics,
- support `expected-warning` / `expected-note` as a hard requirement,
- support `RUN:` command parsing,
- support `FileCheck`,
- support every `@+N`, `@-N`, label, or note-chaining feature from Clang,
- preserve perfect AST quality after malformed input.

The target is much narrower: keep enough structure alive so we can continue and report multiple useful errors in one file.


## Why This Matters

Without recovery, external Clang negative tests are hard to use:

- the first parse error often stops meaningful progress,
- later `expected-error` comments become unreachable,
- diagnostics become noisy and unstable,
- the suite can only be used as crude `pass/fail`.

With lightweight recovery, we can move to:

- multiple useful errors per file,
- selective support for `verify`-style tests,
- better regression coverage for parser and sema.


## Strategy

### 1. Parser Uses Panic-Mode Recovery

When parsing fails, do not try to fully repair the program.

Instead:

- emit one primary diagnostic,
- discard tokens until a safe synchronization point,
- resume parsing from there.

Primary synchronization points:

- `;`
- `}`
- `)`
- `,`
- top-level declaration starters such as type/specifier keywords

This gives us cheap forward progress without building a heavy recovery system.


### 2. Introduce Minimal Invalid Placeholders

We should represent broken constructs explicitly instead of aborting whole subtrees.

Useful placeholder concepts:

- `InvalidExpr`
- `InvalidType`
- `InvalidDecl`
- possibly `InvalidStmt`

These only need to carry enough information for later stages to:

- recognize the node is already broken,
- avoid duplicate diagnostics,
- skip deeper processing safely.


### 3. Sema Short-Circuits On Invalid Inputs

If parser recovery creates invalid placeholders, sema should not keep digging.

Rules:

- if an operand/type/decl is invalid, return invalid immediately,
- do not emit follow-up errors that are obviously consequences of an earlier failure,
- keep walking sibling declarations/statements when safe.

This is the cheapest way to reduce cascade noise.


### 4. Cap Error Count

Set a hard maximum number of hard errors per translation unit.

Example:

- default max: `20`
- after limit is reached, stop with a final message

This prevents pathological files from producing noisy output and keeps recovery cheap.


## Minimal Recovery Points

The first implementation only needs a few high-value recovery hooks.

### A. Top-Level Declarations

If a declaration parse fails:

- report the error,
- skip to the next `;` or top-level declaration boundary,
- continue parsing the next declaration.

This is the most important recovery point for Clang-style conformance files.


### B. Statements In Blocks

If a statement parse fails:

- report the error,
- skip to `;` or `}`,
- continue with the next statement.

This helps negative tests that intentionally place one bad statement among many good ones.


### C. Parenthesized Lists

If parsing fails inside:

- parameter lists,
- argument lists,
- control-flow conditions,

then:

- skip to the matching `)` if possible,
- produce an invalid node,
- continue from the outer construct.

This is especially valuable because malformed parens often poison the rest of the file.


### D. Expression Parsing

If an expression cannot be completed:

- emit one diagnostic,
- return `InvalidExpr`,
- let the caller decide how far to resync.

Avoid repeatedly diagnosing the same token sequence.


### E. Type Parsing

If a type-specifier sequence is malformed:

- emit one diagnostic,
- return `InvalidType`,
- let declaration parsing continue where possible.


## Minimum Viable Alignment With Clang Negative Tests

We only want to align with the simplest and most useful subset of Clang-style negative tests.

### Supported Form

Support only:

- `expected-error {{message fragment}}`
- `expected-error@+N {{message fragment}}`
- `expected-error@-N {{message fragment}}`
- optional simple count forms like `expected-error 2{{...}}`

But only for **errors**.

We do **not** need warnings or notes as a first milestone.


### Matching Policy

The runner should:

- collect all hard errors emitted by `c4cll`,
- compare them against `expected-error` annotations,
- match on:
  - diagnostic kind = `error`
  - approximate source line
  - substring match on message text

Line matching can be tolerant at first:

- exact line match preferred,
- allow `±1` as a temporary compatibility rule if current line accounting is unstable.


### What We Need From The Compiler

For this to work reliably, compiler diagnostics should follow a stable format:

- `path:line:column: error: message`

If parse recovery uses fallback diagnostics, they should still try to include:

- line number,
- error severity,
- a consistent message.


## Recommended Test Scope

We should not try to ingest arbitrary Clang negative tests immediately.

Start with files that are:

- mostly independent errors,
- not heavily template-dependent,
- not dependent on Clang note chains,
- not dependent on exact wording of rich semantic diagnostics,
- not dependent on `FileCheck`,
- not dependent on `RUN:` variations.

Good early targets:

- simple declaration errors,
- scope/name lookup failures,
- bad initializer or arity cases,
- basic access-control or invalid-type cases where one main error is expected.

Avoid initially:

- tests with many `expected-note`,
- tests with dense `@label` references,
- tests relying on many distinct later-phase diagnostics,
- tests where one parser failure ruins the whole grammar context.


## Implementation Phases

### Phase 1. Stabilize Diagnostic Surface

- Ensure hard errors use a consistent `file:line:column: error: ...` shape.
- Keep parse-error messages deterministic.
- Add an error limit.

Success criterion:

- repeated runs on the same malformed file produce the same diagnostics.


### Phase 2. Add Minimal Invalid Nodes

- Introduce `InvalidExpr` and `InvalidType`.
- Optionally introduce `InvalidDecl` if needed by declaration recovery.

Success criterion:

- parser and sema can continue past a malformed subexpression or declaration without crashing or re-reporting endlessly.


### Phase 3. Add Synchronization Hooks

- top-level declaration recovery,
- statement recovery,
- paren-list recovery.

Success criterion:

- one malformed declaration/statement does not prevent later independent declarations from being checked.


### Phase 4. Negative Test Runner: Errors Only

- keep `pass` / `fail`,
- add `verify` for `expected-error`,
- support simple line offsets and counts,
- ignore warnings and notes for now.

Success criterion:

- a curated subset of external Clang negative tests can be checked meaningfully.


### Phase 5. Curate Allowlists

Split external negative tests into buckets:

- `fail`: compiler should reject, but no stable verify yet
- `verify`: compiler should reject and diagnostics are stable enough to match
- `defer`: recovery or wording too unstable right now

Success criterion:

- the external suite gives useful regression signal instead of noise.


## Practical Rules To Keep This Small

- Never try to fully repair syntax if skipping is enough.
- Prefer dropping broken subtrees over preserving rich malformed AST.
- Emit one primary error per broken region when possible.
- Stop semantic work early on invalid placeholders.
- Keep recovery local and explicit instead of adding global complexity.


## Definition Of Done For The First Version

The lightweight plan is successful when all of the following are true:

- `c4cll` can report multiple useful errors from one malformed file,
- one bad declaration does not destroy the whole translation unit,
- cascade diagnostics are noticeably reduced,
- a small curated subset of external Clang negative tests can run in `verify` mode,
- the implementation remains understandable and small.


## Suggested Next Step

After agreeing on this plan, the first coding task should be:

1. add a hard error limit,
2. add `InvalidExpr` / `InvalidType`,
3. add top-level declaration synchronization,
4. validate against a tiny curated `verify` allowlist.

That gives the highest payoff with the lowest complexity.
