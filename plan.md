# AArch64 Loaded Byte Value Reused As Address Runbook

Status: Active
Source Idea: ideas/open/366_aarch64_loaded_byte_value_reused_as_address.md

## Purpose

Repair the AArch64 prepared scalar-memory path that lets a loaded byte/scalar
value remain usable as an address operand after string literal pointer-value
publication has already been fixed.

## Goal

Keep loaded byte/scalar values distinct from pointer/address carriers in
prepared BIR, register allocation, and generated AArch64 emission, then prove
that `c_testsuite_aarch64_backend_src_00173_c` advances beyond the recorded
byte-value-as-address failure or passes.

## Core Rule

Fix the general address-carrier rule. Do not special-case `00173`, a literal,
a stack slot, a register, an instruction sequence, or a loop body.

## Read First

- `ideas/open/366_aarch64_loaded_byte_value_reused_as_address.md`
- Prior evidence recorded there for the bad shape:
  - `ldr x13, [sp, #16]`
  - `ldrb w9, [x13]`
  - `mov x9, x13; ldrb w13, [x9]`
  - later `mov x9, x13; ldrb w13, [x9]` after `sxtb w13, w13`
- Relevant backend surfaces discovered during Step 1, especially prepared BIR,
  scalar memory lowering, address operand materialization, and AArch64 emission
  code that tracks whether a value can be used as an address.

## Current Scope

- Localize the first boundary where a loaded byte/scalar value becomes or
  remains address-capable.
- Repair scalar-memory or address-carrier handling so byte/scalar loads cannot
  feed later memory-address operands unless they are true pointer values.
- Add focused backend coverage for the repaired distinction.
- Preserve string/global pointer publication from idea 365.
- Preserve dynamic pointer-derived byte-load behavior from idea 356.

## Non-Goals

- Do not reopen string/global pointer-value publication unless fresh evidence
  shows the exact old `sp+offset` publication failure returned.
- Do not reopen semantic dynamic pointer-derived byte-load preservation except
  to keep that behavior stable.
- Do not change external-call string/global argument lowering, recursive
  formal-home publication, frontend admission, ABI composite/byval/HFA/f128,
  variadic floating, dynamic stack, runner behavior, timeout policy, CTest
  registration, proof-log handling, expectations, or unsupported
  classifications.

## Working Model

The initial literal pointer is now published correctly into the local pointer
slot. The remaining failure is later: after loading through that pointer, the
byte/scalar result is treated as though it were another address. The fix should
make address eligibility explicit enough that data loaded by `ldrb` does not
become the base register for a later `ldrb` unless the IR semantics actually
describe a pointer load.

## Execution Rules

- Start each code-changing step with the narrowest compile/build proof the
  supervisor delegates.
- Use prepared BIR, register-allocation traces, canonical dumps, or generated
  AArch64 evidence to justify the boundary before changing lowering behavior.
- Keep progress in `todo.md`; do not edit the source idea unless durable source
  intent changes.
- Do not claim progress through expectation rewrites, unsupported
  classification changes, runner changes, timeout changes, proof-log changes,
  filename-specific routing, or emitted-text-only cleanup.
- Escalate to the supervisor for lifecycle split if evidence points to an
  unrelated call-lowering, ABI, frontend, runner, timeout, dynamic stack, or
  test-registration initiative.

## Ordered Steps

### Step 1: Localize The Address-Carrying Boundary

Goal: identify where the loaded byte/scalar value first becomes an address
operand or retains address eligibility.

Concrete actions:

- Reproduce the `00173` AArch64 backend failure with the supervisor-delegated
  proof command.
- Capture the relevant prepared BIR, register-allocation, and generated
  AArch64 evidence around the first bad fact.
- Trace the value loaded by the first `ldrb` from producer to any later memory
  address use.
- Identify the responsible lowering or emission rule without changing
  implementation behavior.

Completion check:

- `todo.md` records the exact boundary, the implementation surface that owns
  it, and the proof command/log used to observe it.

### Step 2: Repair Scalar Value Versus Address Carrier Handling

Goal: make the responsible backend rule preserve the distinction between
loaded scalar data and true pointer/address carriers.

Concrete actions:

- Patch the localized implementation surface from Step 1.
- Ensure byte/scalar load results are not selected as memory base addresses
  unless their source semantics are pointer-valued.
- Preserve correct handling for real pointer loads, local pointer variables,
  and dynamic pointer-derived byte loads.
- Avoid instruction-text pattern matching and testcase-shaped branches.

Completion check:

- Focused backend proof shows the bad loaded-byte-as-address shape is gone for
  the target case without regressing nearby pointer-derived byte-load behavior.

### Step 3: Add Or Tighten Focused Coverage

Goal: make the scalar-value/address-carrier distinction observable in tests.

Concrete actions:

- Add or adjust focused backend coverage only where it asserts the general
  capability rule.
- Include a nearby dynamic pointer-derived byte-load case if existing coverage
  does not already protect idea 356 behavior.
- Keep expectations semantically stronger; do not downgrade supported
  contracts or mark cases unsupported.

Completion check:

- The focused coverage fails without the semantic repair and passes with it,
  or existing coverage is shown to already enforce the distinction.

### Step 4: Prove Residual And Preservation Set

Goal: demonstrate the residual advances while earlier completed behavior stays
intact.

Concrete actions:

- Run the supervisor-delegated narrow proof for
  `c_testsuite_aarch64_backend_src_00173_c`.
- Run preservation proof for string/global pointer publication from idea 365.
- Run preservation proof for dynamic pointer-derived byte-load behavior from
  idea 356.
- Recommend broader backend validation to the supervisor if the patch touches
  shared scalar memory, register allocation, or AArch64 address materialization.

Completion check:

- `00173` advances beyond the recorded byte-value-as-address first bad fact or
  passes, preservation proofs are green, and `todo.md` records any broader
  validation recommendation.
