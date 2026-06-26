Status: Active
Source Idea Path: ideas/open/394_rv64_same_module_sret_callee_home_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Capture Sret Param Home Evidence

# Current Packet

## Just Finished

Lifecycle disposition closed idea 393 and split the later representative
failure into idea 394.

Idea 393 is complete because Step 5 evidence shows the aggregate `va_arg`
cursor-stride boundary is repaired:

- both aggregate helpers now use `payload_size=4`, `copy_size=4`,
  `source_slot=8`, `progression_stride=8`, and `overflow_stride=8`
- the first aggregate read observes `10`
- the second aggregate read observes `20`
- both prior abort branches are skipped

The remaining `920908-1.c` failure is a later same-module sret/callee
`%ret.sret` home-publication boundary. `main` passes the sret address in `a0`,
but callee `f` later loads stack-homed `%ret.sret` from `0(sp)` without a prior
callee-side publication/save of incoming `a0` into that home slot.

## Suggested Next

Execute Step 1: capture prepared/BIR/object evidence for callee `%ret.sret`,
incoming `a0`, the stack home slot, and the final pointer-value return store.
Record the exact missing publication point and repair owner before selecting an
implementation route.

## Watchouts

- Do not reopen idea 393 aggregate `va_arg` cursor stride; the representative
  now reads both aggregate payloads correctly.
- Do not reopen idea 387 caller-side same-module sret object emission unless
  fresh evidence contradicts the current `a0` handoff.
- The current boundary is callee home publication for `%ret.sret`, not caller
  address construction.
- Avoid hard-coding `920908-1.c`, callee `f`, registers, stack offsets, or the
  observed final store sequence.

## Proof

Plan-owner close gate command run for idea 393:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed`

Result: PASS. The accepted current backend baseline remained 326/326 before
and 326/326 after with no new failures. `test_before.log` was used on both
sides for lifecycle-only close validation because `test_after.log` currently
contains Step 5 representative evidence rather than a CTest after-log.

Representative evidence for the split boundary:

- `test_after.log`
- `build/agent_state/393_step5_analysis.log`
- `build/agent_state/393_step5_920908-1.prepared.log`
- `build/agent_state/393_step5_920908-1.bir.log`
- `build/agent_state/393_step5_920908-1.case.log`
- `build/agent_state/393_step5_920908-1.run.log`
- `build/agent_state/393_step5_920908-1.c4c-disasm.log`
- `build/agent_state/393_step5_920908-1.c4c-bin-disasm.log`
- `build/agent_state/393_step5_920908-1.qemu-cpu.log`
