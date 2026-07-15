# Current Packet

Status: Active
Source Idea Path: ideas/open/783_lir_native_vaarg_operand_result_seam_decomposition.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Bind focused probes to native operand/result contracts

## Just Finished

- Resumed 783 at Plan Step 3 after 784 accepted the native three-chain carrier
  foundation at `e45a6b0ee`; `LirPhiIncoming.value` is value transport only and
  labels remain string-only.

## Suggested Next

- Bind one focused frontend-LIR structural probe to each AArch64 GP,
  AArch64 FP/alignment, and AMD64 register/stack contract using the accepted
  native carrier authority at its immediate consumer boundary.

## Watchouts

- Do not add PHI verification, predecessor/edge identity, CFG semantics, 782
  helper fields, HFA ptrmask work, Raw-BIR/importer, backend, target lowering,
  MIR, emission, generic migration, or text-derived identity.

## Proof

- 784 acceptance proof: `cmake --build --preset default && ctest --test-dir
  build -j --output-on-failure -R '^frontend_lir_call_type_ref$'` passed 1/1;
  matching `test_before.log`/`test_after.log` regression comparison passed
  with no new timeout or failure.
