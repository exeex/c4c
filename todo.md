# Current Packet

Status: Active
Source Idea Path: ideas/open/769_lir_global_initializer_label_address_authority.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove the contract and assess the Raw-BIR boundary

## Just Finished

- Step 2 completed: `LirGlobal::initializer_elements` now publishes each
  static label-address initializer as `{LinkNameId enclosing_function,
  LirBlockId target}` from the HIR constant/global-lowering walk. The LIR
  verifier requires exactly one matching function owner and exactly one block
  in that function. Direct frontend-LIR coverage proves structured positive
  publication and malformed owner/target rejection.

## Suggested Next

- Run Step 3 acceptance and assess the named Raw-BIR/importer boundary only:
  determine whether a separate downstream blocker is required before returning
  the parked 768 work to its next route. Do not implement importer/backend,
  direct/local, or carrier behavior in that assessment.

## Watchouts

- `init_text` remains display/compatibility spelling and is not used as
  label-target authority. Raw-BIR/importer global lowering still has no field
  for the structured element, so any integration consumption remains a
  separately scoped downstream blocker.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^frontend_lir_' > test_after.log` passed (5/5,
  including `frontend_lir_global_label_address_initializer`); log:
  `test_after.log`.
