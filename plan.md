# AArch64 Binary128 Softfloat Lowering Runbook

Status: Active
Source Idea: ideas/open/237_aarch64_binary128_softfloat_lowering.md

## Purpose

Transcribe the binary128 AArch64 route into small executable steps that add
structured prepared facts, selected machine nodes, and printer support without
collapsing `F128` values into scalar `F64` shortcuts. Full-width F128 constant
carrier design is tracked separately in
`ideas/open/241_f128_full_width_constant_carriers.md` and must not be absorbed
into this runbook as backend Step 3 expansion.

## Goal

Lower AArch64 binary128 arithmetic, comparisons, casts, constants, and
full-width memory transport through prepared/shared carrier facts and
structured machine nodes that preserve the full 16-byte value.

## Core Rule

Every `F128` value remains a 16-byte value across prepared facts, call helper
boundaries, storage, reload, and selected machine records. Do not represent
binary128 progress by truncating to `F64`, by fixed scratch-register snippets,
or by named-testcase helper shortcuts.

## Read First

- `ideas/open/237_aarch64_binary128_softfloat_lowering.md`
- `docs/backend/x86_codegen_legacy/f128.cpp.md`
- `src/backend/bir/bir.hpp`
- `src/backend/prealloc/regalloc.cpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`
- `tests/backend/mir/backend_aarch64_prepared_memory_operand_records_test.cpp`

## Current Targets

- Prepared carrier facts for `bir::TypeKind::F128` source tracking,
  16-byte temporary storage, and helper-call resource/clobber boundaries.
- AArch64 instruction records for binary128 load, store, copy, sign-bit
  negation, comparison, cast, and soft-float helper-call cases.
- F128 constant transport remains fail-closed here until
  `ideas/open/241_f128_full_width_constant_carriers.md` provides prepared
  full-width constant payload authority.
- Machine-printer support for only the structured records that have complete
  prepared authority.
- Focused backend tests proving full-width transport and helper-boundary facts.

## Non-Goals

- Do not assume hardware quad-FP arithmetic on AArch64.
- Do not route binary128 arithmetic through scalar `F32` or `F64` ALU records.
- Do not extend the i128 route unless a shared carrier helper is genuinely
  reused without changing i128 behavior.
- Do not reserve local scratch registers or spill slots outside prepared
  authority.
- Do not change unsupported expectations or external testcase contracts to
  claim capability progress.
- Do not merge atomic, intrinsic, or inline-assembly machine-node work into
  this plan.
- Do not design BIR or prepared full-width F128 constant carriers inside this
  AArch64 backend runbook; use
  `ideas/open/241_f128_full_width_constant_carriers.md` for that dependency.

## Working Model

- Treat existing i128 carrier and runtime-helper code as a nearby pattern, not
  as a binary128 semantic replacement.
- Start from fail-closed prepared records so incomplete binary128 facts produce
  explicit diagnostics.
- Add selected machine nodes only after the corresponding prepared carrier can
  prove operand homes, storage width, helper callee identity, and live-value
  preservation.
- Keep memory transport separate from arithmetic helper calls so load/store and
  copy proof can land before arithmetic lowering.

## Execution Rules

- Each code-changing step must run a build or targeted compile proof chosen by
  the supervisor, then the delegated narrow backend test subset.
- Add tests beside the backend surface being changed; prefer record-level tests
  before final assembly text tests.
- Preserve existing scalar FP and i128 test behavior after each step touching
  shared dispatch, record, or printer code.
- If a step discovers missing shared BIR or ABI intent that is broader than
  binary128 lowering, stop and ask the supervisor to create or activate a
  separate idea instead of expanding this plan.

## Ordered Steps

### Step 1: Inventory Current F128 Prepared Coverage

Goal: establish the exact missing carrier facts before adding binary128 machine
records.

Primary targets:
- `src/backend/prealloc/regalloc.cpp`
- `src/backend/bir/bir.hpp`
- existing AArch64 memory, scalar-cast, and dispatch tests

