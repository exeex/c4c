# Plan Execution State

## Baseline
- 1850/1850 tests passing (2026-03-17)

## Overall Assessment

The implementation has made real progress, but it does not yet fully match the architecture described in `plan.md`.

Current state in one sentence:
- HIR now preserves useful template/consteval metadata and exposes debug visibility for compile-time activity
- but template instantiation and consteval reduction are still performed eagerly during AST-to-HIR lowering, not by a true HIR-owned reduction loop

So the fairest summary is:
- Phase 1: complete
- Phases 2-4: partially complete, with good observability and metadata preservation
- Phases 5-6: scaffolded, but not complete relative to the original plan goals
- Phase 7: largely complete for current scope

---

## Added Testcases

The work added 12 HIR-oriented regression tests in `tests/internal/InternalTests.cmake`:

- `cpp_hir_template_def_dump`
- `cpp_hir_consteval_template_dump`
- `cpp_hir_template_call_info_dump`
- `cpp_hir_consteval_call_info_dump`
- `cpp_hir_consteval_template_call_info_dump`
- `cpp_hir_compile_time_reduction_stats`
- `cpp_hir_template_instantiation_resolved`
- `cpp_hir_consteval_reduction_verified`
- `cpp_hir_consteval_template_reduction_verified`
- `cpp_hir_fixpoint_convergence`
- `cpp_hir_specialization_key`
- `cpp_hir_materialization_stats`

What these tests do verify:
- template metadata appears in `--dump-hir`
- template-originated calls keep printable template call info
- consteval calls are recorded in HIR dump output
- compile-time pass stats are printed
- specialization keys are printed
- materialization stats are printed

What these tests do not yet verify:
- deferred template instantiation in HIR
- deferred consteval reduction in HIR
- irreducible compile-time nodes flowing through the new diagnostics path
- a real materialization policy that leaves some functions non-materialized
- convergence driven by HIR doing new work, rather than by eager lowering having already done the work

---

## Phase 1: Constrain sema to conservative compile-time work
**Status: COMPLETE**

### Assessment
This phase is effectively satisfied by the current architecture.

### Verified state
- sema still uses constant evaluation mainly for legality and diagnostic-oriented folding
- sema is not the owner of template instantiation
- sema is not the owner of consteval function body interpretation
- heavy compile-time behavior lives in HIR lowering, not in sema

### Notes
This phase being complete does not imply the long-term architecture is complete; it only means sema has not become the wrong owner.

---

## Phase 2: Make HIR preserve template function definitions
**Status: PARTIAL**

### What is implemented
- `HirTemplateDef`, `HirTemplateInstantiation`, and `TypeBindings` were added to HIR
- `Module::template_defs` now records template metadata
- HIR printer emits a `--- templates ---` section
- dump tests confirm the metadata is inspectable

### What is working well
- template definitions are visible as first-class metadata in HIR dumps
- instantiation bindings and specialization keys can be attached to template definitions

### Gap vs `plan.md`
The original plan required template bodies to remain available after lowering for later compile-time reduction and materialization policy.

That is not true yet:
- `HirTemplateDef` preserves metadata only
- the original template body is not stored in HIR
- lowering still lowers concrete instantiated functions eagerly

### Revised conclusion
Phase 2 is partially complete as metadata preservation, but not complete as template-definition preservation in the stronger architectural sense intended by `plan.md`.

---

## Phase 3: Preserve template applications/calls in HIR
**Status: PARTIAL**

### What is implemented
- `TemplateCallInfo` was added to `CallExpr`
- lowering records template source name and resolved template arguments
- HIR printer renders calls in `add<int>(...)` style
- dump tests validate the printed form

### What is working well
- HIR can now remember that a call originated from a template application
- debug output is much better than before

### Gap vs `plan.md`
The original plan wanted template application to remain distinct from an ordinary concrete call, including the notion of a specialized callable request.

Current behavior is weaker:
- the callee is still lowered to an already resolved concrete function symbol
- `TemplateCallInfo` is attached metadata, not a separate HIR node with independent semantics
- specialized callable values are not represented as a first-class HIR concept

### Revised conclusion
Phase 3 is partially complete as call-site annotation and observability, but not complete as a semantic preservation layer for template application.

---

## Phase 4: Represent consteval as HIR-reducible compile-time nodes
**Status: PARTIAL**

