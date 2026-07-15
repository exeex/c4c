# Current Packet

Status: Active
Source Idea Path: ideas/open/823_lir_next_body_parameter_authority_handoff.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish and verify one DirectScalar binary-RHS authority contract

## Just Finished

- Step 2 published one `LirBinOp.rhs` DirectScalar authority contract: the
  producer binds a distinct RHS carrier only from a matching current-function
  definition, and the verifier requires RHS role, value, owner, parameter
  index, type, and ABI coherence. Nearby LIR-only coverage accepts the exact
  tuple and rejects missing, foreign, wrong-role, type, ABI, and value-mismatch
  variants; no Raw-BIR receipt was added.

## Suggested Next

- Complete Step 3 by recording this exact RHS producer/schema/verifier tuple,
  rejected forms, and focused proof in the source idea, then hand the bounded
  receiver contract back to 734. Do not begin Raw-BIR receiver work here.

## Watchouts

- Do not infer RHS authority from `LirBinOp.rhs`, operand spelling, or the LHS
  carrier; the distinct RHS carrier is the only selected authority source.
- Ideas 821 and 822 retain pending, unaccepted implementation work; do not
  modify, discard, or claim acceptance for either slice.
- No generic scalar/parameter admission, Raw-BIR/importer/builder work, or
  presentation-derived recovery is authorized.
- The accepted pointer and DirectScalar binary-LHS rows are historical
  progress; do not reopen or repeat them.

## Proof

- `cmake --build --preset default --target backend_lir_selected_pointer_authority_test && ctest --test-dir build --output-on-failure -R '^backend_lir_selected_pointer_authority$'` passed; output is preserved in `test_after.log`.
