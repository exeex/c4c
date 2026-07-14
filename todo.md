# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.5
Current Step Title: Receive the checked ordinary scalar floating binary result

## Just Finished

- Step 7.5 complete: received only the producer-verified ordinary scalar
  `double` `LirBinOp` chain: native `FAdd` from the admitted direct-call result
  followed by native `FMul` whose lhs is that FAdd result. The importer, core
  builder, and Raw-BIR verifier preserve opcode/type and current-function
  source IDs, reject malformed/missing/invalid/duplicate/unresolved/cross-owner
  linkage transactionally, and do not admit general floating binary handling.

## Suggested Next

- Supervisor should select the next active-plan packet. Any follow-on receiver
  must remain bounded to its producer-verified source authority and preserve
  whole-module rejection; this packet does not authorize general floating
  binary, literal, comparison, cast, vector, pointer, or call expansion.

## Watchouts

- This is an in-scope runbook repair, not source completion: the coverage
  matrix still identifies receiver-ready rows. Step 7.5 admits only the
  checked ordinary floating FAdd-to-FMul chain; unsupported non-scalar and
  presentation-derived forms remain fail-closed.

## Proof

- Step 7.5 passed the supervisor-selected proof: `cmake --build --preset
  default && ctest --test-dir build -j --output-on-failure -R
  '^backend_lir_to_bir_interface$|^frontend_lir_call_type_ref$'` (2/2);
  proof log: `test_after.log`.