### What is implemented
- `ConstevalCallInfo` was added to HIR
- lowering records consteval call name, constant args, template bindings, and result
- HIR printer emits a `--- consteval calls ---` section
- dump tests verify the recorded metadata

### What is working well
- consteval activity is no longer invisible in dumps
- template-aware consteval metadata is preserved for inspection
- the data model is a reasonable base for future deferred reduction

### Gap vs `plan.md`
The original plan wanted consteval to survive into HIR as reducible nodes before final reduction.

Current behavior is still eager:
- consteval calls are interpreted during lowering
- the call is replaced immediately by an `IntLiteral`
- HIR stores a side record describing what happened, rather than preserving an unreduced HIR node that later passes own

### Revised conclusion
Phase 4 is partially complete as metadata preservation and debugging support, but not complete as true HIR-owned consteval reduction infrastructure.

---

## Phase 5: Add HIR compile-time reduction loop
**Status: NOT COMPLETE**

### What is implemented
- a `run_compile_time_reduction()` pass entry point exists
- pass stats and formatting exist
- the pass scans template-originated calls and consteval records
- the pass can report pending items and structured diagnostics
- dump tests verify the stats section appears and contains expected counts

### What is not implemented yet
- the pass does not instantiate templates
- the pass does not reduce consteval calls
- the pass does not mutate HIR to unlock additional work across iterations
- the apparent convergence is mostly a post-hoc verification of work already done in lowering

### Important reality check
The code itself documents this clearly:
- template instantiation is still done during AST-to-HIR lowering
- consteval reduction is still done during AST-to-HIR lowering
- the compile-time pass is currently a verification/reporting hook

### Revised conclusion
Phase 5 should be considered scaffolded, not complete.

### Remaining work to satisfy `plan.md`
- preserve unresolved template applications in HIR until the pass resolves them
- preserve reducible consteval calls in HIR until the pass evaluates them
- let one iteration create new ready work for the next
- prove convergence on cases that genuinely require multi-step HIR-owned reduction
- add tests that fail if lowering eagerly erases the structure again

---

## Phase 6: Define materialization boundary
**Status: NOT COMPLETE**

### What is implemented
- `Function::materialized` flag exists
- `materialize_ready_functions()` pass exists
- codegen skips non-materialized functions
- dump output includes materialization stats

### What is not implemented yet
- current policy marks every concrete function as materialized
- there is no real deferred-emission behavior
- no code path demonstrates template semantics remaining valid while emission is delayed
- tests only prove that a stats line is printed

### Revised conclusion
Phase 6 currently establishes an API seam, but not a meaningful materialization boundary or policy.

### Remaining work to satisfy `plan.md`
- keep compile-time-only entities unmaterialized in at least some cases
- make codegen consume only the functions selected by policy
- add tests that distinguish preserved template semantics from emitted concrete functions

---

## Phase 7: Specialization identity and caching
**Status: MOSTLY COMPLETE**

### What is implemented
- `SpecializationKey` exists
- canonical type stringification exists
- instantiations record a stable key
- HIR printer shows the key
- dump tests verify the expected printed form

### Why this is stronger than some earlier phases
- the feature is concrete, local, and already exercised by HIR metadata
- its current scope is clear and does not depend on deferred reduction being implemented first

### Remaining caveats
- cross-TU and future JIT reuse are architectural goals, not yet demonstrated end-to-end
- current validation is still dump-based rather than integration-level caching behavior

### Revised conclusion
Phase 7 is mostly complete for current in-process metadata needs, with future integration still outstanding.

---

## Revised Summary

What is genuinely delivered:
- sema remains conservative
- HIR now preserves template and consteval metadata
- HIR dumps are much more informative
- compile-time and materialization passes now have visible API seams
- specialization identity has a concrete deterministic representation

What is not yet delivered from the original plan:
- HIR as the actual owner of template instantiation
- HIR as the actual owner of consteval reduction
- a real instantiate/reduce fixpoint loop that performs new work each iteration
- a meaningful delayed materialization policy

## Recommended next milestone

The next honest milestone should be:
- move one real class of deferred template instantiation out of lowering and into `run_compile_time_reduction()`
- move one real class of deferred consteval evaluation out of lowering and into the same loop
- add at least one regression test that only passes if HIR, not lowering, performs the second-step reduction
