Status: Active
Source Idea Path: ideas/open/248_prepared_i128_runtime_helper_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect I128 Helper Authority Gap

# Current Packet

## Just Finished

Step 1 inspection completed for the prepared/shared i128 runtime-helper
authority gap.

Current helper-required i128 operation shapes:

- BIR carries i128 div/rem as ordinary `bir::BinaryInst` values with
  `operand_type == TypeKind::I128`, named `result`, named `lhs`/`rhs`, and
  opcodes `SDiv`, `UDiv`, `SRem`, or `URem`.
- BIR carries float/i128 conversions as ordinary `bir::CastInst` values with
  named `result`/`operand` and opcodes `FPToSI`, `FPToUI`, `SIToFP`, or
  `UIToFP`; the source/result type relationship must be read from the values,
  not from a call boundary.
- Existing selected i128 pair/shift/compare paths consume
  `PreparedI128CarrierFunction` records for source/result value lanes, but
  `SDiv`/`UDiv`/`SRem`/`URem` intentionally remain unsupported by pair
  selection.

Existing generic call/prepared facts:

- Retained `bir::CallInst` sites publish `PreparedCallPlan` with block/index
  identity, wrapper/callee shape, `PreparedCallArgumentPlan`,
  `PreparedCallResultPlan`, optional `PreparedMemoryReturnPlan`,
  preserved-value records, and `PreparedClobberedRegister` entries.
- Retained-call ABI movement and value-location facts are produced only while
  scanning actual `bir::CallInst` instructions; `append_prepared_call_abi_bindings`,
  `append_call_arg_move_resolution`, and `append_call_result_move_resolution`
  skip `BinaryInst` and `CastInst`.
- `PreparedI128Carrier` already exposes producer-owned register-pair or
  memory-backed lane authority for i128 values, including low/high lane order,
  storage/register facts, size/alignment, and missing-fact diagnostics.
- `PreparedBirModule` has `call_plans` and `i128_carriers`, but no
  source-operation-indexed runtime-helper carrier that ties a non-call i128
  source operation to helper kind, callee, argument lanes, result ownership,
  memory-return behavior, clobbers, resources, or ABI policy.

Missing producer-owned helper facts:

- Helper source identity: function, block index, instruction index, result
  value id/name, source opcode, source/result types, and source operands.
- Helper selection authority: explicit helper family/kind and callee identity
  for i128 signed/unsigned div/rem and float/i128 conversions where supported.
- Lane authority: low/high argument bindings and direct low/high result
  bindings as references to prepared i128 carriers, not fixed `x0`/`x1` or
  rendered-register assumptions.
- Memory-return authority: explicit destination ownership, size, alignment,
  and storage facts where a helper ABI returns through memory.
- Boundary policy: clobbered registers/resources and ABI/register-bank
  transition facts equivalent to retained-call facts but owned by the helper
  producer instead of synthesized in AArch64 dispatch.
- Fail-closed diagnostics for missing helper mapping, unsupported conversion
  shape, incomplete lane carrier, missing memory-return ownership, or missing
  clobber/resource policy.

## Suggested Next

Execute Step 2 as a prepared/shared authority packet: add a
`PreparedI128RuntimeHelper` carrier family beside `PreparedCallPlans` and
`PreparedI128Carriers`, plus lookup/printing support, then populate structural
source-operation-to-helper mapping records for i128 `SDiv`, `UDiv`, `SRem`,
and `URem`.

The first code packet should stay narrow:

- Define helper kind/family and callee identity fields in the prepared layer.
- Record function/block/instruction source identity, opcode, result value
  identity, operand value identities, and source/result type facts.
- Populate only complete binary div/rem mapping records first; leave lane
  binding, memory-return ownership, and clobber/resource policy for later
  runbook steps.
- Treat float/i128 conversion mappings as explicit unsupported/deferred cases
  until the Step 2 packet can name complete source/result type and helper
  callee policy.
- Do not add AArch64 selected helper nodes, printer output, target-local helper
  synthesis, fixed-register marshaling, or scalar-i64 substitutes.

Suggested focused proof for that Step 2 code packet:

```sh
(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_liveness|backend_prepare_frame_stack_call_contract|backend_prepared_printer)$') > test_after.log 2>&1
```

## Watchouts

- Do not reuse `PreparedCallPlan` directly for non-call helper operations
  unless a producer-owned wrapper ties it to the original `BinaryInst` or
  `CastInst`; retained-call indexing alone is not enough.
- Do not infer helper callee names from opcode spelling in AArch64 dispatch.
  The mapping must be published before target selection consumes it.
- Do not infer i128 low/high lanes from contiguous register width, register
  adjacency, rendered register names, or fixed `x0`/`x1` conventions.
- `infer_scalar_function_return_abi` currently treats function-level i128
  returns as memory-backed; helper direct-result versus memory-return policy
  still needs explicit carrier facts before idea 236 can consume helper
  boundaries safely.

## Proof

Inspection-only packet. No build, tests, or proof logs were created or
modified.

Commands run:

- `git status --short`
- `sed -n '1,220p' /workspaces/c4c/.codex/skills/c4c-executor/SKILL.md`
- `sed -n '1,220p' /workspaces/c4c/.codex/skills/c4c-clang-tools/SKILL.md`
- `sed -n '1,220p' todo.md`
- `sed -n '1,260p' plan.md`
- `rg -n "PreparedCallPlan|PreparedCallArgumentPlan|PreparedCallResultPlan|PreparedClobberedRegister|PreparedI128Carrier|BinaryOpcode::(SDiv|UDiv|SRem|URem)|CastOpcode::(FPToSI|FPToUI|SIToFP|UIToFP)|TypeKind::I128|i128" src/backend/bir src/backend/prealloc src/backend/mir/aarch64/codegen tests/backend/bir tests/backend/mir`
- `rg -n "enum class BinaryOpcode|enum class CastOpcode|struct BinaryInst|struct CastInst|struct PreparedCallPlan|struct PreparedI128Carrier|struct PreparedCallArgumentPlan|struct PreparedCallResultPlan|struct PreparedClobberedRegister" src/backend/bir src/backend/prealloc src/backend/mir/aarch64/codegen`
- `command -v c4c-clang-tool-ccdb && command -v c4c-clang-tool`
- `c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/prealloc/regalloc.cpp build/compile_commands.json | rg "call|i128|I128|append|carrier|clobber|abi|return"`
- focused `sed`/`rg` reads of BIR instruction structs, prepared call/i128
  carrier structs, `PreparedBirModule`, call ABI preparation, AArch64 i128
  dispatch, and current i128 pair opcode support.
- `git diff -- todo.md`
- `git diff --check`
- `git status --short`
