# Plan Todo

## Goal

Implement the lightweight error-recovery plan from `plan.md` so `c4cll` can:

- continue after common parse/sema failures,
- reduce cascade diagnostics,
- report multiple useful errors per malformed file,
- support a small practical subset of `expected-error` style negative tests.

Keep the implementation intentionally small:

- panic-mode parser recovery,
- minimal invalid placeholders,
- sema short-circuiting on invalid inputs,
- error-only verify support,
- no attempt to fully emulate Clang `lit`, `-verify`, or `FileCheck`.

## Scope Guardrails

Do:

- keep diagnostics deterministic,
- recover at a few high-value synchronization points,
- add only the minimum invalid node/state needed,
- curate a small external negative-test subset.

Do not do in this plan:

- full Clang diagnostic compatibility,
- `expected-warning` / `expected-note` as required features,
- `RUN:` parsing,
- `FileCheck`,
- rich malformed AST preservation,
- broad support for labels or complex line-annotation syntax.

## Current Status

Not started for this plan. Existing `plan_todo.md` content was for an older HIR frontend refactor and has been replaced.

## Phase 1: Stabilize Diagnostic Surface

### Objective

Make malformed-input diagnostics stable enough to support recovery work and future `verify` matching.

### Tasks

1. enforce a consistent hard-error format

- ensure hard errors print as `path:line:column: error: message`
- make parse-error wording deterministic where currently unstable

2. add a translation-unit error cap

- introduce a default hard limit such as `20`
- stop further error emission cleanly after the limit is reached
- emit one final "too many errors" style message if needed

3. validate stability

- run repeated malformed-input cases
- confirm output is stable across repeated runs

### Exit Criteria

- malformed files produce deterministic hard errors
- error count is capped
- diagnostics are stable enough for substring-based matching

## Phase 2: Add Minimal Invalid Placeholders

### Objective

Represent broken constructs explicitly so parser and sema can keep moving without cascading repeatedly.

### Tasks

1. introduce invalid expression/type placeholders

- add `InvalidExpr`
- add `InvalidType`

2. add optional declaration/statement placeholders only if needed

- add `InvalidDecl` only if declaration recovery cannot stay simple otherwise
- add `InvalidStmt` only if block recovery clearly benefits

3. short-circuit semantic processing

- return invalid immediately when any required operand/type/decl is invalid
- suppress obvious follow-on diagnostics from already-broken inputs

### Exit Criteria

- malformed subexpressions and malformed type sequences no longer crash later stages
- sema stops digging into already-invalid inputs
- duplicate/cascade diagnostics are reduced

## Phase 3: Add Synchronization Hooks

### Objective

Recover cheaply at a few high-value parser boundaries.

### Tasks

1. top-level declaration recovery

- on declaration parse failure, emit one primary error
- skip to `;` or the next top-level declaration boundary
- continue parsing the next declaration

2. statement recovery inside blocks

- on statement parse failure, skip to `;` or `}`
- continue with the next statement

3. parenthesized-list recovery

- recover inside parameter lists, argument lists, and control-flow conditions
- skip to the matching `)` where possible
- return an invalid placeholder to the caller

4. expression/type recovery handoff rules

- expression parsing emits once and returns `InvalidExpr`
- type parsing emits once and returns `InvalidType`
- callers own larger resynchronization decisions

### Exit Criteria

- one malformed declaration does not prevent later independent declarations from being checked
- one bad statement does not poison the rest of a block
- malformed parens no longer derail the rest of the file as often

## Phase 4: Negative Test Runner (Errors Only)

### Objective

Add a minimal `verify` mode for stable `expected-error` checks.

### Tasks

1. support a narrow annotation subset

- `expected-error {{message fragment}}`
- `expected-error@+N {{message fragment}}`
- `expected-error@-N {{message fragment}}`
- simple count forms such as `expected-error 2{{...}}` if low-cost

2. implement matching

- collect hard errors from `c4cll`
- match by kind = `error`
- match by approximate source line
- match by substring on message text

3. keep line matching pragmatic

- prefer exact line match
- temporarily allow a small tolerance such as `+-1` if current accounting is not fully stable

4. preserve existing simple result modes

- keep plain `pass` / `fail`
- add `verify` without expanding into warnings/notes yet

### Exit Criteria

- a curated subset of negative tests can be checked in `verify` mode
- verify results are useful without requiring full Clang parity

## Phase 5: Curate Test Buckets

### Objective

Turn external negative tests into actionable regression coverage instead of noisy bulk failures.

### Tasks

1. create allowlist buckets

- `fail`: rejection expected, diagnostics not stable enough for verify
- `verify`: rejection expected and diagnostics stable enough to match
- `defer`: currently too recovery-sensitive or wording-sensitive

2. choose good early targets

- simple declaration errors
- scope/name lookup failures
- bad initializer or arity cases
- basic invalid-type or access-control style cases with one main error

3. exclude poor early targets

- tests relying on many `expected-note`
- tests with dense labels or complex annotation patterns
- tests where one parser failure destroys the surrounding grammar

### Exit Criteria

- external negative coverage produces clear signal
- unstable tests are isolated instead of obscuring regressions

## Suggested Implementation Order

1. add a hard error limit
2. add `InvalidExpr` / `InvalidType`
3. add top-level declaration synchronization
4. validate with a tiny curated `verify` allowlist
5. expand to statement and paren-list recovery

## Definition Of Done

This plan is complete when:

- `c4cll` reports multiple useful errors from one malformed file,
- one bad declaration no longer destroys the whole translation unit,
- cascade diagnostics are noticeably reduced,
- a small curated subset of external Clang negative tests runs in `verify` mode,
- the implementation remains small and understandable.
