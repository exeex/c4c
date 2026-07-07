Status: Active
Source Idea Path: ideas/open/563_bir_wide_vector_abi_signature_representation_owner_decision.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Broader Validation And Closure Handoff

# Current Packet

## Just Finished

Step 2 of `plan.md` selected and implemented the explicit BIR ABI carrier
contract for fixed-vector function signatures at the return-info and
parameter-layout publication boundary.

Selected representation rule:
- fixed vectors up to 4 bytes continue to publish as `I32` carriers
- fixed vectors over 4 and up to 8 bytes continue to publish as `I64` carriers
- fixed vectors over 8 and up to 16 bytes now publish as `I128` carriers
- fixed vectors over 16 bytes remain fail-closed in the function-signature
  semantic family because BIR has no `I256` or first-class vector ABI carrier

Implemented in `src/backend/bir/lir_to_bir/call_abi.cpp` by extending
`lower_fixed_vector_signature_carrier_type` to return `TypeKind::I128` for
16-byte LLVM vector signature types. Focused BIR coverage now asserts 16-byte
vector parameter and return publication as `I128` memory ABI carrier metadata,
32-byte vector return signatures as fail-closed function-signature diagnostics,
and the reviewer-requested void-return 32-byte vector parameter-only signature
as fail-closed at the parameter-layout owner boundary.

## Suggested Next

Supervisor should review/commit this slice or delegate the next packet for any
downstream 16-byte vector operation family that becomes reachable after
signature publication.

## Watchouts

The 16-byte contract deliberately uses the existing `I128` ABI behavior:
parameters publish `size_bytes=16`, `align_bytes=16`, `primary_class=Memory`,
`passed_on_stack=true`; returns publish `return_type=I128`,
`return_size_bytes=16`, `return_align_bytes=16`, and `returned_in_memory=true`.

32-byte LLVM vector signatures still stop at the function-signature owner
boundary. Do not admit them through split scalar pairs, VRM types, or
testcase-shaped matching without a new BIR carrier contract.

The 32-byte parameter-only fail-closed coverage is intentionally void-return so
return-info lowering cannot mask the parameter-layout owner boundary.

## Proof

Ran exact delegated proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed; `test_after.log` records `100% tests passed, 0 tests failed out
of 346`.
