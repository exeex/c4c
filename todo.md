# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.6
Current Step Title: Receive the checked ordinary scalar floating compare result

## Just Finished

- Step 7.5 complete: received only the producer-verified ordinary scalar
  `double` `LirBinOp` chain: native `FAdd` from the admitted direct-call result
  followed by native `FMul` whose lhs is that FAdd result. The importer, core
  builder, and Raw-BIR verifier preserve opcode/type and current-function
  source IDs, reject malformed/missing/invalid/duplicate/unresolved/cross-owner
  linkage transactionally, and do not admit general floating binary handling.

## Suggested Next

- Execute Step 7.6 as one bounded receiver packet: import only the
  producer-verified ordinary scalar floating double OLt comparison and its
  compatibility-result ZExt use through native mode, predicate, type, and
  current-function IDs. Do not receive the ZExt result or widen compare,
  literal, complex/vector/pointer/logical-helper/builtin, or text-derived
  authority.

## Watchouts

- This is an in-scope runbook repair, not source completion: the coverage
  matrix still identifies receiver-ready rows. Step 7.6 admits only the
  checked floating comparison boundary; unsupported non-scalar and
  presentation-derived forms remain fail-closed.

## Proof

- Step 7.5 passed the supervisor-selected proof: `cmake --build --preset
  default && ctest --test-dir build -j --output-on-failure -R
  '^backend_lir_to_bir_interface$|^frontend_lir_call_type_ref$'` (2/2);
  proof log: `test_after.log`. The matching regression guard passed 2/2
  before/after, and fresh broader `^backend_` proof passed 4/4.
