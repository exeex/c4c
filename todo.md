# Current Packet

Status: Active
Source Idea Path: ideas/open/770_lir_to_bir_native_label_address_constant_contract.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove owned contract and return

## Just Finished

- Step 3 proved the owned native label-address constant contract with the
  existing focused interface fixture: a direct function-owned constant lowers
  to a verified Raw-BIR source value consumed by `IndirectJumpTerm`, while
  invalid/missing/duplicate produced identities, nonpointer type,
  invalid/foreign authority, and mismatched direct-use/address cases reject.
  No duplicate-only test edit was needed, and no 768 producer recovery was
  performed.

## Suggested Next

- Supervisor to return 768 to its preserved Step 5 direct producer recovery
  packet using this accepted native contract.

## Watchouts

- LLVM 19 does not accept `%x = blockaddress(...)` as an instruction. The
  accepted route remains a function-owned constant; do not broaden this
  completed contract into synthetic bridges, producer recovery, legacy BIR,
  preparation/MIR/codegen, or carrier publication.

## Proof

- Passed: `cmake --build --preset default`; then `ctest --test-dir build -j
  --output-on-failure -R '^backend_lir_to_bir_interface$' > test_after.log`.
  The focused proof is sufficient for this bounded packet; log:
  `test_after.log`.
