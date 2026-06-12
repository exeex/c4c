Status: Active
Source Idea Path: ideas/open/232_phase_e3_route6_x86_scalar_i32_argument_source_route_debug_follow_up.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Use Route 6 Agreement Only Under Prepared Compatibility

# Current Packet

## Just Finished

Completed Step 1 discovery for `plan.md` Step 1, `Locate The Row And Agreement
Boundary`.

Exact selected route-debug row:

```text
    route6 scalar arg call#0 block=entry inst#2 callee=addip0 arg#0 source=%t1 kind=ArgumentValue
```

Row owner and fixture:

- Emitted by `append_route6_scalar_call_argument_sources(...)` in
  `src/backend/mir/x86/debug/debug.cpp`.
- Reached from `render_report(...)` for the x86 trace path.
- Positive fixture is
  `legalize_single_block_same_module_scalar_call_wrapper_miss_module()` in
  `tests/backend/bir/backend_x86_route_debug_test.cpp`.
- The selected call is function
  `single_block_same_module_scalar_call_wrapper_miss`, block `entry`,
  instruction index `2`, callee `addip0`, argument index `0`, argument `%t1`,
  `i32`, source relationship `Register`, `source_value_id=7`,
  `source_value_name=%t1`.

Agreement boundary:

- `src/backend/mir/x86/x86.hpp` owns `ConsumedPlans` and
  `find_consumed_scalar_i32_call_argument_source(...)`.
- `consume_plans(...)` builds shared prepared lookups and the shared
  `bir::Route6CallUseSourceIndex` with
  `bir::route6_build_call_use_source_index(...)`.
- `find_consumed_scalar_i32_call_argument_source(...)` is the Route
  6/prepared agreement boundary: it requires an existing prepared call argument
  plan, a non-empty shared Route 6 index, a named `i32` argument, an available
  `route6_find_call_argument_source(...)` record, exact argument pointer match,
  `Route6CallUseSourceKind::ArgumentValue`, present Route 6/prepared source
  ids, and equal Route 6/prepared source ids.
- `src/backend/bir/bir.cpp` owns the index status surface:
  `route6_call_argument_source_record(...)` reports missing call, wrong call,
  missing argument, duplicate relationship, missing source relationship,
  ABI-bound exclusion, and available records; `route6_find_call_argument_source(...)`
  returns indexed matches or `MissingCall`/`WrongCall`/`NoMatch`.
- Current module consumers already pass the optional agreed Route 6 source to
  direct-extern scalar `i32` argument emission in
  `src/backend/mir/x86/module/module.cpp`; this is a no-change surface unless
  Step 2 intentionally proves the row through shared agreement.

Minimal Step 2 implementation targets:

- Primary: `src/backend/mir/x86/debug/debug.cpp`, only the selected
  `append_route6_scalar_call_argument_sources(...)` route-debug row path.
- Boundary/helper: `src/backend/mir/x86/x86.hpp`, only if the existing
  `find_consumed_scalar_i32_call_argument_source(...)` contract needs a narrow
  reusable predicate or clearer fail-closed guard for the selected row.
- Query/index helpers: `src/backend/bir/bir.cpp` and `src/backend/bir/bir.hpp`
  are currently read-only targets; use only if Step 2 finds a real semantic
  index-status gap, not to reshape the row text.
