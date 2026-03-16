# Canonical Type Refactor Plan

## Status

The main canonical-type refactor is complete.

Completed outcomes:

- `CanonicalType` is the sema-owned recursive type model
- `ResolvedTypeTable` exists and is threaded into lowering
- top-level declarations, globals, params, and nominal type defs are canonicalized
- callable/prototype helpers read canonical signatures when available
- canonical symbol identity and mangling are now driven by canonical type data
- the current baseline test suite passes at `1794/1794`

The remaining work is cleanup and gap-closing so the old post-sema `TypeSpec` fallback paths can be removed safely.

## Cleanup Scope

### 1. Parser gap: nested function-returning-function-pointer declarators

Known limitation:

- raw declarators like `int (*(*f())())(int)` are not fully captured by the parser
- canonicalization inherits that parser gap because the function node does not always carry the full pointed-to signature

Current test coverage:

- `tests/internal/positive_case/ok_fn_returns_fn_ptr.c`
- `tests/internal/positive_case/ok_fn_returns_variadic_fn_ptr.c`
- `tests/internal/positive_case/ccc_review/0009_nested_fn_returning_fn_ptr.c` is an expected-fail guard for the still-broken nested raw syntax

Exit condition:

- parser preserves complete fn-ptr return signature metadata for nested declarators
- the `ccc_review` XFAIL case can be flipped to a normal passing positive test

### 2. Local declaration canonical tracking

Known limitation:

- `ResolvedTypeTable` does not yet record function-body `NK_DECL` nodes
- local function-pointer declarations therefore still fall back to legacy parser-field recovery in parts of lowering

Exit condition:

- every local declaration that has a resolved semantic type is recorded in `ResolvedTypeTable`
- local fn-ptr lowering uses canonical signatures without parser-field fallback

### 3. Expression-level canonical tracking

Known limitation:

- canonical types are recorded for declaration-like nodes, not for arbitrary expressions
- indirect calls through deref/cast/conditional/member expressions still require partial `TypeSpec` reconstruction

Priority expression forms:

- `NK_VAR`
- `NK_DEREF`
- `NK_ADDR`
- `NK_CAST`
- `NK_MEMBER`
- conditional expressions
- call expressions whose result type is itself callable

Exit condition:

- lowering/codegen can ask sema for canonical expression type directly
- indirect-call result/prototype logic no longer peels `ptr_level` / `is_fn_ptr`

### 4. Legacy fallback removal

Known limitation:

- codegen still retains `TypeSpec`-based return/prototype recovery for cases where canonical data is absent
- this is intentional migration debt, not desired end state

Exit condition:

- all callable signature and call-result recovery paths use canonical data
- legacy `TypeSpec` peeling paths are deleted

## Closing Phases

### Phase A: Lock Current Behavior With Tests

Scope:

- keep passing coverage for simple fn-ptr return and variadic fn-ptr return
- keep an explicit XFAIL for nested raw declarator syntax until parser support lands

Deliverables:

- stable internal regression tests for the current parser/canonical boundary

### Phase B: Fill `ResolvedTypeTable` For Locals

Scope:

- record canonical types for local `NK_DECL`
- use canonical sigs for local fn-ptr declarations in lowering

Deliverables:

- local declaration lookups succeed through canonical path
- fewer fallback hits in `fn_ptr_sig_from_decl_node()`

### Phase C: Add Expression Canonical Types

Scope:

- extend sema/type resolution to annotate expression nodes with canonical type
- thread those results to lowering/codegen consumers

Deliverables:

- indirect calls through expression forms can read canonical callable signatures directly
- expression type consumers stop reconstructing callable structure from parser fields

### Phase D: Delete Legacy Callable Recovery

Scope:

- remove `TypeSpec`-based call-result and signature peeling where canonical data is available everywhere relevant
- keep parser-only `TypeSpec` responsibilities, but delete downstream semantic duplication

Deliverables:

- one semantic callable-type path after sema
- smaller and less fragile indirect-call lowering/codegen logic

## Post-Plan Baseline Fixes

These were completed after the main canonical refactor and now form part of the expected baseline:

- sema now accepts bare `return;` in non-void functions where C89 compatibility requires it
- asm output operands now mark variables initialized, avoiding false uninitialized-read reports
- float vector binary ops now use float LLVM ops, and scalar-to-vector splatting is fixed in the early vector path
- zero-initialized complex arrays now emit valid `zeroinitializer`
- `__builtin_prefetch` is implemented as a no-op with argument evaluation
- `__builtin_clrsb`, `clrsbl`, and `clrsbll` are implemented
- plain `alloca()` is intercepted as an implicit builtin like `__builtin_alloca`
- the external allowlist baseline expanded from `1784` passing tests to `1794`

## Out-Of-Scope Follow-Ups

These are not part of the canonical cleanup phases, but remain good follow-up targets:

- pointer-to-array global initializers such as `&arr[0]` in global init
- fp128 literal pipeline correctness on aarch64, including `__LDBL_MAX__`
- unsupported or partially supported builtins beyond the fixes already landed

## Deferred

Deferred items that should stay out of this cleanup slice:

- substitution compression in mangled names (`S0_`, `S1_`, etc.)
- template argument mangling (`I...E`)
- reference type mangling (`R`, `O`)
- member function qualifiers

## Non-Goals For This Cleanup Slice

- full C++ semantic support
- templates or overload resolution
- replacing parser-facing `TypeSpec`
- final ABI-complete C++ surface area beyond the canonical/mangling groundwork already in place

## Success Criteria

The cleanup phase is done when all of the following are true:

- nested raw fn-ptr return declarators parse and lower correctly
- local declarations are present in `ResolvedTypeTable`
- expression nodes needed by indirect calls have canonical types
- legacy callable `TypeSpec` recovery code can be removed without test loss

## Guardrails

The following should remain true during cleanup:

- no new major feature should deepen dependence on flattened `TypeSpec` flags if canonical type can carry the same meaning
- no lowering stage should need to reconstruct prototypes from multiple AST fields once canonical type is available
- no C++ mangling work should proceed on top of parser-only type identity
- no second independent semantic type system should be introduced beside canonical type

## Design Rule

The core rule remains:

- parser describes how a type was written
- sema defines what the type is

If later stages still need to reconstruct semantic callable identity from parser-only fragments after this cleanup, the structure is still incomplete and should be corrected before further C++ feature work continues.
