Status: Active
Source Idea Path: ideas/open/561_bir_function_signature_semantic_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Function-Signature Producer Boundary

# Current Packet

## Just Finished

Step 2 resolved the larger-vector producer decision as a deliberate fail-closed
function-signature boundary rather than admitting a misleading scalar or VRM
carrier. `call_abi.cpp` already has a general small fixed-vector carrier rule
for vectors up to 8 bytes; BIR has no current signature ABI carrier that
faithfully represents wider LLVM vector return/parameter layouts such as
`<4 x float>`, `<8 x i32>`, or `<4 x i64>` without making a downstream
multi-register or memory ABI ownership decision.

Focused BIR coverage was added in
`tests/backend/bir/backend_prepare_structured_context_test.cpp` to document the
boundary: `<8 x i32>` return-info lowering and `<4 x float>` parameter-layout
lowering both fail closed at `function-signature semantic family`.

Direct representative probes were refreshed without expectation,
unsupported-marker, allowlist, or classification changes:

- `src/ieee/pr72824-2.c` / `foo` still fails in `function-signature semantic
  family` with visible LLVM signature `define internal void @foo(ptr %p.x,
  <4 x float> %p.value)`.
- `src/pr70903.c` / `foo` still fails in `function-signature semantic family`
  with visible LLVM signature `define internal <8 x i32> @foo(<4 x i64>
  %p.x) noinline`.
- `src/simd-6.c` / `foo` now advances beyond the original function-signature
  boundary and fails later in `scalar-binop semantic family`; its visible LLVM
  signature is `define <8 x i8> @foo(<8 x i8> %p.x, <8 x i8> %p.y)`.

## Suggested Next

Move to Step 3 closure handoff for the function-signature producer plan, or
open a separate owner decision for wide LLVM vector ABI representation before
trying to admit 16-byte and 32-byte vector signatures.

## Watchouts

- Do not edit expectations, unsupported markers, allowlists, classifications,
  or the outer `latest function failure` note as evidence of progress.
- Do not route the newly exposed `alloca local-memory semantic family`,
  `scalar-cast semantic family`, or `scalar/local-memory semantic family`
  failures into this function-signature producer plan.
- Do not route the newly exposed `scalar-binop semantic family` failure in
  `src/simd-6.c` into this function-signature producer plan.
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
