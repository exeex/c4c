Status: Active
Source Idea Path: ideas/open/561_bir_function_signature_semantic_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Function-Signature Evidence

# Current Packet

## Just Finished

Step 1 refreshed the function-signature producer evidence. The representative
RV64 object-runner set still stops before prepared object handoff at the BIR
function-signature boundary:

- `src/20050316-3.c` / `test1`: fails in `function-signature semantic family`;
  emitted LLVM signature is `define <2 x i32> @test1(<2 x i32> %p.x)`.
- `src/20071029-1.c` / `foo`: fails in `function-signature semantic family`;
  emitted LLVM signature is `define void @foo(i32 %p.i)`.
- `src/ieee/pr72824-2.c` / `foo`: fails in
  `function-signature semantic family`; emitted LLVM signature is
  `define internal void @foo(ptr %p.x, <4 x float> %p.value)`.
- `src/pr60960.c` / `f1`: fails in `function-signature semantic family`;
  emitted LLVM signature is `define <4 x i8> @f1(<4 x i8> %p.v)`.
- `src/pr70903.c` / `foo`: fails in `function-signature semantic family`;
  emitted LLVM signature is `define internal <8 x i32> @foo(<4 x i64> %p.x)`.
- `src/pr71626-1.c` / `foo` and `src/pr71626-2.c` / `foo`: fail in
  `function-signature semantic family`; emitted LLVM signature is
  `define <1 x i64> @foo()`.
- `src/simd-6.c` / `foo`: fails in `function-signature semantic family`;
  emitted LLVM signature is `define <8 x i8> @foo(<8 x i8> %p.x, <8 x i8> %p.y)`.
- `src/zero-struct-2.c` / `one_raw_spinlock`: fails in
  `function-signature semantic family`; emitted LLVM signature is
  `define %struct._anon_0 @one_raw_spinlock()`.

Existing focused BIR tests already cover structured signature metadata
preference and fail-closed incoming byval layout cases, so the live gap is not
the outer `latest function failure` publication text. Refreshed evidence exposes
real Step 2 producer work in return-info and parameter-layout admission,
especially vector return/parameter signatures and structured aggregate return
info, rather than closure readiness or a downstream RV64 owner boundary.

## Suggested Next

Execute Step 2 with a focused repair in `src/backend/bir/lir_to_bir/call_abi.cpp`
for function-signature return-info and parameter-layout publication. Start with
general vector signature support in `lower_signature_return_info()` /
`lower_function_params_with_layouts()`, then prove one vector-return row
(`src/20050316-3.c` or `src/pr60960.c`) advances beyond the original BIR
signature boundary without expectation or allowlist changes.

## Watchouts

- Do not edit expectations, unsupported markers, allowlists, classifications,
  or the outer `latest function failure` note as evidence of progress.
- Do not route this packet into scalar-control-flow, scalar-binop, RV64 ABI, or
  object-emission work before BIR signature publication is proven correct.
- `src/20071029-1.c` currently reports `foo`, but its visible LLVM signature is
  scalar; it may expose an aggregate temporary or parameter-layout path after
  vector signatures are repaired.
- `src/zero-struct-2.c` is a structured aggregate return-info row and should not
  be used to justify a named-case vector fix.

## Proof

Ran:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`

Result: build had no work to do; backend subset passed `346/346`.
Fresh proof log: `test_after.log`.
