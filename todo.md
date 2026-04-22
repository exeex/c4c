# Execution State

Status: Active
Source Idea Path: ideas/open/75_post_link_prepared_call_lane_clobber_runtime_correctness_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The Overlapping Call-Lane Source-Preservation Seam
Plan Review Counter: 4 / 5
# Current Packet

## Just Finished

Plan step `2` advanced through the direct-global variadic aggregate import seam
in `src/backend/bir/lir_to_bir_calling.cpp` and the focused prepare regression
in `tests/backend/backend_prepare_liveness_test.cpp`.
The variadic direct-global parser now canonicalizes `ptr byval(T)` operands
back to `T` instead of letting them fall through the generic pointer lane, so
same-module `stdarg()->myprintf` calls preserve explicit byval ABI metadata and
the original payload identity during LIR-to-BIR import.
The new x86-focused prepare regression
`lowered_same_module_variadic_global_byval_metadata` proves that a same-module
variadic `@myprintf` call keeps the aggregate payload on `@gpair` while
publishing `ptr byval(size=8, align=4)` ABI facts. The older helper aggregate
prepare assertion was also tightened to accept the current authoritative x86
ABI destination contract instead of assuming the byval lane must always publish
only a stack destination.
Fresh proof keeps `backend_prepare_liveness` and
`backend_x86_handoff_boundary` green. The focused runtime case is still red,
but the owned importer seam is now repaired: `--dump-prepared-bir
--mir-focus-function stdarg` shows `bir.call void myprintf(...)` sites carrying
the preserved `ptr byval(size=..., align=...)` metadata for the direct-global
aggregate payloads (`@s9`, `@hfa34`, `@hfa24`, `@hfa13`, `@hfa12`, `@hfa11`).
The remaining blocker is downstream and outside the owned files: the x86
runtime path still corrupts those aggregate/HFA consumers after import, with
the first visible mismatch in `test_after.log` now appearing in the
`Arguments:` HFA long-double section before the later `Return values:` and
`stdarg` fallout.

## Suggested Next

Stay in plan step `2`, but hand the next packet to the downstream x86
same-module aggregate consumer path outside this packet's ownership.
The importer now hands `stdarg()->myprintf` calls to prepare/x86 with correct
`ptr byval(size=..., align=...)` facts and payload identities intact, so the
next coherent slice should start from the first still-bad x86 runtime consumer
of those aggregates, beginning with the HFA long-double argument corruption
that now shows up first in `test_after.log`.

## Watchouts

- `backend_prepare_liveness` now covers the new same-module variadic global
  byval import contract. Keep that regression narrow: it is guarding the
  importer seam, not the downstream x86 renderer.
- The prepared `stdarg` dump is the authoritative proof that this packet worked:
  `bir.call void myprintf(...)` now prints preserved `ptr byval(size=..., align=...)`
  metadata for the direct-global aggregate varargs. Do not reopen the importer
  unless that prepared dump loses those annotations again.
- `backend_x86_handoff_boundary` is still green, so the remaining blocker is
  later than prepared handoff publication.
- The remaining runtime failure is outside the owned files. The first visible
  mismatch in `test_after.log` is now the HFA long-double `Arguments:` block,
  followed by corrupted `Return values:` and then an empty `stdarg:` section,
  which points at downstream x86 aggregate-consumption work rather than more
  importer metadata loss.

## Proof

Fresh focused proof for this importer packet:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_liveness|backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00204_c)$' | tee test_after.log`
Current proof state:
- `backend_prepare_liveness`: passed, including the new
  `lowered_same_module_variadic_global_byval_metadata` regression
- `backend_x86_handoff_boundary`: passed
- `c_testsuite_x86_backend_src_00204_c`: failed with
  `[RUNTIME_MISMATCH]`; the prepared `stdarg` handoff now preserves byval
  metadata for the direct-global aggregate varargs, but the first visible
  downstream runtime mismatch in `test_after.log` is still the HFA long-double
  `Arguments:` section, followed by corrupted `Return values:` and `stdarg`
  output. That blocker is downstream of the owned importer seam
- Proof log path: `test_after.log`
