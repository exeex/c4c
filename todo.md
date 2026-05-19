# LIR To BIR Local Memory Admission Todo

Status: Active
Source Idea Path: ideas/open/297_lir_to_bir_local_memory_admission.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Local-Memory GEP Admission

# Current Packet

## Just Finished

Step 2 repaired the direct local-array GEP owner:
`try_lower_local_array_slot_gep` now admits dynamic scalar local-array GEPs by
publishing `DynamicLocalAggregateArrayAccess` state for non-pointer element
arrays. The helper still routes pointer-element arrays through
`DynamicLocalPointerArrayAccess`; scalar arrays now publish rendered element
type, element count, stride, lowered dynamic index, and offset-keyed element
leaf slots matching the existing local pointer-array-base pattern.

The delegated proof shows representative movement past the old GEP admission
block:

- `00143`: moved from `gep local-memory semantic family` to AArch64 machine
  printer immediate encoding failure.
- `00157`: moved from `gep local-memory semantic family` to runtime mismatch.
- `00185`: moved from `gep local-memory semantic family` to runtime mismatch.

Residual focused failures in `test_after.log`:

- Store boundary, unchanged for Step 3 scope: `00046`, `00140`.
- Load boundary, unchanged for Step 3 scope: `00216`, `00218`.
- GEP family still present for non-representative or different shapes:
  `00176`, `00181`, `00182`, `00195`, `00205`, `00209`.

## Suggested Next

Keep Step 2 focused if the supervisor wants another GEP packet: classify one
remaining `gep local-memory semantic family` case such as `00176` or `00181`
and repair the next semantic GEP shape without expectation or runner changes.
If the supervisor switches to Step 3, start with the already-isolated
aggregate store/load boundary failures (`00046`, `00140`, `00216`, `00218`).

## Watchouts

- Direct dynamic scalar local-array admission is no longer the blocker for
  representatives `00143`, `00157`, and `00185`; do not conflate their new
  machine/runtime failures with the old GEP admission failure.
- Focused GEP local-memory cases still failing in GEP family: `00176`,
  `00181`, `00182`, `00195`, `00205`, `00209`.
- Store/load boundary checks: `00046`, `00140`, `00216`, `00218`.
- `00204` is a separate bootstrap global aggregate/array semantics gate; do
  not fold it into this owner without evidence.
- Do not change expectations, allowlists, unsupported classifications, CTest
  registration, timeout policy, runner behavior, `test_after.log`, or
  `test_before.log`.
- Do not claim progress through filename matching or named-case shortcuts.
- Step 2 should not special-case `00143`; the rejected form is semantic:
  dynamic GEP into scalar local arrays whose elements are represented as local
  slots.
- Store/load boundary notes for Step 3: `00046` needs nested aggregate
  subobject scalar stores through local address provenance; `00216` needs
  aggregate load/copy from pointer-derived local memory.

## Proof

Ran the delegated proof exactly:

`cmake --build build --target c4cll backend_lir_to_bir_notes_test && ctest --test-dir build -R '^(backend_lir_to_bir_notes|c_testsuite_aarch64_backend_src_(00143|00157|00176|00181|00182|00185|00195|00205|00209|00046|00140|00216|00218)_c)$' --output-on-failure | tee test_after.log`

Result: build passed; CTest reported 1/14 passed and 13/14 failed.
`test_after.log` is the canonical proof log for this packet.
