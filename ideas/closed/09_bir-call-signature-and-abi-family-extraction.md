# BIR Call Signature And ABI Family Extraction

## Intent

Reduce `src/backend/bir/lir_to_bir/calling.cpp` pressure by separating the
call signature and ABI helper family from the main call-lowering flow, without
changing call semantics or adding headers.

This is a structural cleanup after `LirCallOp` lowering moved out of the memory
coordinator. `calling.cpp` is now the right owner for call behavior, but it
should not become one undifferentiated implementation file.

## Rationale

`calling.cpp` currently owns several distinct call-related families:

```text
target ABI classification
function return ABI lowering
call argument ABI lowering
typed call parsing
function signature parameter parsing
extern/decl function lowering
direct/indirect call instruction lowering
runtime intrinsic dispatch
call failure notes
```

The first cleanup should isolate the signature/ABI pieces because they are
relatively pure, widely reused by call lowering, and easier to validate without
touching runtime call behavior.

## Design Direction

Keep ownership simple:

```text
calling.cpp
  main LirCallOp, extern, decl, and runtime intrinsic call behavior

call_abi.cpp
  target ABI and signature/return/parameter helper implementations

lowering.hpp
  remains the complete private index
```

The new file should be an implementation split only. Do not create
`call_abi.hpp`, `calling.hpp`, or other new headers.

## Scope

1. Create a new implementation file:

```text
src/backend/bir/lir_to_bir/call_abi.cpp
```

2. Move only signature/ABI helper implementations from `calling.cpp`.
   Good candidates include:
   - target float register policy helpers
   - `compute_call_arg_abi`
   - `compute_function_return_abi`
   - `lower_return_info_from_type`
   - `infer_function_return_info`
   - `lower_signature_return_info`
   - `lower_param_type`
   - `lower_function_params`
   - `parse_function_signature_params`
   - small helper parsing routines used only by these functions

3. Keep main call behavior in `calling.cpp`.
   - `lower_call_inst` stays in `calling.cpp`.
   - `parse_typed_call` and `parse_direct_global_typed_call` may stay in
     `calling.cpp` unless the active plan proves they belong with signature
     parsing.
   - `lower_runtime_intrinsic_inst` stays in `calling.cpp`.
   - failure note helpers stay where they best support call behavior.

4. Preserve declarations.
   - `lowering.hpp` remains the private index for member declarations.
   - Existing namespace-level declarations in `lir_to_bir_detail` remain
     reachable without adding a header.

5. Build and prove no behavior change.
   - Build `c4c_codegen`.
   - Run relevant BIR/LIR-to-BIR call tests covering ABI, byval/sret,
     direct/indirect calls, extern/decl functions, and runtime intrinsic calls.
   - Do not rewrite expectations.

## Acceptance Criteria

- `src/backend/bir/lir_to_bir/call_abi.cpp` exists and owns signature/ABI helper
  implementations.
- `calling.cpp` remains the owner of main call instruction behavior.
- No new `.hpp` files are added.
- `lowering.hpp` remains the complete private lowerer index.
- Call ABI, byval/sret, direct/indirect call, extern/decl, and runtime
  intrinsic behavior are unchanged.
- `c4c_codegen` builds.
- Relevant call tests pass with no expectation rewrites.

## Non-Goals

- Do not redesign call ABI classification.
- Do not split all call lowering in one pass.
- Do not move memory intrinsic implementation out of `memory/intrinsics.cpp`.
- Do not introduce a call-lowering state owner.
- Do not convert member call lowering into free functions over
  `BirFunctionLowerer& self`.

## Closure

Closed after the implementation split landed through `4e8a051f`.
`src/backend/bir/lir_to_bir/call_abi.cpp` owns the signature and ABI helper
family, `calling.cpp` still owns main call and runtime intrinsic behavior, no
new headers were added, and `lowering.hpp` remains the private declaration
index.

Close proof used the canonical broad call-family logs:
`test_before.log` and `test_after.log` both covered 71 tests with 71 passed,
and `c4c-regression-guard` reported no regressions.
