# BIR Call Signature And ABI Family Extraction Runbook

Status: Active
Source Idea: ideas/open/09_bir-call-signature-and-abi-family-extraction.md

## Purpose

Reduce pressure in `src/backend/bir/lir_to_bir/calling.cpp` by moving the
signature and ABI helper family into a dedicated implementation file, without
changing call semantics or adding headers.

## Goal

Create `src/backend/bir/lir_to_bir/call_abi.cpp` as an implementation split
for call signature and ABI helpers while keeping main call lowering behavior in
`calling.cpp`.

## Core Rule

This is a behavior-preserving extraction. Do not redesign ABI classification,
do not rewrite expectations, and do not add `call_abi.hpp`, `calling.hpp`, or
any other new header.

## Read First

- `ideas/open/09_bir-call-signature-and-abi-family-extraction.md`
- `src/backend/bir/lir_to_bir/calling.cpp`
- `src/backend/bir/lir_to_bir/lowering.hpp`
- `tests/backend/CMakeLists.txt`
- `src/codegen/CMakeLists.txt`

## Current Targets

- New implementation file:
  `src/backend/bir/lir_to_bir/call_abi.cpp`
- Existing owner to slim:
  `src/backend/bir/lir_to_bir/calling.cpp`
- Private declaration index:
  `src/backend/bir/lir_to_bir/lowering.hpp`
- Hard-coded backend test source list:
  `tests/backend/CMakeLists.txt`

## Non-Goals

- Do not redesign target ABI classification.
- Do not split all call lowering in this runbook.
- Do not move memory intrinsic implementation out of
  `src/backend/bir/lir_to_bir/memory/intrinsics.cpp`.
- Do not introduce a call-lowering state owner.
- Do not convert member call lowering into free functions over
  `BirFunctionLowerer& self`.
- Do not move `lower_call_inst` or `lower_runtime_intrinsic_inst` out of
  `calling.cpp`.

## Working Model

- `calling.cpp` remains the owner of main `LirCallOp`, extern, decl, direct and
  indirect call instruction, runtime intrinsic, and call failure-note behavior.
- `call_abi.cpp` owns target ABI and signature/return/parameter helper
  implementations.
- `lowering.hpp` remains the complete private index for declarations; the split
  is implementation-only.
- `parse_typed_call` and direct-call typed parsing stay in `calling.cpp` unless
  extraction proves a specific helper belongs with signature parsing.

## Execution Rules

- Keep each move mechanical and symbol-preserving.
- Preserve namespaces, member signatures, and existing declaration reachability.
- Prefer moving whole helper definitions over rewriting logic.
- If a helper is used by both `calling.cpp` and `call_abi.cpp`, keep its
  declaration in `lowering.hpp` or leave it in the file that owns the behavior.
- Update build/test source lists only where the new `.cpp` file is not already
  discovered automatically.
- Run fresh compile proof after any code-moving packet.
- Run relevant call-family tests before treating the extraction as complete.

## Steps

### Step 1: Audit helper boundaries and wire the new file

Goal: establish the exact extraction boundary and make the project aware of the
new implementation unit.

Primary target:
`src/backend/bir/lir_to_bir/calling.cpp`

Actions:

- Inspect current definitions for the candidate helpers:
  `compute_call_arg_abi`, `compute_function_return_abi`,
  `lower_return_info_from_type`, `infer_function_return_info`,
  `lower_signature_return_info`, `lower_param_type`,
  `lower_function_params`, `parse_function_signature_params`, and any small
  parsing helpers used only by those functions.
- Identify dependencies each candidate needs from `lir_to_bir_detail`,
  `BirFunctionLowerer`, and `TypeDeclMap`.
- Create `src/backend/bir/lir_to_bir/call_abi.cpp` with the existing include
  pattern for this directory.
- Update `tests/backend/CMakeLists.txt` so `backend_lir_to_bir_notes_test`
  links the new implementation file.
- Do not add headers or change declarations in `lowering.hpp` unless the move
  requires declaration exposure that already belongs in the private index.

Completion check:

- `call_abi.cpp` exists.
- No new `.hpp` files exist for this split.
- The code still compiles far enough to expose only real extraction issues,
  using `cmake --build build --target c4c_codegen`.

### Step 2: Move pure target ABI and return classification helpers

Goal: extract the relatively pure ABI helpers before moving member-signature
logic.

Primary target:
`src/backend/bir/lir_to_bir/call_abi.cpp`

Actions:

- Move target float-register policy helpers used only by ABI classification.
- Move namespace-level ABI helpers such as `compute_call_arg_abi` and
  `compute_function_return_abi`.
- Move return-info helpers that do not depend on active call-lowering state,
  including `lower_return_info_from_type` and `lower_signature_return_info`,
  if their dependencies remain clean.
- Keep all behavior and diagnostic text unchanged.

Completion check:

- `calling.cpp` no longer owns the extracted ABI classification bodies.
- External callers that include `lowering.hpp` still resolve the same helper
  declarations.
- `cmake --build build --target c4c_codegen` succeeds.
- `ctest --test-dir build -R "backend_lir_to_bir_notes" --output-on-failure`
  succeeds.

### Step 3: Move function signature and parameter helpers

Goal: complete the signature/parameter extraction without moving main call
behavior.

Primary target:
`src/backend/bir/lir_to_bir/call_abi.cpp`

Actions:

- Move `lower_param_type`, `lower_function_params`,
  `parse_function_signature_params`, and parsing helpers used only by function
  signature parsing.
- Move `infer_function_return_info` only if the dependency on lowerer state
  remains local and does not pull call instruction behavior into `call_abi.cpp`.
- Leave `parse_typed_call`, `parse_direct_global_typed_call`, `lower_call_inst`,
  and runtime intrinsic lowering in `calling.cpp` unless a dependency audit
  proves a small shared parser should move with signature parsing.
- Remove now-unused includes or using declarations from `calling.cpp` only when
  the compiler proves they are unused or invalid.

Completion check:

- `call_abi.cpp` owns the accepted signature/ABI helper implementations.
- `calling.cpp` still owns main call instruction and runtime intrinsic
  behavior.
- `lowering.hpp` remains the only private declaration index.
- `cmake --build build --target c4c_codegen` succeeds.
- `ctest --test-dir build -R "backend_lir_to_bir_notes" --output-on-failure`
  succeeds.

### Step 4: Prove call-family behavior did not change

Goal: validate the structural split across ABI, byval/sret, direct/indirect
calls, extern/decl functions, and runtime intrinsic calls.

Primary target:
call-family tests selected by the supervisor.

Actions:

- Run compile proof:
  `cmake --build build --target c4c_codegen`
- Run the focused semantic BIR notes test:
  `ctest --test-dir build -R "backend_lir_to_bir_notes" --output-on-failure`
- Run call/ABI route coverage, at minimum a supervisor-approved subset matching
  these families:
  `backend_codegen_route_x86_64_.*call.*observe_semantic_bir`,
  `backend_runtime_byval_helper_.*`,
  `backend_prepare_frame_stack_call_contract`, and relevant direct/indirect
  handoff tests.
- Do not rewrite test expectations to make the extraction pass.

Completion check:

- Build proof is fresh and green.
- Relevant call-family tests are green with no expectation rewrites.
- The final diff is an implementation split only: one new `.cpp`, reduced
  `calling.cpp`, necessary build-list updates, and lifecycle progress in
  `todo.md`.
