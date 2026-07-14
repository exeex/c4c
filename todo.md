# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.5
Current Step Title: Receive the checked ordinary scalar floating binary result

## Just Finished

- Step 7.4 complete: received only the resolved external `double target(void)`
  direct-call result and its exact later double `FAdd` lhs use. The importer
  requires one bodyless module declaration with the shared LinkNameId, native
  double return, and structured fixed-void signature; it preserves the caller
  result/source ordering and rejects non-double, unresolved, duplicate,
  malformed, or cross-owner call/use authority without admitting FAdd broadly.

## Suggested Next

- Execute Step 7.5 as one bounded receiver packet: import only the
  producer-verified ordinary scalar floating `LirBinOp` double FAdd-to-FMul
  chain using native opcode/type and source IDs, with transactional positive
  and neighboring negative proof. Do not receive floating literal authority,
  comparisons, casts, calls beyond the already admitted fixed-void result, or
  any complex/vector/pointer/logical-helper/compound/builtin form.

## Watchouts

- This is an in-scope runbook repair, not source completion: the coverage
  matrix still identifies receiver-ready rows. Step 7.5 admits only the
  checked ordinary floating chain; unsupported literal, non-scalar, and
  presentation-derived forms remain fail-closed.

## Proof

- Step 7.4 passed: `cmake --build --preset default && ctest --test-dir build
  -j --output-on-failure -R
  '^backend_lir_to_bir_interface$|^frontend_lir_call_type_ref$'` (2/2);
  proof log: `test_after.log`; the supervisor's broader `^backend_` regression
  guard passed 4/4 before and after. Step 7.5 must use the proof selected
  by the supervisor for its bounded implementation packet.
