# Current Packet

Status: Active
Source Idea Path: ideas/open/129_aarch64_i128_shift_support_completeness.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Current Shift Contract And Coverage

## Just Finished

Step 1: Map Current Shift Contract And Coverage completed for AArch64 i128
shifts.

Current lowering/contract:

- `src/backend/mir/aarch64/codegen/i128_ops.cpp` routes i128 `Shl`, `LShr`,
  and `AShr` through `make_prepared_i128_shift_record()` and
  `make_i128_shift_instruction()` when prepared i128 register-pair carriers,
  value locations, and storage-plan facts are present.
- `is_supported_i128_shift_count()` accepts immediate counts `0..127` and
  non-immediate counts when the count type has a scalar register view; other
  counts fail closed as `unsupported_shift_count`.
- Shift records preserve kind/lane semantics for left, logical right, and
  arithmetic right shifts, set `count_kind` to `Immediate` or `Register`, and
  add two result defs plus source-pair uses and the count use.
- `print_i128_shift()` currently prints only immediate counts `0..63`.
  Immediate `0` emits a pair move. Immediate `1..63` emits cross-lane AArch64
  spelling using `lsl`/`extr` for left, `extr`/`lsr` for logical right, and
  `extr`/`asr` for arithmetic right.
- Immediate counts `64..127` are accepted by record construction but rejected
  by the printer with `i128 shift immediate is outside the printable 0..63
  subset`.
- Variable-count shifts are accepted into records when scalar count storage is
  available but rejected by the printer with `i128 shift printer currently
  requires an immediate shift count`.

Covered categories:

- Constant count below 64 is covered at the record/effects layer by
  `tests/backend/mir/backend_aarch64_target_instruction_records_test.cpp`
  with immediate `AShr` count `17`.
- Constant count below 64 is covered at the printer layer by
  `tests/backend/mir/backend_aarch64_machine_printer_test.cpp` with immediate
  `LShr` count `12`, proving `extr` plus `lsr` spelling.
- Constant count below 64 is covered at dispatch/lowering by
  `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp` with
  immediate `LShr` count `12` from prepared carriers.

Uncovered or not yet supported categories:

- No focused coverage for immediate `Shl` printing.
- No focused coverage for immediate `AShr` printing.
- No focused coverage for immediate count `0` pair-copy printing.
- No focused coverage for immediate count `63` boundary behavior.
- Immediate counts `64..127` have a mixed contract: record construction
  accepts them but printer rejects them. There is no focused diagnostic or
  success coverage for `== 64` or `> 64`.
- Variable-count i128 shifts have a mixed contract: record construction accepts
  register-backed counts but printer rejects all non-immediate counts. There is
  no focused diagnostic or success coverage for variable `Shl`, `LShr`, or
  `AShr`.

Relevant files/tests:

- Implementation and contract: `src/backend/mir/aarch64/codegen/i128_ops.cpp`
  and `src/backend/mir/aarch64/codegen/i128_ops.hpp`.
- Record shapes and opcode/family spelling:
  `src/backend/mir/aarch64/codegen/instruction.hpp` and
  `src/backend/mir/aarch64/codegen/instruction.cpp`.
- Printer dispatch:
  `src/backend/mir/aarch64/codegen/machine_printer.cpp`.
- Focused tests:
  `tests/backend/mir/backend_aarch64_target_instruction_records_test.cpp`,
  `tests/backend/mir/backend_aarch64_machine_printer_test.cpp`, and
  `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`.
- Proposed narrow proof subset after implementation:
  `ctest --test-dir build -R 'backend_aarch64_(target_instruction_records|machine_printer|instruction_dispatch)' --output-on-failure`.

## Suggested Next

Execute Step 2 by completing or explicitly contracting immediate i128 shifts
with counts greater than or equal to 64. First implementation category to fix
or contract: immediate `Shl`, `LShr`, and `AShr` count `== 64`, then the
representative `> 64` range, because record construction already accepts
`0..127` and the current inconsistency is at printable lowering.

## Watchouts

- Keep the route target-local to AArch64 i128 shifts.
- Do not weaken supported-path expectations or add named-testcase shortcuts.
- Do not expand into f128, helper ABI/resource policy, carrier ownership, or
  shared BIR/prealloc policy.
- Avoid only changing diagnostics for `64..127` unless that is an explicit
  support-contract decision from the supervisor; the current record builder
  already treats those counts as supported enough to select an `I128Shift`.
- Variable-count support is a separate Step 3 category. Do not mix it into the
  first large-immediate implementation packet unless the supervisor explicitly
  broadens the packet.

## Proof

Proof: analysis-only, no build/test run.
