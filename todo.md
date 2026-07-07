Status: Active
Source Idea Path: ideas/open/561_bir_function_signature_semantic_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Broader Validation And Closure Handoff

# Current Packet

## Just Finished

Step 3 refreshed closure handoff evidence for the function-signature producer
plan after the Step 2 repairs and fail-closed decisions. The named small-vector
and empty-struct representatives now advance beyond the original
`function-signature semantic family` boundary without expectation,
unsupported-marker, allowlist, or classification changes:

- `src/20050316-3.c` / `test1` advances past the function-signature boundary
  and now stops at `scalar-cast semantic family`.
- `src/pr60960.c` / `f1` advances past the function-signature boundary and now
  stops at `scalar/local-memory semantic family`.
- `tests/c/external/gcc_torture/src/zero-struct-2.c` / `one_raw_spinlock`
  advances past the function-signature boundary and now stops at
  `alloca local-memory semantic family`.
- `src/simd-6.c` / `foo` advances past the function-signature boundary and now
  stops at `scalar-binop semantic family`.

Wide LLVM vector signatures remain an explicit fail-closed
function-signature producer boundary pending a separate ABI representation
owner decision. Focused BIR coverage documents that `<8 x i32>` return-info
lowering and `<4 x float>` parameter-layout lowering fail closed at
`function-signature semantic family` rather than publishing misleading scalar,
VRM, or ad hoc multi-register ABI metadata.

The remaining named wide-vector representatives are still signature-bound for
that reason:

- `src/ieee/pr72824-2.c` / `foo` still fails in `function-signature semantic
  family` with visible LLVM signature `define internal void @foo(ptr %p.x,
  <4 x float> %p.value)`.
- `src/pr70903.c` / `foo` still fails in `function-signature semantic family`
  with visible LLVM signature `define internal <8 x i32> @foo(<4 x i64>
  %p.x) noinline`.

Closure handoff evidence: the repaired small-vector and empty-struct producer
gaps no longer block BIR signature publication; the still-failing wide-vector
cases are deliberate fail-closed owner-boundary evidence, not an accidental
producer hole in the currently supported signature carrier set. Newly exposed
failures belong to downstream `scalar-cast`, `scalar/local-memory`,
`alloca local-memory`, and `scalar-binop` owner families.

## Suggested Next

Ask the plan owner to decide whether to close the function-signature producer
source idea, with a separate open idea or owner decision for wide LLVM vector
ABI representation if the project wants to admit 16-byte and 32-byte vector
signatures.

## Watchouts

- Do not edit expectations, unsupported markers, allowlists, classifications,
  or the outer `latest function failure` note as closure evidence.
- Do not route the newly exposed `scalar-cast`, `scalar/local-memory`,
  `alloca local-memory`, or `scalar-binop` failures into this
  function-signature producer plan.
- Larger vector signatures such as `<4 x float>`, `<8 x i32>`, and `<4 x i64>`
  need a separate ABI representation owner decision before signature admission;
  mapping them to `I128`, scalar integer pairs, or `Vrm*` inside `call_abi.cpp`
  would cross into downstream ABI/object ownership without a real carrier
  contract.
- `src/20071029-1.c` currently reports `foo`, but its visible LLVM signature is
  scalar; keep it as a sample for aggregate temporary or parameter-layout
  follow-up rather than vector evidence.

## Proof

Ran:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`

Result: build completed and backend subset passed `346/346`.
Fresh proof log: `test_after.log`.
