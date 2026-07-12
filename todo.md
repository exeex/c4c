# Current Packet

Status: Active
Source Idea Path: ideas/open/721_x86_defined_function_prepared_core_completion.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair General Prepared-Core Production

## Just Finished

- Step 2 added a common `BirPreAlloc` prerequisite guard for independently
  callable stack-layout, liveness, and regalloc production. When legalization
  has not completed, the first later producer runs the genuine common legalize
  phase and publishes control-flow facts before admitting the same definitions
  to addressing/liveness/value-location production.
- The localized manual-phase `grouped_spill_reload_contract` route now reaches
  x86 emission with a common prepared-core view. The already-legalized route is
  unchanged because the guard recognizes its completed legalize phase, keeping
  stable `FunctionNameId` interning from production through lookup.
- States that already claim legalization are not cleared, synthesized, or
  repaired by the guard, so missing or inconsistent published facts remain
  subject to the existing fail-closed prepared-core admission and emitter
  invariant.

## Suggested Next

- Step 3: strengthen focused positive coverage across the manual-phase and
  already-legalized defined-function shapes, plus negative coverage showing
  genuinely absent or inconsistent preparation still fails closed.

## Watchouts

- Step 3 should explicitly exercise a state that claims completed legalization
  but lacks or mismatches a required bank; the prerequisite guard deliberately
  does not overwrite such state.
- Keep coverage on the common producer/admission contract and avoid any x86
  fallback, fixture injection, expectation rewrite, idea 716 cursor change, or
  idea 718 attribution/publication expansion.

## Proof

- Ran exactly:
  `cmake --build --preset default > test_after.log 2>&1; ctest --test-dir build -j --output-on-failure -R '^backend_prepare_frame_stack_call_contract$' >> test_after.log 2>&1`.
- Build passed and the focused test passed (1/1) without expectation changes.
  The supervisor-selected proof is sufficient for Step 2. Canonical log:
  `test_after.log`.