Actions:
- Inspect all current `F128` branches in BIR, prealloc, AArch64 dispatch, and
  machine printing.
- Identify which value-home, memory-access, local-slot, call-preservation, and
  helper-call facts already carry 16-byte size and alignment.
- Add focused fail-closed tests for the first missing binary128 prepared fact
  family without changing selected machine-node behavior.

Completion check:
- Tests prove the current route either preserves existing `F128` prepared facts
  or reports explicit missing-fact diagnostics for the first unsupported family.
- `todo.md` records the first implementation packet and proof command.

### Step 2: Add Binary128 Full-Width Carrier Facts

Goal: make prepared output authoritative for binary128 value homes, temporary
16-byte slots, memory accesses, and live-value preservation across helper
calls.

Primary targets:
- `src/backend/prealloc/regalloc.cpp`
- prepared data structures referenced by AArch64 dispatch
- backend prepared-record tests

Actions:
- Add or reuse structured carrier records for binary128 low/high storage
  identity, 16-byte size/alignment, and source value tracking.
- Ensure helper-call resource facts describe clobbers, preserved values, and
  result ownership without fixed scratch-register assumptions.
- Keep i128 carrier behavior unchanged unless a shared helper is only renamed or
  generalized with identical i128 semantics.

Completion check:
- Record tests cover binary128 prepared carriers for memory, temporary storage,
  and helper-boundary preservation.
- Existing i128 helper and scalar FP tests still pass under the delegated
  subset.

### Step 3: Select Binary128 Memory And Copy Nodes

Goal: lower binary128 load, store, and copy transport into structured AArch64
machine records while leaving constant transport fail-closed until the separate
full-width constant-carrier dependency exists.

Primary targets:
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`

Actions:
- Add binary128 instruction-family or opcode ownership for full-width memory
  transport.
- Lower only when prepared carrier facts prove the source, destination, and
  16-byte storage contract.
- Preserve failure diagnostics for missing carrier facts instead of falling
  through to scalar memory or unsupported text output.
- Keep F128 constant transport unsupported here unless a structured full-width
  constant carrier from
  `ideas/open/241_f128_full_width_constant_carriers.md` has first landed.

Completion check:
- Dispatch tests prove selected binary128 load/store/copy nodes preserve
  low/high halves or equivalent full-width storage facts.
- Missing-fact and F128 constant-transport tests remain fail-closed when no
  full-width constant carrier exists.

### Step 4: Select Binary128 Soft-Float Helper Nodes

Goal: lower binary128 arithmetic, comparisons, casts, and sign-bit negation
through structured records backed by the correct prepared helper or
full-width-operation authority.

Primary targets:
- `src/backend/prealloc/regalloc.cpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- helper-boundary tests

Actions:
- Keep the completed add/sub/mul/div route as the binary arithmetic helper
  boundary only; do not reuse that record shape for comparison, cast, or
  sign-bit work by analogy.
- For each remaining helper family, define the prepared helper identity,
  operand/result ownership, ABI/marshaling, clobber policy, live-preservation,
  and later-user reload contract before admitting selection.
- Keep unsupported operations diagnosed until their helper contract is
  structured.

#### Step 4.1: Binary Arithmetic Helper Boundary

Goal: select F128 add, sub, mul, and signed division through structured
soft-float helper-call records.

Actions:
- Map only the binary arithmetic opcodes with complete prepared authority to
  explicit helper identities such as `__addtf3`, `__subtf3`, `__multf3`, and
  `__divtf3`.
- Require full F128 operand/result types, q-register or equivalent full-width
  carrier facts, ABI/marshaling facts, clobbers, live-preservation policy, and
  selected-call ownership.
- Keep unsigned division, remainders, comparisons, casts, bitwise/logical
  families, and sign-bit negation outside this record until their contracts
  are deliberately modeled.

Completion check:
- Record-level tests prove add/sub/mul/div helper calls preserve full 16-byte
  operands/results and remain fail-closed for adjacent unsupported helper
  families.

