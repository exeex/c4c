# Current Packet

Status: Active
Source Idea Path: ideas/open/773_lir_gep_direct_label_address_constant_contract.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Carry the validated form through printer and backend/lowering

## Just Finished

- Step 2 complete: the LIR printer renders only the table-backed current-function
  direct-label-address GEP base, and LIR-to-Raw-BIR lowers that validated
  `DirectConstant(LirValueId)` as 774's typed `LabelAddressGepBase` without
  display recovery, fabricated SSA, or global projection. Nearby interface
  coverage proves both the accepted base identity and rejection of an arbitrary
  direct constant.

## Suggested Next

- Supervisor: select the next active-plan packet. Do not edit 772
  `emit_indexed_gep` forwarding or pr70460 as part of this completed Step 2 slice.

## Watchouts

- Direct GEP identity is table-backed only. Do not recover labels from text,
  fabricate SSA/globals, coerce to a global base, or widen generic GEP
  authority. The rejected 3037/1 pr70460 baseline remains out of scope.

## Proof

- Passed fresh `cmake --build --preset default` and
  `ctest --test-dir build -j --output-on-failure -R '^backend_'` (5/5).
  The required proof log is `test_after.log`. Do not refresh or accept
  `test_baseline.new.log`; 772 owns the later pr70460 forwarding repair and
  baseline route.
