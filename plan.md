# Implementation Plan

## Goal

Implement template and consteval support in an order that preserves long-term flexibility for:

- delayed template instantiation
- HIR-level compile-time reduction
- future JIT specialization

The key decision is:

- sema may do conservative early folding and diagnostics
- but HIR must own template preservation, template instantiation, and the full compile-time reduction loop

## Core Model

Template functions should not be treated as eager frontend syntax sugar.

They should be preserved as compile-time decorator-like entities.

Example:

```cpp
template<typename T>
T add(T, T);
```

Conceptually:

```text
consteval add(typename T) -> (add<T>(T, T) -> T)
```

Interpretation:

- `add` is a compile-time decorator
- `add<T>` is a specialized callable produced by applying template arguments
- specialization/materialization is a later policy decision

## Required Ordering

For the long-term architecture, we should not hardcode:

- template instantiate
- then consteval

because template arguments may themselves depend on consteval results.

Instead, the full compile-time pipeline should converge through iteration in HIR:

1. preserve template and consteval structure into HIR
2. instantiate templates that are ready
3. reduce consteval calls that are ready
4. repeat instantiate/reduce until no additional compile-time nodes can be eliminated

So the intended steady-state HIR loop is:

```text
template instantiate -> consteval -> template instantiate -> consteval -> ...
```

until convergence.

## Division Of Responsibility

### Sema

Sema is allowed to do:

- legality diagnostics
- local constant folding
- simple non-template consteval folding
- preservation of template and compile-time shape

Sema must not become the final owner of:

- full template instantiation
- final template materialization
- erasing template application structure needed by HIR/JIT

### HIR

HIR must eventually own:

- template function preservation
- template function call/application preservation
- instantiation scheduling
- consteval reduction scheduling
- fixpoint iteration until compile-time convergence
- later materialization policy

## Implementation Sequence

### Phase 1: constrain sema to conservative compile-time work

Goals:

- keep sema useful for diagnostics and simple folding
- avoid baking eager template materialization into sema

Tasks:

- keep simple non-template consteval folding in sema only where it is obviously safe
- keep diagnostics in sema for:
  - invalid consteval usage
  - non-constant immediate arguments
  - address-of consteval function
- do not require sema to fully instantiate template functions
- do not rewrite away template application structure that HIR will need

Exit criteria:

- sema can still reject invalid consteval uses early
- sema does not become the only place where template+consteval semantics work

### Phase 2: make HIR preserve template function definitions

Goals:

- lower template functions into HIR without immediately erasing them into concrete functions

Tasks:

- add a HIR representation for template function definitions
- preserve template parameter environment on the HIR-side definition
- keep the function body available for later compile-time reduction/materialization

Exit criteria:

- template definitions survive lowering as first-class HIR entities
- template bodies remain available after lowering

### Phase 3: make HIR preserve template function application/calls

Goals:

- distinguish template application from ordinary function call

Tasks:

- add a HIR representation for template application requests such as `add<int>`
- ensure `add<int>(1, 2)` is not flattened into an ordinary call too early
- represent specialized callable values distinctly from plain function symbols

Exit criteria:

- HIR can represent:
  - template definition
  - template application
  - specialized callable request

### Phase 4: represent consteval as HIR-reducible compile-time nodes

Goals:

- stop treating consteval reduction as a lowering-only side effect

Tasks:

- preserve consteval call intent in HIR
- attach enough environment information for later reduction:
  - constant arguments
  - template bindings
  - enclosing compile-time context
- keep reduction failures structured enough for diagnostics

Exit criteria:

- consteval is representable in HIR before final reduction
- HIR can defer consteval when inputs are not ready yet

### Phase 5: add HIR compile-time reduction loop

Goals:

- make compile-time behavior converge through repeated passes instead of fixed frontend ordering

Tasks:

- implement a worklist or fixpoint pass in HIR
- repeatedly run:
  - template instantiation for newly-ready applications
  - consteval reduction for newly-ready immediate calls
- continue until no new compile-time nodes are reducible

Exit criteria:

- template arguments produced by consteval can unlock later instantiations
- instantiated templates can expose new consteval calls
- the system converges without frontend eager erasure

### Phase 6: define materialization boundary

Goals:

- separate compile-time specialization from code emission

Tasks:

- decide when a specialized callable becomes a concrete emitted function
- keep that decision separate from:
  - template definition semantics
  - template application semantics
  - consteval semantics

Exit criteria:

- concrete codegen is a policy step
- template semantics remain valid even if emission is delayed to link-time/JIT

### Phase 7: specialization identity and caching

Goals:

- make later link-time/JIT reuse deterministic

Tasks:

- define a stable specialization key
- ensure template application encoding is deterministic
- preserve enough identity for:
  - cross-TU dedup
  - lazy materialization
  - future JIT caches

Exit criteria:

- specialization identity is stable and serializable

## Recommended Transitional Strategy

Before introducing fully separate HIR node families, we may use a simpler transitional model:

- treat template definitions as compile-time decorator-shaped callable entities
- treat `add<T>` as decorator application producing a specialized callable
- reuse existing callable/call node machinery where practical

This is acceptable as a transition as long as:

- the semantics are documented clearly
- template structure is still preserved
- later migration to explicit HIR nodes remains possible

## Final Success Criteria

The implementation is on the right track when:

