Status: Active
Source Idea Path: ideas/open/257_aarch64_returns_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Reconcile and Delete `returns.md`

# Current Packet

## Just Finished

Step 4 reconciled `src/backend/mir/aarch64/codegen/returns.md` against the
compiled returns owner and current AArch64 contracts, then deleted the markdown
shard because no durable valid content remained only in markdown.

Reconciliation summary:

- Ordinary return node construction, operand authority, missing-authority
  diagnostics, origin metadata, return uses, and return/control-flow side
  effects are covered by `returns.cpp::lower_prepared_return_terminator` and
  `returns.cpp::make_return_instruction`.
- Return-specific printer decisions are covered by the compiled
  `ReturnValuePrintForm` helpers and immediate-materialization helper functions
  declared in `returns.hpp`.
- Legacy scalar integer and `i128` "already in return registers" text is stale
  as local policy; current contracts require prepared `BeforeReturn` movement
  and ABI-binding facts instead of choosing return resources from markdown.
- Legacy F32/F64/F128 `fmov`, `q0`, `__extenddftf2`, second-component
  `s1`/`d1`/`q1`, stack-slot, and `track_f128_self` notes are out of scope for
  this returns owner unless structured carriers name those resources. The
  AAPCS64 contract records F128, i128, aggregate splitting, multi-register
  return resources, runtime-helper resources, and return-boundary metadata as
  carrier candidates or shared prepared-carrier gaps, not local register
  shortcuts.
- Historical dependencies on Rust `ArmCodegen`, `emit_return_default`,
  epilogue emission, raw stack-slot helpers, and rendered ABI register spelling
  are stale under the current target-MIR contract.
- No code change was needed in `returns.cpp` or `returns.hpp`; no test
  expectations, broad owners, `plan.md`, source idea, or tests were changed.

## Suggested Next

Next packet: supervisor-selected Step 5 acceptance validation for the completed
returns shard redistribution, including any broader backend or regression guard
check needed before lifecycle close review.

## Watchouts

- Do not reintroduce `returns.md` as a source of register policy. Return payload
  placement must come from prepared `BeforeReturn` moves and ABI bindings.
- F128 soft-float helper resources, `i128`/aggregate splitting, second
  floating-point return components, and explicit return-boundary/x30 metadata
  remain carrier-surface work if later implementation needs them.
- `machine_printer.cpp` still owns generic printer validation, register-name
  formatting, and line emission for returns.

## Proof

Supervisor-selected proof for Step 4:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_aarch64_(return_lowering|signature_metadata|machine_printer|instruction_dispatch|target_record_core_contract|target_instruction_records|prepared_scalar_alu_records)$' > test_after.log 2>&1
```

`test_after.log` reports 7/7 tests passed:
`backend_aarch64_return_lowering`, `backend_aarch64_signature_metadata`,
`backend_aarch64_machine_printer`, `backend_aarch64_instruction_dispatch`,
`backend_aarch64_target_record_core_contract`,
`backend_aarch64_target_instruction_records`, and
`backend_aarch64_prepared_scalar_alu_records`.