#### Step 4.2: Comparison Helper Boundary

Goal: lower F128 comparisons only after predicate-specific helper semantics and
non-F128 result ownership are prepared.

Actions:
- Define predicate-to-helper identity and result shape for ordered/unordered
  comparison cases selected by the supervisor.
- Model how later users consume the comparison result while reloading or
  preserving the full tracked F128 sources.
- Prove missing predicate/helper authority reports diagnostics instead of
  falling through to scalar `F64` comparison or arithmetic helper records.

Completion check:
- Comparison tests prove helper identity, result ownership, full-source
  preservation, and fail-closed behavior for unmodeled predicates.

#### Step 4.3: Cast Helper Boundary

Goal: lower F128 casts only after unary source/result type-pair contracts and
ABI ownership are prepared.

Actions:
- Define supported F128 cast source/result pairs and their helper identities.
- Model unary argument marshaling, result ownership, source preservation, and
  clobbers from prepared facts.
- Keep unsupported casts diagnosed until their type-pair helper contracts are
  explicit.

Completion check:
- Cast tests prove full-width F128 source preservation, correct result
  ownership for the destination type, and fail-closed behavior for unsupported
  cast pairs.

#### Step 4.4: Sign-Bit Negation Boundary

Goal: decide and implement the sign-bit negation route as a distinct semantic
boundary, not as binary arithmetic by analogy.

Actions:
- Determine whether F128 sign-bit negation belongs to a helper call,
  full-width bit operation, or separate prepared transport/mask authority.
- Add selection only after the chosen route has complete full-width operand,
  result, clobber, and preservation facts.
- Reject scalar `F64` sign manipulation and fixed scratch-register snippets.

Completion check:
- Sign-bit negation tests prove the chosen full-width route and keep incomplete
  or unmodeled authority diagnosed.

#### Step 4.5: Unsupported Helper-Family Diagnostics

Goal: preserve explicit diagnostics for helper families whose prepared
contracts are still absent.

Actions:
- Keep unsigned division, remainders, bitwise/logical families, and other
  unmodeled F128 helper cases unsupported unless a focused packet adds complete
  prepared authority.
- Test that unsupported cases do not silently select binary arithmetic helper
  records, scalar `F64` operations, or testcase-shaped callee guesses.

Completion check:
- Unsupported-family tests prove fail-closed diagnostics remain clear after
  comparison, cast, and sign-bit packets land.

Completion check:
- Step 4.1 through Step 4.5 completion checks are satisfied.
- Any helper family that lacks complete prepared authority remains diagnosed
  instead of selected through scalar approximations or binary-arithmetic record
  reuse.

### Step 5: Print Supported Binary128 Machine Nodes

Goal: add AArch64 assembly printing for selected binary128 records that have
complete structured facts.

Primary targets:
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- AArch64 machine-printer tests

Actions:
- Print full-width load/store/copy sequences and helper-call boundaries from
  machine records.
- Emit diagnostics for binary128 records whose prepared facts are incomplete.
- Avoid reconstructing operand semantics from already-rendered assembly text.

Completion check:
- Printer tests cover successful full-width binary128 transport and helper
  calls.
- Unsupported or incomplete records produce explicit target diagnostics.

### Step 6: End-To-End Binary128 Route Proof

Goal: prove the route works through the selected AArch64 backend without
weakening unsupported contracts.

Primary targets:
- backend route tests under `tests/backend`
- any narrow external AArch64 smoke case selected by the supervisor

Actions:
- Add or enable representative binary128 route cases for memory transport,
  arithmetic helper calls, comparison or cast reuse, and full-width store-back.
- Run the supervisor-selected narrow proof, then a broader backend validation
  if multiple shared surfaces changed.
- Verify no expectation downgrade, named-case matching, or scalar `F64`
  approximation appears in the diff.

Completion check:
- Narrow binary128 proof is green.
- Broader backend proof is green when required by the supervisor.
- `todo.md` records the final proof and any remaining unsupported binary128
  operations as follow-up notes, not as hidden failures.