- sema only performs conservative early compile-time work
- HIR preserves template function definitions and template applications
- template instantiation is no longer forced entirely in sema/frontend lowering
- consteval reduction can interact with template instantiation through HIR iteration
- compile-time-only structure can remain intact long enough for future JIT specialization

## Checklist

### Phase 1 Checklist: constrain sema

- [ ] Audit current consteval entry points in:
  - `src/frontend/sema/sema.cpp`
  - `src/frontend/sema/consteval.cpp`
  - `src/frontend/hir/ast_to_hir.cpp`
- [ ] Document which existing consteval folds are:
  - safe to keep in sema
  - required to move to HIR later
- [ ] Keep sema-side diagnostics working for:
  - non-constant consteval arguments
  - invalid consteval declarations
  - address-of consteval function
- [ ] Avoid adding new sema logic that eagerly erases template application structure

### Phase 2 Checklist: preserve template definitions in HIR

- [ ] Add or reserve a HIR representation for template function definitions
- [ ] Ensure template parameter names/bindings survive lowering
- [ ] Ensure template function bodies remain available after lowering
- [ ] Stop lowering template functions only as concrete emitted functions
- [ ] Add printer/debug output so preserved template definitions are inspectable

Files likely involved:

- `src/frontend/hir/ir.hpp`
- `src/frontend/hir/ast_to_hir.cpp`
- `src/frontend/hir/hir_printer.cpp`

### Phase 3 Checklist: preserve template applications/calls in HIR

- [ ] Add or reserve a HIR representation for template application requests
- [ ] Distinguish:
  - plain function call
  - template application
  - call of a specialized callable
- [ ] Preserve call-site template arguments in HIR
- [ ] Avoid flattening `add<int>(...)` into an ordinary call too early
- [ ] Add printer/debug output for template applications

Files likely involved:

- `src/frontend/hir/ir.hpp`
- `src/frontend/hir/ast_to_hir.cpp`
- `src/frontend/hir/hir_printer.cpp`

### Phase 4 Checklist: preserve consteval as HIR-reducible nodes

- [ ] Define how consteval call intent appears in HIR
- [ ] Preserve enough environment for later reduction:
  - argument constants when known
  - template bindings when known
  - enclosing compile-time context when needed
- [ ] Keep `evaluate_consteval_call()` reusable from a later HIR pass
- [ ] Avoid making consteval reduction only a side effect of lowering
- [ ] Add printer/debug support for unreduced compile-time call nodes

Files likely involved:

- `src/frontend/sema/consteval.hpp`
- `src/frontend/sema/consteval.cpp`
- `src/frontend/hir/ir.hpp`
- `src/frontend/hir/ast_to_hir.cpp`
- `src/frontend/hir/hir_printer.cpp`

### Phase 5 Checklist: add HIR fixpoint reduction loop

- [ ] Add a HIR pass entry point for compile-time reduction
- [ ] Implement one iteration step for template instantiation
- [ ] Implement one iteration step for consteval reduction
- [ ] Re-run until convergence or explicit iteration limit
- [ ] Surface structured diagnostics on irreducible required compile-time nodes
- [ ] Ensure pass ordering is deterministic

Files likely involved:

- `src/frontend/hir/ast_to_hir.cpp`
- `src/frontend/sema/sema.cpp`
- new HIR pass files if needed

### Phase 6 Checklist: define materialization boundary

- [ ] Decide what counts as a materialized specialized function
- [ ] Keep non-materialized template entities representable in HIR
- [ ] Ensure codegen only consumes materialized entities
- [ ] Keep compile-time reduction semantics separate from emission policy

Files likely involved:

- `src/frontend/hir/ir.hpp`
- `src/frontend/hir/ast_to_hir.cpp`
- later codegen/lowering files

### Phase 7 Checklist: specialization identity

- [ ] Define a stable specialization key
- [ ] Include canonicalized template args in the key
- [ ] Ensure identity is deterministic across TUs
- [ ] Keep encoding serializable for future link-time/JIT use

Files likely involved:

- `src/frontend/sema/canonical_symbol.hpp`
- `src/frontend/sema/canonical_symbol.cpp`
- future HIR metadata files

## First Patch Checklist

If we want the smallest useful first implementation step, do this first:

- [ ] Introduce a HIR-visible representation for template function definitions
- [ ] Introduce a HIR-visible representation for template applications/calls
- [ ] Preserve existing parser/canonical template metadata through lowering
- [ ] Add HIR printer support so we can inspect preserved template structure
- [ ] Do not change final materialization semantics yet

Suggested files for the first patch:

- `src/frontend/hir/ir.hpp`
- `src/frontend/hir/ast_to_hir.cpp`
- `src/frontend/hir/hir_printer.cpp`

## Regression Checklist

- [ ] `tests/internal/cpp/postive_case/template_func.cpp` still passes
- [ ] `tests/internal/cpp/postive_case/consteval_template.cpp` still passes
- [ ] `tests/internal/cpp/postive_case/consteval_template_sizeof.cpp` still passes
- [ ] `tests/internal/cpp/postive_case/consteval_nested_template.cpp` still passes
- [ ] `tests/internal/cpp/postive_case/if_constexpr_template_chain.cpp` still passes
- [ ] Existing non-template consteval tests still pass
- [ ] HIR/canonical dump output remains inspectable for template cases
