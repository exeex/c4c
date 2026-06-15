Status: Active
Source Idea Path: ideas/open/268_phase_f4_prepared_bir_module_liveness_authority_blocker_follow_up.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Map Fail-Closed Rows

# Current Packet

## Just Finished

Step 3, `Map Fail-Closed Rows`, mapped the liveness row-validity blocker
families without implementation changes. The map preserves the Step 2 decision:
there is no exact identity-only reader and no independent semantic liveness
fact today; the only semantic liveness authority is still the public prepared
row produced by `BirPreAlloc::run_liveness()`.

Fail-closed row map:
- Absent/skipped rows. Expected behavior before any authority movement:
  production preparation must either have one current liveness row for every
  non-declaration function that downstream liveness-derived phases depend on,
  or must stop publication of derived allocation, preservation, helper, printer,
  route-debug, and target-output facts for the affected function with explicit
  missing-fact/status evidence. Current support is partial only. Production
  `BirPreAlloc::run()` always calls `run_liveness()` at
  `src/backend/prealloc/prealloc.cpp:29-36`, even though `PrepareOptions` still
  exposes `run_liveness` at `src/backend/prealloc/names.hpp:18-23`.
  `run_liveness()` clears and rebuilds `prepared_.liveness.functions` at
  `src/backend/prealloc/liveness.cpp:906-909`, skips declarations at lines
  912-915, and publishes one row per visited function at line 1001. However,
  `run_regalloc()` sizes and iterates only the rows present at
  `src/backend/prealloc/regalloc.cpp:470-485`; a missing row silently means no
  regalloc/value-location row for that function rather than an explicit
  fail-closed diagnostic. Call plans look up by function name at
  `src/backend/prealloc/call_plans.cpp:2748-2756`; preservation construction
  returns empty when `liveness_function == nullptr` at lines 1486-1489 and
  1596-1603. F128/I128 helper ownership does append
  `live_preservation_requires_structured_live_across_helper_facts` when
  liveness/regalloc/call-point evidence is absent
  (`src/backend/prealloc/f128_runtime_helpers.cpp:920-950`,
  `src/backend/prealloc/i128_runtime_helpers.cpp:935-1007`). Proof gap/blocker:
  no central invariant proves row coverage or prevents empty derived output for
  absent rows. Compatibility surfaces to preserve: production full-route output,
  helper missing-fact spelling, tests that manually stage liveness then run
  regalloc with `run_liveness = false`, and public `prepared.liveness` test
  access.
- Stale rows. Expected behavior before any authority movement: a liveness row
  built from an older module, older names table, older stack-layout context, or
  older BIR body must be rejected before target-policy readers consume it.
  Current support is missing. `run_liveness()` clears rows when it runs, but
  manual staged flows can pass an existing `PreparedBirModule` into a new
  `BirPreAlloc` and call later phases; `run_regalloc()` trusts the existing row
  contents at `src/backend/prealloc/regalloc.cpp:478-485`, combines them with
  current target/profile/stack state at lines 500-568, and uses call points for
  register assignment at lines 635-735. The call-plan and helper paths use
  first matching `function_name` rows and do not compare generation, module
  identity, instruction count, block labels, or value universe
  (`src/backend/prealloc/call_plans.cpp:646-668`,
  `src/backend/prealloc/f128_runtime_helpers.cpp:190-207`,
  `src/backend/prealloc/i128_runtime_helpers.cpp:209-225`). Proof gap/blocker:
  no generation stamp, source-module fingerprint, or row freshness assertion is
  present. Compatibility surfaces to preserve: staged test helpers that reuse
  prepared rows across manual phase calls, derived regalloc/value-location
  output, and exact target output derived from those rows.
- Mismatch rows. Expected behavior before any authority movement: rows whose
  `function_name`, block program points, instruction counts, value ids, value
  names, stack object ids, or intervals disagree with the prepared module must
  fail closed before publishing target behavior. Current support is partial and
  local. Call-plan lookup returns the first row whose `function_name` matches
  (`src/backend/prealloc/call_plans.cpp:646-655`), and missing block mapping
  makes preservation return empty (`src/backend/prealloc/call_plans.cpp:657-668`
  and 1588-1609). Regalloc still trusts every row it iterates, derives value
  identity and allocation state from row values at
  `src/backend/prealloc/regalloc.cpp:500-540`, builds constraints at lines
  546-568, publishes interference at lines 578-596, and allocates fallback stack
  slots at lines 737-775. Helper ownership records missing live-preservation
  facts when the call point cannot be found, but the generic regalloc and
  call-plan readers do not emit a liveness-mismatch fact. Proof gap/blocker: no
  row/module consistency validator checks all mismatch dimensions before
  derived output is published. Compatibility surfaces to preserve: current empty
  preservation behavior for missing call points, helper status facts, prepared
  printer summaries, route-debug filtering, and target-output behavior.
- Duplicate/conflict rows. Expected behavior before any authority movement:
  duplicate rows for the same function, or conflicts among duplicate rows, must
  be rejected or deterministically diagnosed before any reader chooses one row
  and another reader consumes both. Current support is missing. `run_liveness()`
  normally publishes one row per non-declaration function, but no reader checks
  uniqueness. Regalloc iterates all rows at `src/backend/prealloc/regalloc.cpp:485`,
  so duplicates can publish duplicate regalloc/value-location function records.
  Call-plan and helper lookup helpers return the first matching row
  (`src/backend/prealloc/call_plans.cpp:646-655`,
  `src/backend/prealloc/f128_runtime_helpers.cpp:190-198`,
  `src/backend/prealloc/i128_runtime_helpers.cpp:209-217`), so first-match
  behavior can disagree with regalloc's all-rows behavior. Proof gap/blocker:
  no duplicate detector or conflict diagnostic exists for liveness rows.
  Compatibility surfaces to preserve: first-match helper/call-plan behavior
  until replaced by an explicit lifecycle-authorized diagnostic, and existing
  regalloc publication shape for valid single-row modules.
