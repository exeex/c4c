Status: Active
Source Idea Path: ideas/open/268_phase_f4_prepared_bir_module_liveness_authority_blocker_follow_up.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Liveness Authority

# Current Packet

## Just Finished

Step 1, `Inventory Liveness Authority`, inventoried `PreparedBirModule::liveness`
authority without implementation changes.

Evidence:
- Definition/storage: `src/backend/prealloc/liveness.hpp:38` defines
  `PreparedLiveInterval`; `src/backend/prealloc/liveness.hpp:44` defines
  `PreparedLivenessValue` with prepared value identity, stack object identity,
  type/kind, def/use points, and optional interval; `src/backend/prealloc/liveness.hpp:59`
  defines block live-in/live-out rows; `src/backend/prealloc/liveness.hpp:70`
  defines function rows with intervals, call points, loop depth, blocks, and
  values; `src/backend/prealloc/liveness.hpp:80` stores all rows in
  `PreparedLiveness::functions`. `src/backend/prealloc/module.hpp:38` defines
  `PreparedBirModule`, and `src/backend/prealloc/module.hpp:47` stores public
  `PreparedLiveness liveness`.
- Construction/publication: `src/backend/prealloc/module.hpp:377` constructs
  `BirPreAlloc` around a mutable `PreparedBirModule`; `src/backend/prealloc/prealloc.cpp:29`
  runs legalize, stack layout, liveness, out-of-SSA, regalloc, then contract
  publishers. `src/backend/prealloc/liveness.cpp:906` is the sole prepared
  liveness row producer found: it clears/reserves `prepared_.liveness.functions`,
  builds dense named values, CFG successors/predecessors, phi predecessor uses,
  program points, live-in/live-out dataflow, intervals, call points, loop depth,
  blocks, and values, then pushes each `PreparedLivenessFunction` at
  `src/backend/prealloc/liveness.cpp:1001`.
- Producers of prepared liveness rows: only `BirPreAlloc::run_liveness()` in
  `src/backend/prealloc/liveness.cpp:906`. `PrepareOptions::run_liveness` exists
  at `src/backend/prealloc/names.hpp:21`, but the active `BirPreAlloc::run()`
  path calls `run_liveness()` unconditionally at `src/backend/prealloc/prealloc.cpp:33`.
  Route focus filtering copies and erases rows but does not produce semantic
  rows: missing function focus clears liveness at `src/backend/backend.cpp:599`;
  function focus erases non-matching liveness rows at `src/backend/backend.cpp:635`.
- Direct readers of prepared liveness rows: `BirPreAlloc::run_regalloc()` reads
  `prepared_.liveness.functions` at `src/backend/prealloc/regalloc.cpp:473` and
  `src/backend/prealloc/regalloc.cpp:479`, then derives regalloc/value-location
  rows, allocation constraints, interference, call-crossing policy, spill/reload
  ops, and regalloc notes from the liveness rows through
  `src/backend/prealloc/regalloc.cpp:500`, `src/backend/prealloc/regalloc.cpp:520`,
  `src/backend/prealloc/regalloc.cpp:546`, `src/backend/prealloc/regalloc.cpp:578`,
  `src/backend/prealloc/regalloc.cpp:647`, and `src/backend/prealloc/regalloc.cpp:779`.
  The supporting liveness consumers in `src/backend/prealloc/regalloc/intervals.cpp:13`,
  `src/backend/prealloc/regalloc/intervals.cpp:46`, `src/backend/prealloc/regalloc/intervals.cpp:61`,
  `src/backend/prealloc/regalloc/spill_reload.cpp:13`, and
  `src/backend/prealloc/regalloc/classification.cpp:9` consume
  `PreparedLivenessValue`/`PreparedLivenessFunction` facts.
