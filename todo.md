Status: Active
Source Idea Path: ideas/open/563_bir_wide_vector_abi_signature_representation_owner_decision.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Wide-Vector ABI Signature Evidence

# Current Packet

## Just Finished

Step 1 of `plan.md` refreshed the wide-vector ABI signature boundary without
choosing or implementing the representation decision.

Representative set inspected:
- `src/backend/bir/lir_to_bir/call_abi.cpp`: `lower_fixed_vector_signature_carrier_type`
  currently maps fixed-vector signature carriers only up to 8 bytes to `I32`
  or `I64`; 16-byte and 32-byte vectors return `nullopt`.
- `tests/backend/bir/backend_lir_to_bir_notes_test.cpp`: existing focused
  coverage includes AArch64 16-byte vector intrinsics and local vector
  alloca/store/load lane-source facts, but no focused 16-byte or 32-byte
  vector function return-info or parameter-layout publication contract.
- `tests/c/external/gcc_torture/src/ieee/pr72824-2.c` / `foo`: generated LLVM
  signature is `define internal void @foo(ptr %p.x, <4 x float> %p.value)`;
  `--dump-bir --target aarch64-unknown-linux-gnu --mir-focus-function foo`
  fails in `function-signature semantic family`. Because return is `void` and
  `BirFunctionLowerer::lower()` reaches params after successful return-info,
  this is a parameter-layout publication boundary for the 16-byte vector param,
  not vector-binop, scalar-cast, local-memory, alloca, RV64, or object emission.
- `tests/c/external/gcc_torture/src/pr70903.c` / `foo`: generated LLVM
  signature is `define internal <8 x i32> @foo(<4 x i64> %p.x) noinline`;
  `--dump-bir --target riscv64-linux-gnu --mir-focus-function foo` fails in
  `function-signature semantic family`. Because return-info is inferred before
  params, this is currently a return-info publication boundary for the 32-byte
  vector return; the 32-byte vector parameter remains the next same-family
  boundary once return-info publication is decided.

## Suggested Next

Step 2 should make the BIR ABI carrier contract explicit at the signature
publication boundary before admission changes: either define and test a real
16-byte/32-byte vector carrier representation for return-info and params, or
codify a deliberate fail-closed diagnostic for those signatures. Start with
focused BIR tests for one 16-byte parameter-only function and one 32-byte
return-plus-parameter function, then change only `call_abi.cpp` publication
logic to match the selected contract.

## Watchouts

Keep `ideas/open/585_bir_vector_binop_semantic_producer_admission.md` separate;
it owns vector arithmetic after signature facts are available, not wide-vector
ABI signature carrier representation.

Do not satisfy Step 2 by mapping 16-byte or 32-byte vectors to `I128`, VRM
types, split scalar pairs, or memory-like metadata unless that is the explicit
documented BIR ABI representation. The current named representatives do not yet
reach downstream vector-binop/scalar-cast/local-memory/alloca/RV64/object
emission; they stop at signature publication.

## Proof

Ran exact delegated proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed; `test_after.log` records `100% tests passed, 0 tests failed out
of 346`.