- Unsupported rows. Expected behavior before any authority movement:
  unsupported liveness features should remain represented as missing/unsupported
  facts on the consuming compatibility surface, not as silently usable semantic
  authority. Current liveness rows have no unsupported-state field; unsupported
  and missing-fact behavior lives in downstream carrier/helper systems instead.
  Examples include helper live-preservation missing facts in F128/I128 helper
  ownership (`src/backend/prealloc/f128_runtime_helpers.cpp:946-950`,
  `src/backend/prealloc/i128_runtime_helpers.cpp:999-1003`) and broader
  prepared-printer fail-closed missing-fact surfaces in tests, but there is no
  liveness-specific unsupported row contract. Proof gap/blocker: unsupported
  liveness row semantics are not modeled, so demotion/private-pass-context work
  cannot rely on an unsupported-row fail-closed protocol. Compatibility surfaces
  to preserve: downstream `missing_required_facts`, prepared-printer diagnostic
  spelling, helper/oracle/status behavior, and unsupported target-route
  behavior.
- Fallback rows. Expected behavior before any authority movement: fallback from
  incomplete liveness must not produce target behavior that looks fully
  authoritative unless the fallback is explicitly marked. Current behavior
  contains unmarked fallback allocation. `run_regalloc()` sends values without a
  live interval or without a register class through stack-slot fallback when
  `normalized_value_size(value) != 0` at
  `src/backend/prealloc/regalloc.cpp:755-765`, and ordinary unassigned values
  also fall back to stack slots at lines 768-775. Call preservation returns
  empty preserved values when liveness is absent or the call point is missing
  (`src/backend/prealloc/call_plans.cpp:1486-1489` and 1596-1603). Helper paths
  add explicit missing facts for absent liveness/call point, but regalloc/value
  locations and call plans do not uniformly mark fallback. Proof gap/blocker:
  fallback output is not globally distinguishable from complete authority.
  Compatibility surfaces to preserve: current stack-slot fallback behavior,
  call-preservation empty-set behavior, helper missing facts, and exact target
  output generated from fallback allocation.
- Derived printer and target behavior. Expected behavior before any authority
  movement: prepared printing, route-debug filtering, helper/status output,
  regalloc/value-location summaries, call plans, and exact target emission must
  remain stable unless a separate lifecycle idea proves a semantic authority
  replacement and compatibility migration. Current support is compatibility
  only. Prepared printing starts at `src/backend/prealloc/prepared_printer.cpp:39`
  and emits completed phases, notes, BIR body, then derived summaries via
  appenders at lines 77-84 and later; it does not print raw liveness rows as an
  isolated semantic fact. Route-debug focus filtering clears liveness and other
  prepared rows when the function name is absent at
  `src/backend/backend.cpp:594-613` and erases rows outside the selected
  function at lines 635-640. Target emission consumes downstream prepared
  outputs such as regalloc, value locations, call plans, stack layout,
  storage/helper facts, and exact instruction/output spelling, all of which can
  be derived from liveness. Proof gap/blocker: no identity-only reader or
  compatibility wrapper proves these surfaces can be moved off public
  `PreparedBirModule::liveness`. Compatibility surfaces to preserve: prepared
  printer text, route-debug filtered structure, helper/oracle/status text,
  wrapper and fallback behavior when present, and exact target output.

Decision from Step 3:
- The fail-closed map is blocker-heavy. Absent/skipped rows have only partial
  helper/status fail-closed behavior; stale, duplicate/conflict, unsupported,
  and fallback rows lack central fail-closed contracts; mismatch handling is
  local and inconsistent across readers.
- No private-pass-context, demotion, wrapper, migration, deletion, or public
  field hiding packet is authorized by this evidence. Such work would move or
  hide the only current semantic liveness authority while fail-closed behavior
  remains incomplete.

## Suggested Next

Execute Step 4 from `plan.md`: decide later work eligibility from the blocker
map. Expected result, based on Step 2 and Step 3 evidence, is that later
private-pass-context, demotion, wrapper, migration, or deletion work remains
blocked unless the supervisor/plan-owner creates a separate lifecycle idea for
row validation or semantic authority replacement.

## Watchouts

- This active plan is analysis and blocker mapping only.
- Do not demote, delete, privatize, wrap, migrate, or implement
  `PreparedBirModule::liveness`.
- Do not weaken printer, route-debug, helper/oracle/status, fallback, wrapper,
  exact target-output, unsupported, or baseline behavior.
- Do not claim progress through expectation rewrites, helper renames,
  classification-only notes, or named-fixture proof.
- Step 2 did not find an exact identity-only reader. Step 3 did not find a
  complete fail-closed row contract. Treat public `PreparedBirModule::liveness`
  as blocked semantic authority.
- `PrepareOptions::run_liveness` remains present but is not honored by
  production `BirPreAlloc::run()`; manual staged tests use it as phase-control
  compatibility.
- Do not infer implementation eligibility from helper missing facts alone:
  helper status is the strongest fail-closed evidence, but regalloc,
  value-location, call-plan, route-debug, printer, and target-output surfaces do
  not share a central liveness row validator.

## Proof

Analysis-only packet. Delegated proof command:
- read back `todo.md`
- `git status --short`
- `git diff --check`

No build or CTest was required by the packet because only `todo.md` changed.
Result: `todo.md` readback completed, `git status --short` reported only
`M todo.md`, and `git diff --check` passed with no whitespace errors.