- More direct readers: `populate_call_plans()` locates the row with
  `find_liveness_function(prepared.liveness, ...)` at
  `src/backend/prealloc/call_plans.cpp:2751`; call preservation uses liveness
  program points and live intervals in `build_call_preserved_values()` at
  `src/backend/prealloc/call_plans.cpp:1479` and
  `src/backend/prealloc/call_plans.cpp:1588`. The f128/i128 runtime helper
  publishers locate liveness rows at `src/backend/prealloc/f128_runtime_helpers.cpp:1163`
  and `src/backend/prealloc/i128_runtime_helpers.cpp:1440`, then use call
  program points and liveness function pointers for helper boundary/carrier
  facts (`src/backend/prealloc/f128_runtime_helpers.cpp:920`,
  `src/backend/prealloc/i128_runtime_helpers.cpp:938`,
  `src/backend/prealloc/i128_runtime_helpers.cpp:1308`).
- Public prepared compatibility surfaces retained: the public
  `PreparedBirModule::liveness` field; prepared-printer output from
  `src/backend/prealloc/prepared_printer.cpp:39`, which does not print raw
  liveness rows but prints liveness-derived summaries/plans such as regalloc,
  value locations, call plans, storage plans, helper facts, and notes via
  `src/backend/prealloc/prepared_printer.cpp:77`; route-debug focus filtering
  and counts in `src/backend/backend.cpp:287`, `src/backend/backend.cpp:588`,
  `src/backend/backend.cpp:699`, `src/backend/backend.cpp:934`, and
  `src/backend/backend.cpp:1056`; target emission paths consume derived
  prepared surfaces, not raw liveness rows (`src/backend/backend.cpp:1336`,
  `src/backend/backend.cpp:1343`, `src/backend/backend.cpp:1351`,
  `src/backend/backend.cpp:1377`, `src/backend/backend.cpp:1390`,
  `src/backend/backend.cpp:1501`, and `src/backend/backend.cpp:1525`).
  A search found no direct `.liveness`/`PreparedLiveness` reader under
  `src/backend/mir`.
- Candidate semantic liveness fact: the strongest current semantic fact is the
  prepared liveness dataflow row itself produced by `BirPreAlloc::run_liveness()`:
  named-value identity, CFG block identity, exact def/use program points,
  predecessor-edge phi uses, live-in/live-out sets, live intervals, call points,
  and loop depth. No separate BIR or route-level liveness fact was found that
  owns this meaning independently; downstream route facts appear derived from
  prepared liveness plus target/prepared policy rather than authoritative
  semantic liveness.
- Test/support readers: backend liveness tests read the public field directly
  through helper functions at `tests/backend/bir/backend_prepare_liveness_test.cpp:91`
  and `tests/backend/bir/backend_prepare_liveness_test.cpp:113`, with coverage
  for phi, loop priority, cross-call, and helper-family call-point behavior.
  Frame/stack call-contract tests also read public liveness rows at
  `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp:100`.

## Suggested Next

Execute Step 2 from `plan.md`: classify each direct reader as identity-only,
printer/debug/status, fallback, wrapper, target-policy, or unsupported
behavior. The likely high-risk readers to classify first are regalloc,
call-preservation planning, and f128/i128 helper fact publication.

## Watchouts

- This active plan is analysis and blocker mapping only.
- Do not demote, delete, privatize, wrap, migrate, or implement
  `PreparedBirModule::liveness`.
- Do not weaken printer, route-debug, helper/oracle/status, fallback, wrapper,
  exact target-output, unsupported, or baseline behavior.
- Do not claim progress through expectation rewrites, helper renames,
  classification-only notes, or named-fixture proof.
- Step 1 did not prove an exact identity-only reader. The direct production
  fact is semantic, but the direct readers observed so far derive target-facing
  allocation, preservation, spill/reload, helper, and compatibility surfaces.
- `PrepareOptions::run_liveness` appears present but not honored by the active
  `BirPreAlloc::run()` pipeline; treat absent/skipped row behavior as unmapped
  until Step 3.

## Proof

Analysis-only packet. Delegated proof command:
- read back `todo.md`
- `git status --short`
- `git diff --check`

No build or CTest was required by the packet because only `todo.md` changed.
