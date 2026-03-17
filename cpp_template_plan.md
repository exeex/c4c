# C++ Template Plan

## Goal

Build a C++ subset template system that treats templates as first-class decorator values instead of eager frontend-only syntax sugar.

Long-term target:

- `template` is represented as a higher-order entity:
  - `add : (typename T) -> fn(T, T) -> T`
- full instantiation is a policy decision, not a parser decision:
  - compile time
  - link time
  - runtime / JIT
- future JIT support should be possible by preserving HIR template bodies in an ELF section, while materialized code remains optional

Immediate target:

- finish the frontend and semantic groundwork so templates are preserved structurally through parser, canonical symbol/type, and later HIR

## Current Workspace Changes

The following files are currently modified in the worktree and are part of this template groundwork:

- [src/apps/c4cll.cpp](/workspaces/c4c/src/apps/c4cll.cpp)
- [src/frontend/parser/ast.hpp](/workspaces/c4c/src/frontend/parser/ast.hpp)
- [src/frontend/parser/parser.hpp](/workspaces/c4c/src/frontend/parser/parser.hpp)
- [src/frontend/parser/parse.cpp](/workspaces/c4c/src/frontend/parser/parse.cpp)
- [src/frontend/parser/declarations.cpp](/workspaces/c4c/src/frontend/parser/declarations.cpp)
- [src/frontend/parser/expressions.cpp](/workspaces/c4c/src/frontend/parser/expressions.cpp)
- [src/frontend/parser/statements.cpp](/workspaces/c4c/src/frontend/parser/statements.cpp)
- [src/frontend/parser/types.cpp](/workspaces/c4c/src/frontend/parser/types.cpp)
- [src/frontend/sema/canonical_symbol.hpp](/workspaces/c4c/src/frontend/sema/canonical_symbol.hpp)
- [src/frontend/sema/canonical_symbol.cpp](/workspaces/c4c/src/frontend/sema/canonical_symbol.cpp)

## What Has Been Completed

### 1. Parser now preserves C++ subset structure instead of discarding it

Implemented:

- source-profile-aware parser construction
- `template <typename T>` parsing at top level
- template parameter injection into the parser type namespace
- template-id parsing for calls such as `add<int>(...)`
- `constexpr`
- `consteval`
- `extern "C"`
- `if constexpr`
- C++ braced initialization in declaration contexts
- injected record type names for `.cpp` mode
- struct member function parsing skeleton

New AST metadata now records:

- template parameter names on declaration nodes
- template argument types on references
- `is_constexpr`
- `is_consteval`
- `linkage_spec`

### 2. Canonical symbol/type layer now models templates as decorators

Implemented in canonical sema:

- `CanonicalTemplateParam`
- `template_params` on `CanonicalSymbol`
- template-argument substitution API:
  - `substitute_template_args(...)`
- symbol application API:
  - `instantiate_symbol(...)`

This supports:

- unapplied template decorators
- partial application
- full application

In other words:

- template declaration can stay as a higher-order entity
- partial specialization can be represented as a symbol with remaining template params
- delayed instantiation is now structurally possible

### 3. Canonical dumps now show the intended model

Current `--dump-canonical` output already shows the desired direction.

Example:

```text
fn add : consteval (typename T) -> fn(typedef 'T')(typedef 'T', typedef 'T')
```

This is not the final pretty-printer shape, but it already captures the correct semantics:

- `add` is a template decorator
- `T` is still abstract
- the function type is preserved underneath

`extern "C"` also now flows into canonical linkage.

## Current Validation Status

Parser validation on `tests/internal/cpp/postive_case/*.cpp`:

- `consteval_func.cpp`: parse OK
- `constexpr_if.cpp`: parse OK
- `constexpr_var.cpp`: parse OK
- `extern_c.cpp`: parse OK
- `struct_method.cpp`: parse OK
- `template_func.cpp`: parse OK

Full compile status after parser/canonical changes:

- `consteval_func.cpp`: compiles and runs
- `constexpr_if.cpp`: compiles and runs
- `extern_c.cpp`: compiles and runs
- `template_func.cpp`: compiles and runs
- `constexpr_var.cpp`: compiles, but runtime result is still wrong because constexpr/global constant folding is incomplete
- `struct_method.cpp`: still fails during lowering/codegen because member-function lookup/call lowering is not implemented yet

## Important Architectural Decision

We should not model templates as “always consteval”.

Better model:

- template is a decorator value
- `consteval` is one possible instantiation policy
- other future policies include:
  - link-time materialization
  - runtime / JIT materialization

That means:

- parser should preserve the structure
- canonical layer should preserve application state
- HIR must eventually gain explicit template entities and template application nodes
- materialization must be a later stage decision

Related decision for the current consteval work:

- sema may do light, local constant folding and diagnostics
- but full template instantiation must not be forced in sema
- HIR should become the first stage that can preserve:
  - template function definitions
  - template function application requests
  - compile-time decorator/application nodes
