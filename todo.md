Status: Active
Source Idea Path: ideas/open/583_rv64_pointer_arithmetic_result_publication.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Add Focused Pointer Publication Coverage

# Current Packet

## Just Finished

Step 2 added focused RV64 object-emission coverage for pointer-valued
`add` and `sub` where a prepared pointer base register combines with a prepared
integer byte-offset register and produces a prepared pointer destination home.
The fixture now includes a later pointer-value local-memory store through the
published `%result.ptr` owner, so Step 3 must materialize and publish the
pointer result rather than only returning it.

The current suite remains green by pinning the precise
`unsupported_pointer_arithmetic` diagnostic at the pointer-valued `BinaryInst`
for both add and sub shapes. This preserves the missing-publication contract
without implementing the repair in this packet.

## Suggested Next

Execute Step 3 from `plan.md`: repair RV64 object emission for supported
prepared pointer-result add/sub by materializing the pointer base plus integer
byte offset into the prepared destination home, then allow later pointer-value
local-memory consumers to use that published owner.

## Watchouts

- Do not select or mutate deferred `ideas/open/560_bir_scalar_signature_control_semantic_producer_admission.md`.
- The focused fixture is semantic: do not repair it with filename, function,
  block, value-name, or diagnostic-string shortcuts.
- Preserve fail-closed behavior for pointer arithmetic forms outside prepared
  pointer base plus integer byte-offset add/sub with a prepared destination
  home.
- The later local-memory store is there to prove destination publication; a
  repair that only computes a transient address but does not publish
  `%result.ptr` is incomplete.

## Proof

`test_after.log` records the delegated proof:

`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$'; } > test_after.log 2>&1`

Result: passed, `backend_riscv_object_emission` 1/1.
