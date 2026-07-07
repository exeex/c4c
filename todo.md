Status: Active
Source Idea Path: ideas/open/561_bir_function_signature_semantic_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Function-Signature Producer Boundary

# Current Packet

## Just Finished

Step 2 completed the focused structured aggregate return-info producer repair.
`src/backend/bir/lir_to_bir/call_abi.cpp` now accepts zero-sized structured
`Struct` layouts as valid signature aggregates when the layout resolver
publishes nonzero alignment, instead of rejecting them solely because the ABI
payload size is zero. Invalid layouts and zero-sized non-struct aggregate
shapes still fail closed.

Focused BIR coverage was added in
`tests/backend/bir/backend_prepare_structured_context_test.cpp` for an RV64
empty structured return signature. The coverage verifies that BIR lowering
publishes returned-in-memory return ABI metadata and a zero-byte, align-1 sret
formal parameter for `%struct.Empty`.

Direct representative probes now advance beyond the original function-signature
boundary without expectation, unsupported-marker, allowlist, or classification
changes:

- `src/zero-struct-2.c` / `one_raw_spinlock` now fails later in
  `alloca local-memory semantic family`.

## Suggested Next

Continue Step 2 only if the supervisor has another current
function-signature-specific representative. Otherwise move to Step 3 closure
handoff, because the named vector and zero-sized structured aggregate
representatives now advance to downstream scalar/local-memory or scalar-cast
owner boundaries.

## Watchouts

- Do not edit expectations, unsupported markers, allowlists, classifications,
  or the outer `latest function failure` note as evidence of progress.
- Do not route the newly exposed `alloca local-memory semantic family`,
  `scalar-cast semantic family`, or `scalar/local-memory semantic family`
  failures into this function-signature producer plan.
- The vector repair intentionally publishes a scalar integer ABI carrier for
  small fixed vectors; larger vector signatures such as `<4 x float>` or
  `<8 x i32>` still need a separate owner decision before claiming full vector
  signature closure.
- `src/20071029-1.c` currently reports `foo`, but its visible LLVM signature is
  scalar; keep it as a sample for aggregate temporary or parameter-layout
  follow-up rather than vector evidence.

## Proof

Ran:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`

Result: build completed and backend subset passed `346/346`.
Fresh proof log: `test_after.log`.
