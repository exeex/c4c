# RV64 Standard `.insn` Inline Asm Stage 1 Runbook

Status: Active
Source Idea: ideas/open/340_rv64_standard_insn_inline_asm_stage1.md
Activated from: ideas/open/339_rv64_inline_asm_custom_vector_encoding_umbrella.md

## Purpose

Create the first executable child route for RV64 inline asm custom encoding by
supporting standard RISC-V `.insn` with scalar constraints and object-route
proof.

## Goal

An RV64 source case using standard `.insn r ... %0, %1, %2` compiles through
c4c, allocates/substitutes scalar registers, and emits verifiable object bytes.

## Core Rule

Do not claim `.insn` support through external assembler delegation, testcase
string matching, or expectation downgrades. The route must repair the compiler
capability that later vector and EV children will extend.

## Read First

- `ideas/open/340_rv64_standard_insn_inline_asm_stage1.md`
- `ideas/open/339_rv64_inline_asm_custom_vector_encoding_umbrella.md`
- Existing inline-asm carrier, metadata, and backend substitution code.
- Existing RV64 backend object-emission tests and CMake helpers.

## Current Scope

- Standard RV64 `.insn` inline asm.
- Scalar GPR constraints: `r`, `=r`, `+r`, and required matching/tied forms.
- Positional operands such as `%0`, `%1`, `%2`.
- `asm volatile`, clobber, and memory-side-effect preservation.
- Object-route emitted-byte proof for at least one standard RISC-V encoding.

## Non-Goals

- Vector register classes or `VR*` constraints.
- EV `.insn.d` syntax or 64-bit custom encoding.
- Named operands and `%c[...]` template modifiers.
- Runtime or consteval/template-produced asm strings.
- Broad RVV semantics or compiler-known EV operation mnemonics.

## Working Model

- Keep generic inline-asm carrier fixes only when they are necessary for RV64
  `.insn` Stage 1.
- Prefer target-owned parsing/classification of supported `.insn` forms over
  raw template passthrough.
- Preserve operand order and constraint metadata from frontend/HIR through BIR
  and into backend/object emission.
- Use tests to pin malformed input diagnostics and emitted bytes.

## Execution Rules

- Keep changes narrowly tied to the Stage 1 source idea.
- Add diagnostics instead of silently accepting unsupported constraints or
  malformed `.insn` forms.
- Do not weaken existing tests or mark supported-path tests unsupported.
- If a missing prerequisite is separable from Stage 1, stop and ask the
  supervisor to create a focused child idea instead of expanding this plan.
- Each code-changing step needs `cmake --build --preset default` plus the
  supervisor-selected narrow proof, normally `ctest --test-dir build -j
  --output-on-failure -R '^backend_'`.

## Steps

### Step 1: Map Existing Inline-Asm And RV64 Object Route

Goal: identify the current carrier, metadata, substitution, and object-emission
surfaces before changing code.

Primary targets:

- `src/codegen/lir/`
- `src/backend/bir/`
- `src/backend/prealloc/`
- `src/backend/mir/aarch64/codegen/inline_asm.*`
- RV64 backend/object route files and backend CMake tests.

Actions:

- Inspect how inline asm is represented from frontend/HIR into LIR/BIR.
- Identify whether RV64 has an inline-asm carrier, allocator hook, and object
  emission path or needs one.
- Locate the narrowest backend test harness that can prove emitted bytes.
- Record any separable prerequisite as a blocker rather than absorbing broad
  generic work silently.

Completion check:

- The executor can name the concrete files/functions that must change for
  `.insn` parsing/classification, scalar substitution, and object proof.

### Step 2: Preserve And Classify Stage 1 Constraints

Goal: make the compiler retain the scalar inline-asm contract needed by RV64
`.insn`.

Actions:

- Ensure `r`, `=r`, `+r`, and needed matching/tied scalar constraints survive
  the current lowering layers.
- Keep volatile, clobber, and memory-side-effect metadata visible to backend
  consumers.
- Reject unsupported Stage 1 constraints explicitly instead of treating them as
  generic strings.
- Add focused frontend/HIR or BIR tests if the carrier changes before backend
  emission can use it.

Completion check:

- A `.insn` fixture reaches backend-visible metadata with the expected operand
  ordering and scalar constraint classification.

### Step 3: Implement RV64 `.insn` Substitution And Encoding

Goal: support the standard `.insn` form needed for the first object proof.

Actions:

- Parse or classify the selected `.insn r` operand shape enough to encode it
  deterministically.
- Allocate legal RV64 scalar registers for operands using the Stage 1
  constraints.
- Substitute positional placeholders with stable RV64 register spellings or
  register numbers as required by the encoder.
- Emit the resulting standard RISC-V instruction bytes through c4c's object
  route.
- Diagnose malformed `.insn` fields and impossible or unsupported operands.

Completion check:

- The positive Stage 1 `.insn r` fixture emits the expected instruction bytes
  from c4c's object path, and negative fixtures diagnose unsupported forms.

### Step 4: Harden Proof And Boundaries

Goal: make Stage 1 acceptance resistant to overfit and ready for later child
ideas.

Actions:

- Add or update backend tests for positive object bytes, bad `.insn` fields,
  unsupported constraints, and tied/read-write scalar behavior if implemented.
- Confirm no external assembler is the primary proof path.
- Confirm vector constraints, EV `.insn.d`, named operands, and consteval
  template strings remain out of scope.
- Run the selected narrow proof and any frontend/HIR proof required by changed
  layers.

Completion check:

- Build and selected tests pass, the source idea's acceptance criteria are met,
  and the remaining umbrella stages can be started as separate child ideas.
