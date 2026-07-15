# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.34
Current Step Title: Receive selected direct-pointer body-parameter authority

## Just Finished

- Closed 817 accepted the native direct non-expanded pointer authority handoff
  in `613f947b5`; 734's resumption record preserves the exact permitted row
  and its focused 1/1 proof.

## Suggested Next

- Execute Step 7.34 only: receive the selected typed-GEP-base direct-pointer
  parameter row into Raw-BIR with transactional positive/malformed coverage.

## Watchouts

- Use only `LirValueId`, parameter index, pointer `LirTypeRef`, current
  `LinkNameId` owner, and `LirNativeBodyParameterAbi::DirectPointer`. Do not
  use spelling, signature text, raw operands, or diagnostics; all nonselected
  parameter forms remain fail closed.

## Proof

- Fresh `cmake --build --preset default`, focused
  `ctest --test-dir build -j --output-on-failure -R '^backend_lir_to_bir_interface$'`,
  then supervisor-selected matching regression guard and broader proof.
