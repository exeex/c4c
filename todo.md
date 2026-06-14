Status: Active
Source Idea Path: ideas/open/256_phase_f3_prepared_liveness_authority_blocker_map.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Final Blocker Map Summary

# Current Packet

## Just Finished

Step 5 - Final Blocker Map Summary completed the analysis-only runbook for
`PreparedBirModule::liveness`.

Consumer buckets mapped:

- Producers and construction: `PreparedBirModule::liveness`,
  `BirPreAlloc::run_liveness()`, pipeline ordering, and
  `PrepareOptions::run_liveness` create or gate public prepared liveness
  records. They are not demotion candidates because they produce identity plus
  intervals, program points, call points, loop depth, predecessor-edge facts,
  and `crosses_call` data used by later planning.
- Direct authority consumers: `BirPreAlloc::run_regalloc()`, regalloc interval
  and classification helpers, spill/reload and value-location production,
  call-plan preservation, and i128/f128 helper planners consume liveness as
  allocation, storage/home, move-scheduling, carrier/helper, selected-call,
  target-register, or target-storage authority.
- Compatibility and output consumers: liveness identity tests,
  `filter_prepared_module_to_function(...)`, prepared printer/status behavior,
  and CLI/debug filtering keep the public prepared liveness surface observable.
- Derived target consumers: AArch64 helper/call/regalloc behavior, x86
  aggregate/query wrappers, and RISC-V aggregate input risk remain compatibility
  blockers because their observable behavior depends on liveness-derived
  helper, regalloc, value-location, call-plan, or aggregate-shape contracts.

Demotion blockers:

- Allocation, storage/home, move scheduling, helper planning, target register,
  target storage, output/status policy, and broad prepared compatibility are all
  still attached to the public liveness records.
- `PrepareOptions::run_liveness` remains ambiguous for normal-pipeline
  absent/skipped liveness: skipped or manually-run liveness is not proven
  fail-closed for regalloc, call plans, helper planning, printer/status, or
  target paths.
- Duplicated lookup/helper boundaries and derived printer/target behavior make
  a wrapper, rename, deletion, or private-pass-context move unsafe without a
  future source idea and proof set.

Fail-closed gaps:

- No complete proof covers absent/skipped liveness flowing through normal
  regalloc, call-plan, helper-planning, printer/status, or target paths.
- No complete stale-record, duplicate/conflict, function/value mismatch,
  unsupported-row, fallback, route-conflict, stale-call-point, or duplicate
  preserved-value proof exists for a candidate reader.
- Existing missing-fact rows and unsupported coverage prove compatibility
  behavior in narrow areas; they do not prove `liveness` can be hidden,
  demoted, deleted, wrapped, renamed, or moved behind private pass context.

Rejected one-reader candidates:

- Direct liveness identity tests are public prepared-surface assertions, not an
  exact reader that excludes policy-sensitive records.
- Prepared filtering uses function identity but preserves
  `filtered.liveness.functions` as dump/filter compatibility, so it cannot be
  isolated without the missing fail-closed rows and retained public surface.
- `BirPreAlloc::run_liveness()` is the producer, not a reader, and the facts it
  produces feed regalloc, call plans, helper planning, printer/status, and
  derived target behavior.
- Regalloc, call-plan, helper-planning, prepared printer/status, AArch64, x86,
  and RISC-V rows are policy or derived-compatibility authority, not
  identity-only candidates.

Final readiness decision: no current one-reader implementation candidate
remains. `PreparedBirModule::liveness` remains blocked public prepared planning
authority unless and until a future idea supplies a single exact reader, one
semantic fact, the retained prepared compatibility surface, and full
fail-closed proof for absent/skipped, stale, mismatch, duplicate/conflict,
unsupported, fallback, and derived printer/target behavior.

No code, test expectations, output strings, `plan.md`, or source idea content
were changed in this runbook step.

## Suggested Next

Supervisor should decide whether the active runbook is ready for lifecycle
closure. No implementation split is requested from this evidence.

## Watchouts

- This runbook made analysis-only `todo.md` updates. It did not demote, delete,
  wrap, rename, hide, or move `liveness`.
- The no-candidate decision depends on both direct policy consumers and missing
  fail-closed proof; future ownership work needs a new source idea with one
  exact reader and the complete proof set.
- `PrepareOptions::run_liveness` remains a normal-path absent/skipped liveness
  ambiguity.

## Proof

`git diff --check -- todo.md`

Passed for this packet. The delegated proof command does not produce
`test_after.log`; no replacement log should be created.
