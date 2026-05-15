Status: Active
Source Idea Path: ideas/open/232_aarch64_variadic_function_entry_carriers.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Validate And Summarize

# Current Packet

## Just Finished

Step 6, Validate And Summarize, reran broader backend validation and recorded
the current prepared-carrier route status for supervisor lifecycle review.

Route status:
- The prepared variadic-entry carrier route is now present through prepared BIR,
  AArch64 helper dispatch recognition, structured AArch64 helper-call records,
  printer diagnostics, and fail-closed consumption guards.
- The route validates the existence and completeness of prepared carrier facts
  before AArch64 helper lowering may proceed. It intentionally stops before
  target-local reconstruction of AAPCS64 storage or helper semantics.
- Broader backend validation passed with all `backend_` tests green, including
  the focused AArch64 variadic-entry guard, record, and printer coverage added
  by the preceding slices.

## Suggested Next

Supervisor should decide the lifecycle status for
`ideas/open/232_aarch64_variadic_function_entry_carriers.md`: close if prepared
entry carriers plus fail-closed AArch64 consumption guards satisfy the source
idea, mark blocked if downstream machine-node consumption is required by the
idea but lacks prepared storage/scratch authority, or leave open for a new
downstream AArch64 machine-node consumption packet.

## Watchouts

- `va_start`, scalar `va_arg`, aggregate `va_arg`, and `va_copy` remain
  unsupported at the final AArch64 machine-node consumption point. Current
  helper records preserve the prepared provenance and helper kind but still use
  `deferred_unsupported` for actual lowering.
- Storage and scratch-resource consumption remains unsupported because the
  prepared/frame authority does not yet allocate or expose complete AAPCS64
  facts such as register-save-area slot/offset, overflow-area base slot/offset,
  `va_list` layout storage, helper scratch counts, and any machine-node
  operands needed to materialize the helper side effects.
- A downstream consumption slice should consume prepared structured facts
  directly. It should not rebuild the AAPCS64 variadic ABI inside AArch64
  target lowering, and it should not claim support by weakening diagnostics or
  rewriting expectations around the known helper calls.

## Proof

Ran the delegated proof command:

`(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Result: passed. `test_after.log` contains the successful build and broader
backend CTest run: 139/139 tests passed.