- eager compile-time materialization should be one policy, not the only meaning

In particular, template arguments may themselves depend on consteval results.
That means a fixed frontend order like:

- template instantiate
- then consteval

is too rigid for the long-term design.

The more correct long-term model is:

- preserve template/application structure into HIR
- run an iterative compile-time reduction loop in HIR:
  - template instantiate
  - consteval
  - template instantiate
  - consteval
  - ...
- stop only when no additional compile-time-only nodes can be reduced

This keeps the door open for:

- delayed specialization
- link-time materialization
- runtime / JIT specialization

## Near-Term Work Needed

### A. Finish semantic correctness for current `.cpp` cases

1. `constexpr` variable evaluation

- evaluate global `constexpr` initializers using existing consteval/constant-expression machinery
- materialize folded values into global initializers
- fix `constexpr_var.cpp` so `answer` becomes `42`, not `0`

2. Struct member function support beyond parsing

- carry struct methods into HIR instead of dropping them
- define method lookup rules
- support `counter.get()` lowering
- decide whether methods lower as:
  - namespaced free functions with an explicit `this`
  - record-scoped functions with implicit `this`

3. Template-aware call resolution

- current parser preserves `add<int>`
- sema/HIR still need to interpret template application, not just ignore it

### B. Strengthen canonical template model

1. Add canonical representation for template application requests

Needed concepts:

- template declaration identity
- applied argument list
- remaining unapplied parameter list

2. Define specialization identity

Specialization key should eventually be stable across TUs.

Likely pieces:

- canonical template symbol identity
- canonicalized template args
- source/body version fingerprint

This identity will later be reusable for:

- link-time instantiation
- specialization deduplication
- future JIT caches

3. Distinguish declaration vs application vs materialized entity

We should conceptually separate:

- template definition
- template application request
- materialized function entity

This distinction should survive into HIR.

## HIR Work That Should Follow

Even though JIT is not the immediate target, the HIR must be designed now so we do not paint ourselves into a compile-time-only corner.

Recommended future HIR entities:

- `TemplateDef`
- `TemplateApply`
- `TemplateMaterialize`

Intent:

- `TemplateDef`: stores the template body and its parameter environment
- `TemplateApply`: represents partial/full application such as `add<int>`
- `TemplateMaterialize`: forces emission according to policy

Materialization policy should remain separate from the template entity itself.

Near-term representation shortcut:

- before introducing brand new HIR node families, we can model template functions
  in a decorator-shaped form that reuses existing callable-node conventions
- for example:
  - `template<typename T> T add(T, T);`
  - interpret as:
    - `add` = compile-time decorator
    - `add<T>` = specialized callable produced by decorator application

Informally:

```text
consteval add(typename T) -> (add<T>(T, T) -> T)
```

This is not the final syntax, but it captures the intended semantics:

- `add` itself is compile-time-only
- applying template args produces a specialized callable value
- the specialized callable can then be lowered or materialized later
- existing node/call machinery can be reused where practical

Possible policies later:

- eager compile-time
- lazy link-time
- lazy runtime/JIT

## Link-Time / JIT-Oriented Constraints To Preserve

These constraints should guide current work even before HIR template nodes are implemented:

1. Do not eagerly erase template structure in frontend lowering

- once erased, link-time or runtime materialization becomes much harder

2. Keep template application canonical and serializable

- needed for object metadata, linker plugins, or ELF sidecar sections

3. Avoid tying template semantics to AST-only constructs

- HIR must be rich enough to survive serialization

4. Keep specialization keys deterministic

- required for deduplication across translation units

5. Treat code generation as optional materialization

- template definition itself is not code yet

## Suggested Execution Order

### Phase 1: finish current frontend correctness without forcing eager template erasure

- constexpr global folding
- struct member-function lowering
- template call interpretation for existing positive cases
- keep sema-side consteval limited to simple local folding / diagnostics

### Phase 2: make HIR preserve template function and template call structure

- represent template function definitions in HIR
- represent template application requests in HIR
- keep template decorators unapplied until a later reduction/materialization pass

### Phase 3: run compile-time reduction as a fixpoint in HIR

- template instantiate
- consteval
- template instantiate
- consteval
- continue until convergence

### Phase 4: define explicit HIR template nodes cleanly

- `TemplateDef`
- `TemplateApply`
- `TemplateMaterialize`

### Phase 5: specialization identity and dedup

- canonical specialization key
- cross-TU stable encoding


## Notes On This Iteration

What this iteration intentionally did not complete:

- no true template instantiation in sema or HIR yet
- no HIR-level template-preserving representation yet
- no member-function lowering yet
- no constexpr-global folding completion yet
- no mangled-name integration for template specializations yet
- no link-time or JIT pipeline work yet

What this iteration achieved:

- parser no longer destroys the information we need
- canonical layer now has a decorator-shaped template model
- partial application can now be represented conceptually instead of being bolted on later

This is the right first step for a future link-time/JIT-friendly template system.
