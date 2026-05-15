# AArch64 CRC And Vector Intrinsic Carriers Runbook

Status: Active
Source Idea: ideas/open/241_aarch64_crc_vector_intrinsic_carriers.md

## Purpose

Establish the target-neutral semantic and prepared carrier authority required
before AArch64 CRC, vector memory, and vector operation intrinsics can be
selected into machine records.

Goal: make CRC/vector intrinsic facts complete and testable without selecting
or printing AArch64 intrinsic machine instructions.

Core Rule: do not use intrinsic spelling, ordinary call plans, archived scratch
registers, or final assembly text as authority for CRC/vector support.

## Read First

- `ideas/open/241_aarch64_crc_vector_intrinsic_carriers.md`
- `ideas/open/239_aarch64_intrinsic_machine_nodes.md`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/lir_to_bir/calling.cpp`
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/prealloc.cpp`
- `src/backend/prealloc/prepared_printer.cpp`
- `tests/backend/bir/backend_prepared_printer_test.cpp`
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`

## Current Scope

- Add intrinsic family and operation facts for accepted CRC, vector memory, and
  vector operation cases.
- Preserve feature, width, lane, element type, signedness, operand count,
  result identity, memory operand, immediate, side-effect, and register/home
  authority where each accepted family requires it.
- Publish prepared carrier facts that future AArch64 machine-node selection can
  consume without string matching.
- Add diagnostics and tests for unsupported x86-only, missing-feature,
  missing-lane/type, missing-memory, and incomplete-carrier paths.

## Non-Goals

- Do not select or print AArch64 CRC/vector machine records.
- Do not rewrite generic vector lowering, atomics, inline asm, binary128
  helpers, or ordinary call lowering.
- Do not claim support for all x86 SSE/AES/CLMUL intrinsics on AArch64.
- Do not emit assembly text from intrinsic names or final call spelling.
- Do not weaken unsupported-path expectations without explicit user approval.

## Working Model

- BIR owns semantic intrinsic classification.
- Prepared intrinsic carriers own completeness and register/home authority.
- Prepared printer output and diagnostics are the proof surface for this
  dependency route.
- AArch64 machine-node selection remains a later consumer in
  `ideas/open/239_aarch64_intrinsic_machine_nodes.md`.

## Execution Rules

- Keep each step behavior-preserving except for the explicit addition of new
  complete carrier facts and fail-closed diagnostics.
- Prefer structured enum/data fields over name or fixture matching.
- Each accepted carrier family needs at least one complete representative test
  and nearby incomplete or unsupported tests.
- Run `cmake --build --preset default` before accepting code-changing packets.
- Use narrow CTest filters first, then escalate to broader backend validation
  when multiple carrier families or shared prepared structures are changed.

## Ordered Steps

### Step 1: Inventory accepted intrinsic families and carrier fields

Goal: define the minimum structured authority required for CRC, vector memory,
and vector operation carriers.

Primary targets:

- `src/backend/bir/bir.hpp`
- `src/backend/prealloc/prealloc.hpp`
- existing intrinsic tests under `tests/backend/bir/` and `tests/backend/mir/`

Actions:

- Inspect the existing scalar `fabs` intrinsic path from LIR/BIR lowering
  through prepared carriers and prepared-printer output.
- Decide the concrete enum values and carrier fields needed for one accepted
  CRC representative, one vector memory representative, and one vector
  operation representative.
- Identify unsupported x86-only and incomplete-fact cases that must remain
  fail-closed.

Completion check:

- The executor can point to the exact BIR/prepared fields each family needs,
  with no dependency on final assembly spelling or scratch registers.

### Step 2: Extend BIR semantic intrinsic facts

Goal: represent accepted CRC/vector families in BIR with enough semantic facts
for prepared lowering.

Primary targets:

- `src/backend/bir/bir.hpp`
- `src/backend/bir/lir_to_bir/calling.cpp`
- BIR lowering or notes tests as needed

Actions:

- Add structured intrinsic family/operation values for the accepted CRC,
  vector memory, and vector operation representatives.
- Thread operand/result, width/lane/type, memory/immediate, feature, and
  side-effect facts through `bir::IntrinsicOperation` or adjacent structured
  BIR data.
- Keep unsupported or incomplete calls diagnosed instead of silently converted
  to generic complete intrinsic facts.

Completion check:

- BIR-level tests prove complete semantic classification for the accepted
  representatives and fail-closed behavior for nearby unsupported or malformed
  cases.

### Step 3: Extend prepared intrinsic carriers

Goal: carry complete CRC/vector intrinsic authority through prepared facts.

Primary targets:

- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/prealloc.cpp`
- prepared-carrier lookup/helper code

Actions:

- Extend `PreparedIntrinsicCarrier` or add scoped companion structs so each
  accepted family carries feature, lane/type, operand/result, memory/immediate,
  side-effect, and register/home authority.
- Mark carriers complete only when all required family-specific facts are
  present.
- Preserve explicit `missing_required_facts` diagnostics for unsupported or
  incomplete cases.
- Do not treat `PreparedCallPlan` alone as enough authority for CRC/vector
  completion.

Completion check:

- Prepared-carrier tests prove complete representatives for CRC, vector memory,
  and vector operation cases, plus missing-feature, missing-lane/type,
  missing-memory, and unsupported x86-only failures.

### Step 4: Expose proof through prepared printer and diagnostics

Goal: make carrier completeness observable without implying selected machine
support.

Primary targets:

- `src/backend/prealloc/prepared_printer.cpp`
- `tests/backend/bir/backend_prepared_printer_test.cpp`
- existing diagnostic assertion tests

Actions:

- Print the structured fields needed to audit complete CRC/vector carriers.
- Print or preserve missing-fact diagnostics clearly for incomplete carriers.
- Keep the prepared-printer text distinct from AArch64 selected instruction
  output.

Completion check:

- Tests can prove carrier completeness and fail-closed diagnostics from the
  prepared proof surface alone.

### Step 5: Guard future AArch64 selection boundaries

Goal: ensure the current dependency route cannot be mistaken for selected
machine support.

Primary targets:

- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`
- any existing AArch64 intrinsic fail-closed tests

Actions:

- Add or adjust tests proving CRC/vector carriers do not yet select or print
  AArch64 machine records under this idea.
- Preserve scalar `fabs` selected-machine behavior from the previous route.
- Confirm unsupported x86-only paths still fail closed rather than fabricating
  zero-fill or scratch-register output.

Completion check:

- AArch64 dispatch/printer tests distinguish complete prepared carrier
  authority from selected machine-node support.

### Step 6: Run acceptance validation

Goal: prove the carrier dependency is complete enough for the future
machine-node route to consume.

Actions:

- Run `cmake --build --preset default`.
- Run narrow tests covering BIR intrinsic lowering, prepared carrier/printer
  output, and AArch64 fail-closed dispatch boundaries.
- Escalate to `ctest --test-dir build -j --output-on-failure -R '^backend_'`
  before the supervisor treats the idea as milestone-complete.

Completion check:

- All acceptance criteria in the source idea are satisfied, and
  `ideas/open/239_aarch64_intrinsic_machine_nodes.md` can later consume the
  new carriers without intrinsic-name matching or scratch-register conventions.
