# Current Packet

Status: Active
Source Idea Path: ideas/open/780_lir_cross_function_value_id_ownership_restoration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Restore regression baseline and return to 778 boundary

## Just Finished

- Step 3 — Prove nearby multi-function authority behavior: replaced the
  obsolete separate-function same-ID expectation with structural load checks
  across `read_counter`, `read_pair`, and `read_counter_again`. The test now
  requires distinct valid module-wide native IDs while preserving each
  function's result display, load type, selected global, and verifier-valid
  current-function semantics. Existing standalone native-cast coverage retains
  the flag-gated missing, invalid, same-function duplicate, and foreign-owner
  rejection contract.

## Suggested Next

- Step 4: restore the matching regression baseline and return to the 778
  boundary without widening the 780 ownership slice.

## Watchouts

- The focused proof now covers real lowered module-wide value IDs and the
  existing flag-gated fail-closed foreign-owner contract. Do not absorb
  PHI/generic producer migration while restoring the regression boundary.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R
  '^(frontend_lir_call_type_ref|llvm_gcc_c_torture_src_pr52129_c)$'` passed:
  `frontend_lir_call_type_ref` and `llvm_gcc_c_torture_src_pr52129_c` both
  pass (2/2). No canonical root log was written by this packet; supervisor
  owns root logs.
