Status: Active
Source Idea Path: ideas/open/124_hir_legacy_string_lookup_removal_convergence.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Classify Remaining HIR String Surfaces

# Current Packet

## Just Finished

Started Step 5 classification for the remaining HIR string surfaces. The major
retained surfaces are now classified as:

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

## Suggested Next

Next coherent Step 5 packet: have the supervisor review whether the remaining
rendered struct/template/member compatibility caches should stay classified for
closure or be split into a follow-up metadata-propagation idea before Step 6.

## Watchouts

This packet is classification-only. It did not convert another lookup family or
change lookup precedence. Remaining rendered maps that still participate in
lookup are either explicit legacy compatibility fallbacks, local-scope maps, or
unresolved producer/consumer boundaries with structured mirrors/parity coverage
where the current runbook has already added it.

## Proof

Supervisor-selected proof ran exactly:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_hir_lookup_tests$' > test_after.log 2>&1
```

Result: passed. `test_after.log` contains `frontend_hir_lookup_tests`: 1 passed
/ 0 failed after rebuilding the default preset.
