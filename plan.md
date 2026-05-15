# F128 Full-Width Constant Carriers Runbook

Status: Active
Source Idea: ideas/open/241_f128_full_width_constant_carriers.md

## Purpose

Transcribe the F128 constant-carrier dependency into small executable steps
that give binary128 constants exact 16-byte payload authority before any
target backend claims structured F128 constant transport.

## Goal

Represent binary128 constants as structured full-width carrier facts that
preserve both 64-bit halves, or equivalent exact 16-byte provenance, through
BIR or prepared state and expose that fact to AArch64 selection.

## Core Rule

Do not lower an `F128` constant through `F64`, scalar floating approximations,
rendered assembly text, `immediate`, `immediate_bits`, or any other single
64-bit payload. Constant progress requires a structured full 16-byte fact that
target selection can inspect.

## Read First

- `ideas/open/241_f128_full_width_constant_carriers.md`
- `ideas/open/237_aarch64_binary128_softfloat_lowering.md`
- `src/backend/bir/bir.hpp`
- `src/backend/prealloc/regalloc.cpp`
- `src/backend/prealloc/legalize.cpp`
- `src/backend/prealloc/prepared_printer.cpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`
- `tests/backend/mir/backend_aarch64_prepared_memory_operand_records_test.cpp`

## Current Targets

- Target-neutral representation for binary128 constants with exact 16-byte
  payload provenance.
- Prepared or shared carrier state that transports that payload to backend
  selection without target-local reconstruction.
- AArch64 dispatch proof that valid F128 constants are distinguishable from
  missing or partial carrier facts.
- Focused tests that preserve scalar FP and i128 immediate behavior.

## Non-Goals

- Do not add AArch64 constant assembly printing before the structured carrier
  exists and is selected.
- Do not lower F128 constants through scalar `F32`, `F64`, `double`,
  `immediate`, `immediate_bits`, or single-lane integer payloads.
- Do not add arithmetic, comparison, cast, sign-bit, or helper-call lowering
  except where needed to prove a constant carrier reaches an existing normal
  backend consumer.
- Do not rewrite unrelated i128, scalar FP, atomic, intrinsic, inline-assembly,
  or binary128 helper-call routes.
- Do not weaken unsupported expectations or external testcase contracts to
  claim F128 constant progress.

## Working Model

- Treat existing scalar immediates as insufficient for binary128 constants.
- Prefer a target-neutral BIR or prepared payload record that names both halves
  explicitly or otherwise carries exact 16-byte provenance.
- Keep AArch64 as the first consumer, not the owner of the constant fact.
- Preserve fail-closed behavior whenever the full-width payload is absent,
  partial, or not connected to the selected value.

## Execution Rules

- Each code-changing step must run the supervisor-delegated build or compile
  proof and the matching narrow backend test subset.
- Add focused record-level tests before final assembly text tests.
- Preserve existing scalar FP and i128 immediate behavior after every step
  touching shared BIR, prealloc, prepared printing, dispatch, or machine record
  code.
- If execution discovers that frontend literal parsing or non-backend constant
  modeling needs a separate durable route, stop and ask the supervisor to
  create or activate a separate idea instead of expanding this plan.

## Ordered Steps

### Step 1: Inventory Current Constant Payload Authority

Goal: establish the exact current representation and failure mode for F128
constants before adding a new carrier.

Primary targets:
- `src/backend/bir/bir.hpp`
- `src/backend/prealloc/legalize.cpp`
- `src/backend/prealloc/regalloc.cpp`
- existing scalar immediate and F128 backend tests

Actions:
- Inspect current `bir::Value`, immediate, constant, legalize, and prepared
  storage paths for how literal payloads are represented.
- Identify every current `F128` constant branch, fallback, diagnostic, and
  scalar immediate path that would lose high-half payload data.
- Add or update focused fail-closed tests for the first unsupported F128
  constant path without adding target-local assembly printing.

Completion check:
- Tests document that current F128 constants do not claim support without a
  full-width payload carrier.
- `todo.md` records the first implementation packet and proof command.

### Step 2: Define The Full-Width Constant Carrier

Goal: add the target-neutral data model for exact binary128 constant payloads.

Primary targets:
- `src/backend/bir/bir.hpp`
- shared prepared structures consumed by AArch64 dispatch
- `src/backend/prealloc/prepared_printer.cpp`

Actions:
- Define a structured binary128 constant payload with low/high halves or
  equivalent exact 16-byte provenance.
- Connect the carrier to the appropriate BIR or prepared value identity so a
  backend can prove which `F128` value owns the payload.
- Add prepared-printer or diagnostic visibility only for structured facts, not
  rendered assembly text shortcuts.
- Preserve scalar immediate and i128 payload semantics unchanged.

Completion check:
- Record or printer tests prove both halves, or the exact 16-byte provenance,
  survive through the shared carrier.
- Existing scalar FP and i128 immediate tests still pass under the delegated
  subset.

### Step 3: Transport F128 Constants Through Prepared State

Goal: make prepared output carry full-width F128 constants to normal backend
selection surfaces.

Primary targets:
- `src/backend/prealloc/regalloc.cpp`
- prepared value-home and storage-plan records
- backend prepared-record tests

Actions:
- Attach the full-width constant carrier to prepared value homes or storage
  plans where rematerializable constants are represented.
- Reject or diagnose partial F128 payloads rather than falling back to scalar
  immediate materialization.
- Keep named prepared F128 carriers and 16-byte memory operands compatible with
  the new constant-carrier path.

Completion check:
- Prepared-record tests cover valid full-width F128 constant transport and the
  missing-carrier diagnostic.
- Existing named F128 carrier, scalar immediate, and i128 behavior remains
  unchanged.

### Step 4: Expose Constant Carriers To AArch64 Selection

Goal: let AArch64 dispatch consume only structured full-width F128 constant
facts and fail closed otherwise.

Primary targets:
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`

Actions:
- Add selected record fields or operand metadata for F128 constants that carry
  the full 16-byte payload fact.
- Lower AArch64 F128 constant cases only when the prepared or BIR carrier
  proves the exact payload.
- Preserve explicit diagnostics for missing, partial, or scalar-only constant
  facts.

Completion check:
- Dispatch tests prove AArch64 can distinguish a valid full-width F128
  constant carrier from unsupported constant transport.
- No AArch64 printer support is claimed unless the selected record has complete
  structured facts.

### Step 5: Prove The Dependency And Guard Adjacent Routes

Goal: finish the constant-carrier dependency without broadening into unrelated
binary128 or machine-node work.

Primary targets:
- focused backend route tests under `tests/backend`
- scalar FP and i128 immediate regression subset chosen by the supervisor

Actions:
- Add a representative proof that a binary128 constant reaches an existing
  normal backend consumer as a full-width structured payload.
- Run the supervisor-selected narrow proof, then broader backend validation if
  shared BIR or prepared structures changed.
- Verify the diff contains no expectation downgrade, named-case matching,
  scalar `F64` approximation, or target-local constant reconstruction.

Completion check:
- Narrow F128 constant-carrier proof is green.
- Broader backend proof is green when required by the supervisor.
- `todo.md` records final proof and any remaining unsupported F128 constant
  assembly-printing work as follow-up, not as hidden support.
