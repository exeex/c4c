# Prepared Object Data Static Storage Runtime

Status: Retired
Type: Implementation
Parent: `ideas/open/658_backend_baseline_history_umbrella_triage.md`
Related:
- `docs/backend_baseline_history_triage/failure_classification.md`
- `docs/backend_baseline_history_triage/follow_up_order.md`
Owning Layer: retired misclassification; evidence routed to RV64
callee-saved/live-value object-route runtime
Queue Order: 63
Proof Surface: current baseline rows 183 and 184 from
`log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`.

## Retirement Decision

Step 1 evidence exhausted this static-storage object-data route. Both focused
rows have coherent prepared object-data, static section layout, initializer
payload, object symbols, relocations, and linked data addresses before the
runtime mismatch.

The first observed mismatch is in RV64 object-route text/runtime consumption:
`main` copies the call result from `a0` into `t0`, then immediately overwrites
`t0` from stale `s2` before storing the saved values. The same object-route
failure appears for both the zero-initialized and explicitly initialized
static-local rows, while the assembly-route snippets do not contain the stale
`mv t0,s2` overwrite.

Rows 183 and 184 are therefore routed to
`ideas/open/666_rv64_callee_saved_gpr_runtime.md` rather than to static-storage
publication, layout, initializer placement, symbol binding, or relocation
repair.

## Original Goal

Repair prepared object-data and RV64 runtime handling for static local storage
and initialized static local storage.

## Why This Existed

Step 2 assigned two current object-runtime rows to prepared object-data/static
storage ownership. The family was narrower than the broad prepared/call-boundary
routes but should stay separate from packed local member offsets and generic
object emission.

## Final Evidence

- row 183,
  `backend_obj_runtime_rv64_prepared_object_data_static_local_storage`,
  returns `224` under QEMU instead of `11`.
- row 184,
  `backend_obj_runtime_rv64_prepared_object_data_static_local_initialized_storage`,
  returns `224` under QEMU instead of `24`.
- row 183 publishes `.bss`, `.balign 4`,
  `__static_local_rv64_step3_static_counter_0`, and `.zero 4`; the object has
  a 4-byte `.bss` section, an object symbol for the static local, and
  `R_RISCV_PCREL_HI20`/`R_RISCV_PCREL_LO12_I` relocations from both static
  load and store sites to that symbol.
- row 184 publishes `.data`, `.balign 4`,
  `__static_local_rv64_step4_static_initialized_counter_0`, and `.word 13`;
  the object has a 4-byte `.data` section containing `0d000000`, an object
  symbol for the static local, and the same PC-relative relocation shape from
  both static load and store sites.
- linked binary disassembly resolves the static-local accesses to concrete
  data addresses.

## Retired Scope

Do not reactivate this idea for rows 183 or 184 unless new evidence contradicts
the Step 1 finding and shows a first failure in prepared object-data
publication, static storage layout, initialization payload placement,
relocation, or symbol binding.

## Reviewer Reject Signals

- Reject reopening this static-storage route for rows 183 or 184 while the
  first mismatch remains stale RV64 object-route live-value consumption from
  `s2`.
- Reject patches to static-storage publication, layout, initializer bytes,
  object symbols, or relocations as progress for these rows unless refreshed
  evidence proves those facts are missing, stale, ambiguous, or mismatched.
- Reject named-case fixes for the static local or initialized static local
  storage rows.
- Reject expectation rewrites, unsupported-marker downgrades, allowlist edits,
  helper renames, or classification-only edits claimed as capability progress.
- Reject final assembly or object bytes as the only authority when prepared
  object-data facts are missing or ambiguous.
