Status: Active
Source Idea Path: ideas/open/124_hir_legacy_string_lookup_removal_convergence.md
Source Plan Path: plan.md
Current Step ID: Step 6
Current Step Title: Reprove HIR Cleanup And Decide Closure Or Split

# Current Packet

## Just Finished

Step 6 lifecycle review after committed Step 5 classification decided that the
remaining HIR string surfaces can stay classified in this source idea; no
follow-up split is needed for the rendered struct/template/member compatibility
caches at this time. The major retained surfaces are classified as:

- Compatibility bridges: `Module::fn_index` / `global_index`,
  `find_*_by_name_legacy`, `CompileTimeState` rendered template/consteval maps,
  rendered enum/const-int maps, and Lowerer rendered struct/template/member
  caches that still sit beside structured mirrors and parity checks.
- Final spelling / display: link-visible `name` / `mangled_name` fields,
  template specialization keys, `struct_defs` rendered tags, HIR printer labels,
  owner-key debug labels, and emitted ABI/codegen spelling.
- Diagnostics / dumps: compile-time diagnostics, deferred-template diagnostic
  strings, runtime lowering diagnostics, HIR dump/summary formatting, and parity
  mismatch reporting text.
- Local scope: `FunctionCtx` locals, params, labels, local const bindings,
  local function-pointer signature maps, static-local names, and pack-parameter
  bindings used while lowering a single function body.
- Unresolved boundaries: encoded template-argument refs, deferred NTTP
  expression text, pending-template-type dedup/progress keys, early
  `function_decl_nodes_` return-type probes, and rendered struct/member lookup
  inputs where every producer/consumer has not yet moved to owner/member keys.

Added focused comments at the primary declaration hubs so those compatibility,
local-scope, and unresolved-boundary roles are visible without changing
behavior.

Close rejected for now: source-idea completion appears satisfied, and the
supervisor's broader HIR proof passed, but the canonical close-time regression
logs do not cover the same scope. Current `test_before.log` is the earlier
single `frontend_hir_lookup_tests` scope, while current `test_after.log` is the
broader 78-test HIR-labeled scope. This delegation forbids touching
`test_before.log` or `test_after.log`, so plan-owner cannot refresh the
matching close gate here.

## Suggested Next

Next coherent Step 6 packet: provide matching before/after regression logs for
the broader HIR close scope, then rerun the close decision. If the matching
guard passes, this source idea is ready to close without a follow-up split.

## Watchouts

The Step 5 packet was classification-only. It did not convert another lookup
family or change lookup precedence. Remaining rendered maps that still
participate in lookup are either explicit legacy compatibility fallbacks,
local-scope maps, or unresolved producer/consumer boundaries with structured
mirrors/parity coverage where the current runbook has already added it.

## Proof

Step 5 focused proof ran exactly:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_hir_lookup_tests$' > test_after.log 2>&1
```

Result: passed. `test_after.log` contains `frontend_hir_lookup_tests`: 1 passed
/ 0 failed after rebuilding the default preset.

Supervisor-selected broader proof later ran exactly:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -L hir > test_after.log 2>&1
```

Result: passed. Current `test_after.log` contains 78 passed / 0 failed HIR-
labeled tests. Close-time regression guard is not accepted because current
`test_before.log` and `test_after.log` are different scopes.
