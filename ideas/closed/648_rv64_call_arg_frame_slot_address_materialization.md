# RV64 Call-Argument Frame-Slot Address Materialization

Status: Closed
Type: Implementation
Parent: `ideas/closed/638_rv64_string_label_pointer_runtime_object_correctness.md`
Related:
- `ideas/closed/656_20000722_local_memory_access_object_route.md`
- `ideas/closed/638_rv64_string_label_pointer_runtime_object_correctness.md`
- `ideas/closed/630_string_constant_local_memory_policy.md`
- `ideas/closed/618_runtime_mismatch_ownership_investigation.md`
Owning Layer: RV64 call-argument lowering for selected local frame-slot address values
Queue Order: 48
Prerequisites: prepared call planning must explicitly identify an argument
source selection as `local_frame_address_materialization`.
Proof Surface: `src/20000722-1.c` and a narrow backend assertion for
`arg.source_selection=local_frame_address_materialization`

Lifecycle Note: Reactivated after
`ideas/closed/656_20000722_local_memory_access_object_route.md` repaired the
earlier `unsupported_local_memory_access` blocker. The representative
`src/20000722-1.c` object route now emits an object, and a fresh asm snapshot
again reaches call-argument setup with a stale `mv a0, s1` copy. Focused
text-route coverage for `arg.source_selection=local_frame_address_materialization`
remains green, so the next work is to classify and repair the representative
call-argument lowering mismatch without reopening local-memory policy.

Completion Note: Closed after repairing the RV64 object-route call-argument
consumer for explicit `local_frame_address_materialization` source selections.
Focused dump/text/object coverage proved direct frame-slot address
materialization into `a0`; representative `src/20000722-1.c` object evidence
showed the `bar` call now passes `mv a0, sp` instead of the stale source-home
copy. Close guard passed on matched RV64 before/after logs:
before passed=88 failed=18 total=106, after passed=89 failed=18 total=107,
new failing tests=0.

## Goal

Repair RV64 call-argument lowering so a call argument whose prepared source is
an address-materialized local frame-slot value lowers to the selected
frame-slot address calculation, not to a stale register-home copy.

## Why This Exists

Research idea 638 originally indicated that `src/20000722-1.c` had advanced
past string-label pointer admission, local/global memory, stack layout, ABI
destination convention, branch/control flow, and true runtime support into a
call-argument lowering mismatch. Prepared BIR and the prepared call plan select
the local frame-slot address for `%lv._clit_` and record
`arg.source_selection=local_frame_address_materialization`.

Fresh focused coverage now proves the RV64 text-route call-argument consumer
sets up the ABI argument with a frame-slot address calculation rather than a
stale register-home copy. The representative object route previously failed
earlier with `unsupported_local_memory_access`; idea 656 repaired that blocker.
The representative row now reaches call-argument setup again, where fresh asm
evidence still shows a stale `mv a0, s1` copy.

## In Scope

- Locate the RV64 call-argument lowering path that consumes prepared
  `arg.source_selection=local_frame_address_materialization`.
- Lower that argument source as a selected local frame-slot address
  calculation into the ABI destination register.
- Add or update a narrow backend assertion proving that this source selection
  does not lower through a register-home copy.
- Use `src/20000722-1.c` as the representative runtime/object proof surface.
- Preserve fail-closed behavior when the prepared call plan lacks an explicit
  selected frame-slot address source.
- Continue this reactivated route only while the renewed representative owner
  is call-argument frame-slot address materialization.

## Out Of Scope

- Reopening string-constant local-memory admission or broadening
  `StringConstantLabelPointer` policy.
- Generic runtime support, branch/control-flow lowering, stack layout changes,
  or ABI destination-register convention changes.
- Source-file-specific handling for `src/20000722-1.c`.
- Expectation, unsupported-marker, allowlist, timeout, runtime-comparison, or
  pass/fail accounting changes.

## Acceptance Criteria

- A narrow backend assertion proves that
  `arg.source_selection=local_frame_address_materialization` lowers to a
  selected frame-slot address calculation rather than a register-home copy.
- `src/20000722-1.c` advances through the bad `mv a0,s2` failure mode, with
  object/disassembly evidence showing the call receives the selected local
  frame-slot address.
- Negative proof keeps missing, ambiguous, or non-frame-slot call argument
  sources fail-closed rather than guessing from stack offsets or source syntax.
- No test expectation, unsupported-marker, allowlist, timeout/accounting, or
  runtime-comparison change is claimed as capability progress.

## Reviewer Reject Signals

- Reject filename-only or shape-only fixes for `src/20000722-1.c`, `%lv._clit_`,
  `foo`, `s2`, or `a0` without consuming the semantic prepared call-argument
  source selection.
- Reject lowering that reconstructs the frame-slot address from incidental
  final assembly, source syntax, or stack-offset guesses instead of explicit
  prepared frame-slot address authority.
- Reject leaving `arg.source_selection=local_frame_address_materialization`
  on the old `call_arg_register_to_register` path while renaming helpers or
  changing diagnostics.
- Reject broad ABI, stack-layout, local-memory, string-literal, branch, or
  runtime rewrites that do not prove this call-argument source boundary.
- Reject expectation, unsupported-marker, allowlist, timeout/accounting,
  runtime-comparison, or pass/fail accounting changes as progress.
