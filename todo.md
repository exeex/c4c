Status: Active
Source Idea Path: ideas/open/238_aarch64_atomic_machine_nodes.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Print AArch64 Atomic Machine Nodes

# Current Packet

## Just Finished

Step 5 `Print AArch64 Atomic Machine Nodes` added final AArch64 machine-printer
emission for selected atomic machine nodes from structured record facts.

Changed behavior:
- Atomic loads and stores now print width-aware plain/acquire/release AArch64
  forms from selected `AtomicMemoryInstructionRecord` fields.
- Non-relaxed fences print `dmb ish` from selected fence records.
- Atomic RMW records print exclusive-access retry loops using structured
  pointer, old-value, input, new-value scratch, status, opcode, and ordering
  facts.
- Compare-exchange records print success/failure paths from structured
  expected, desired, loaded-value, result, status, success ordering, failure
  ordering, result-mode, retry-loop, and monitor-clear facts.
- Printer-only scratch/status fields were added to selected atomic records so
  loops do not fall back to archived fixed scratch-register contracts.
- Incomplete selected records remain fail-closed when printer-required loop
  scratch, status, or loaded-value facts are absent.

Added test coverage:
- `backend_aarch64_machine_printer_test` now proves selected atomic
  load/store/fence records print concrete AArch64 assembly.
- The same printer test proves RMW retry-loop output and compare-exchange
  boolean/old-value success/failure output are emitted from structured records.
- Fail-closed printer coverage rejects atomic RMW and compare-exchange records
  that lack required printer facts.

## Suggested Next

Begin Step 6 `Prove Atomic Route And Decide Lifecycle` by validating the
representative atomic backend route and asking the plan owner whether the source
idea is complete or needs a follow-on gap recorded.

## Watchouts

- Do not infer atomic semantics from volatile flags, rendered text, fixed
  scratch-register snippets, or named testcase shortcuts.
- Step 5 prints from selected atomic records, but the prepared-to-selected route
  still does not allocate loop scratch/status registers for all end-to-end
  atomic programs; Step 6 should decide whether that is a lifecycle-closing gap
  or a follow-on packet.
- Preserve ordinary volatile memory behavior separately from atomic behavior;
  atomic selection must continue to require carrier provenance.
- RMW and compare-exchange printer records now require explicit scratch/status
  register facts; do not replace that with fixed scratch-register assumptions.
- Do not fold intrinsic, inline-assembly, binary128, scalar FP, or i128 behavior
  into this route.

## Proof

Proof command:
`set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } 2>&1 | tee test_after.log`

Result: passed, `139/139` backend tests green.

Proof log: `test_after.log`.
