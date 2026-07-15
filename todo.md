# Current Packet

Status: Active
Source Idea Path: ideas/open/817_lir_body_parameter_receiver_authority_handoff.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove and record the 734 handoff

## Just Finished

- 817 Step 2 completed: `LirCurrentFunctionBodyParameterDefinition` now
  carries an explicit `LirNativeBodyParameterAbi::DirectPointer` class, which
  the producer publishes only for the existing native non-expanded pointer
  predicate. The verifier requires that class alongside the current function,
  identity, parameter index, pointer type, and structured signature type; a
  `LirGepOp.ptr` can use the carrier only with that class.
- Focused coverage verifies the selected typed-GEP-base positive path and
  rejects missing, invalid, foreign, duplicate, type-incoherent, and
  malformed-ABI authority. Other parameter forms remain unrepresented and
  fail closed.

## Suggested Next

- Execute 817 Step 3 only: record the selected direct-pointer authority
  contract, rejected forms, focused proof, and the one-row 734 return point;
  do not add Raw-BIR receipt work.

## Watchouts

- The selected row remains only native direct non-expanded pointer
  `p[0]`/typed-GEP-base use. Scalar, byval/aggregate, HFA/vector, array,
  variadic, and all other parameter forms remain fail-closed; no Raw-BIR or
  receiver code was changed.

## Proof

- `cmake --build --preset default` passed. `ctest --test-dir build -j
  --output-on-failure -R '^backend_lir_selected_pointer_authority$'` passed;
  proof output is preserved in `test_after.log`.