- Tests for Step 2/3 proof: `tests/backend/bir/backend_x86_route_debug_test.cpp`,
  `tests/backend/bir/backend_x86_handoff_boundary_direct_extern_call_test.cpp`,
  and nearby Route 6/prepared lookup coverage in
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`.

## Suggested Next

Delegate Step 2 from `plan.md`: make the selected x86 route-debug row consume
Route 6 scalar `i32` argument-source evidence only through
`find_consumed_scalar_i32_call_argument_source(...)` after prepared call
argument agreement, then prove the row and fail-closed paths without changing
row spelling, wrapper output, expected strings, baselines, supported contracts,
or prepared call/debug output.

## Watchouts

Positive and fallback proof cases already located:

- Positive row check:
  `backend_x86_route_debug_test.cpp` expects the exact selected row in
  `single_block_same_module_scalar_call_wrapper_miss_trace`.
- Route-debug fallback checks in the same test:
  `selected_route6_scalar_arg_without_route6_facts()` clears `arg_sources`;
  `selected_route6_scalar_arg_with_invalid_source_id()` resets the Route 6
  source id; `selected_route6_scalar_arg_with_duplicate_conflict()` adds a
  conflicting duplicate source; `selected_route6_scalar_arg_with_prepared_mismatch()`
  changes the prepared source id. Each keeps the function row and forbids the
  selected Route 6 scalar arg row.
- `backend_x86_handoff_boundary_direct_extern_call_test.cpp`
  `check_consumed_plans_threads_route6_scalar_call_argument_source()` proves the
  `ConsumedPlans` helper threads an agreed scalar `i32` Route 6 argument source,
  fails closed when Route 6 facts are absent, preserves the prepared call
  argument selector, and keeps prepared fallback asm unchanged.
- `backend_prepared_lookup_helper_test.cpp` nearby Route 6 call-argument
  coverage proves available records, ABI-bound exclusion, duplicate source
  records, missing argument, wrong call, missing source relationship, missing
  producer, and source-id/prepared mismatch behavior.

Nearby same-feature cases to keep in proof:

- Same x86 route-debug fixture also includes non-selected scalar call behavior
  in `single_block_same_module_scalar_call_wrapper_miss`: later `i64` call
  argument/source activity must not grow a new row.
- x86 direct-extern boundary test covers another scalar `i32` call-argument
  source (`printf` argument index `1`, source `%t0`) through the same
  `ConsumedPlans` boundary.
- Prepared lookup helper tests cover same Route 6 call-argument source/index
  semantics independent of x86 route-debug text.
- Prepared printer call wrapper checks cover same-module, direct extern fixed
  arity, direct extern variadic, and indirect wrapper rows; these are no-change
  surfaces.

No-change surfaces:

- Do not change the exact route-debug row text or add expectation/baseline
  rewrites.
- Do not change x86 wrapper behavior, ABI/call wrapper policy, public fallback,
  prepared call printer/debug text, prepared lookup semantics, direct-call or
  helper-oracle strings, supported/unsupported contracts, `ConsumedPlans`
  ownership, or BIR Route 6 public record contracts.
- Do not replace prepared authority in module lowering. Existing optional Route
  6 use in `module.cpp` remains subordinate to prepared value homes and
  prepared call bundles.
- Do not add fixture-name or row-text shortcuts. The proof must exercise the
  shared agreement helper and nearby same-feature cases.

Step 2 proof gaps to close or explicitly prove:

- Current `build` has `C4C_ENABLE_X86_BACKEND_TESTS=OFF`, so
  `backend_x86_route_debug` and `backend_x86_handoff_boundary` are not present
  in the local `CTestTestfile.cmake` or `build.ninja`; Step 2 needs an enabled
  x86 test build or an agreed supervisor proof command that builds/runs those
  targets.
- Route-debug tests currently cover absent, invalid source id,
  duplicate/conflict, and prepared mismatch. They do not visibly isolate
  `WrongCall`/`NoMatch`, non-named argument, non-`i32` argument, empty Route 6
  index, or missing prepared call argument on the selected row.
- If Step 2 touches shared helpers, add or run `backend_prepared_lookup_helper`
  so Route 6 public record/status behavior stays stable.
- If Step 2 touches `module.cpp` or prepared dispatch indirectly, add or run
  `backend_x86_handoff_boundary` and prepared printer/wrapper checks so
  direct-call, helper-oracle, wrapper, and prepared output surfaces remain
  byte-stable.

## Proof

Proof command delegated for this discovery packet:

```sh
git diff --check -- todo.md && git status --short
```

No implementation, test behavior, route-debug text, expected string, baseline,
wrapper, direct-call, helper-oracle, supported/unsupported, or prepared
call/debug output changes were made in this packet.
