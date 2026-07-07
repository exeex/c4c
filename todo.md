Status: Active
Source Idea Path: ideas/open/561_bir_function_signature_semantic_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Function-Signature Producer Boundary

# Current Packet

## Just Finished

Step 2 completed the first focused function-signature producer repair for
fixed-size vector signatures. `src/backend/bir/lir_to_bir/call_abi.cpp` now
admits fixed LLVM vector signature types whose scalar lanes fit in an integer
carrier up to 8 bytes, publishing `i32` or `i64` return-info and parameter ABI
metadata instead of rejecting the signature boundary.

Focused BIR coverage was added in
`tests/backend/bir/backend_prepare_structured_context_test.cpp` for structured
signature metadata on `<2 x i32>` and `<4 x i8>` return/parameter signatures.
The coverage verifies that both return ABI metadata and formal parameter ABI
metadata are published with the carrier type.

Direct representative probes now advance beyond the original function-signature
boundary without expectation, unsupported-marker, allowlist, or classification
changes:

- `src/20050316-3.c` / `test1` now fails later in `scalar-cast semantic family`.
- `src/pr60960.c` / `f1` now fails later in `scalar/local-memory semantic family`.

## Suggested Next

Continue Step 2 with the next focused function-signature producer repair:
structured aggregate return-info admission, starting with a representative such
as `src/zero-struct-2.c` / `one_raw_spinlock`, and keep the work limited to
return-info or parameter-layout publication in the BIR signature producer path.

## Watchouts

- Do not edit expectations, unsupported markers, allowlists, classifications,
  or the outer `latest function failure` note as evidence of progress.
- Do not route the newly exposed `scalar-cast semantic family` or
  `scalar/local-memory semantic family` failures into this function-signature
  producer plan.
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
