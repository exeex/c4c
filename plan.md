# RV64 EV64 `.insn.d` RISC-V Length Prefix Alignment Runbook

Status: Active
Source Idea: ideas/open/385_rv64_ev64_insn_d_riscv_length_prefix.md
Activated from: ideas/open/385_rv64_ev64_insn_d_riscv_length_prefix.md

## Purpose

Repair the EV64 `.insn.d` 64-bit instruction-word encoding so emitted RV64
object bytes follow the RISC-V variable-length instruction prefix convention.

## Goal

Make every supported EV64 `.insn.d` word use low 7 bits `0x3f`, keep c4c's
own roundtrip/extractor behavior coherent with the new layout, and prove LLVM
RISC-V objdump consumes the bytes as one 8-byte unknown instruction.

## Core Rule

Change the general EV64 `.insn.d` field layout, not a fixture-specific expected
hex string. Preserve existing `.insn.d` operand allocation and validation
semantics unless the source idea explicitly requires a layout migration.

## Read First

- `ideas/open/385_rv64_ev64_insn_d_riscv_length_prefix.md`
- EV64 `.insn.d` encoder and decoder/extractor implementation surfaces
- RV64 inline-asm/object-emission tests that assert EV64 `.insn.d` bytes
- RV64 roundtrip contract documentation or fixture notes for EV64 encoding

## Current Targets

- EV64 `.insn.d` binary layout
- c4c EV64 encoder
- c4c EV64 decoder / objdump extractor / roundtrip path
- tests for canonical `.insn.d` emission, operand substitution, and malformed
  inputs
- LLVM RISC-V objdump proof for one 8-byte unknown instruction

## Non-Goals

- Do not add upstream LLVM decoder-table support for c4c EV64 instructions.
- Do not add arbitrary RISC-V long-instruction support beyond this EV64
  `.insn.d` template.
- Do not reinterpret existing RV64I 32-bit instruction encodings.
- Do not add semantic lowering for new EV operations.
- Do not weaken `.insn.d` parser validation or register/immediate checks.
- Do not expand into unrelated RV64 object-route lowering work.

## Working Model

The old EV64 `.insn.d` encoding placed a c4c major opcode in bits `[6:0]`, so
the canonical fixture could emit a low byte such as `0x0a`. LLVM's RISC-V
disassembler uses the low bits of the first byte to classify instruction
length. A 64-bit RISC-V slot is recognized when bits `[6:0]` are `0x3f`; any
other low-bit pattern can make LLVM split the EV64 word into smaller
instructions. The new layout must dedicate bits `[6:0]` to the length prefix
and move c4c/user opcode data elsewhere.

## Execution Rules

- Pin the LLVM objdump behavior with a byte-level probe before changing the
  c4c encoder.
- Document the exact new field mapping before or alongside implementation.
- Keep reserved bits `[47:45]` emitted as zero unless a separate idea claims
  them.
- Preserve vector register operand allocation/substitution coverage.
- Run a narrow proof for EV64 `.insn.d` roundtrip and LLVM disassembly, then
  the supervisor-selected backend regression scope.

## Steps

### Step 1: Prove The 64-Bit Prefix Contract

Goal: capture a byte-level LLVM objdump proof for an EV64 candidate word whose
low 7 bits are `0x3f`.

Primary target: a minimal generated object or byte fixture independent of the
c4c encoder.

Actions:

- Construct a candidate 8-byte EV64 word with bits `[6:0] = 0x3f`.
- Run it through LLVM RISC-V objdump without c4c-specific decoder support.
- Confirm objdump consumes one 8-byte unknown instruction rather than smaller
  fragments.
- Record the command, bytes, and expected output in `todo.md`.

Completion check: `todo.md` contains a reproducible LLVM objdump proof and the
exact length-prefix expectation to pin in tests.

### Step 2: Map The New EV64 Field Layout

Goal: define the precise bit allocation used by the encoder and extractor.

Primary target: EV64 layout helpers and RV64 roundtrip contract notes.

Actions:

- Map the 25-bit rv32-like payload in bits `[31:7]`.
- Assign the c4c/user opcode to bits `[39:32]`.
- Assign `rs4/msk` to bits `[44:40]`.
- Keep bits `[47:45]` reserved and zero.
- Assign `dtype imm16` to bits `[63:48]`.
- Record migration notes for old expected values in `todo.md`.

Completion check: `todo.md` names every field, width, and reserved-bit policy
before implementation starts.

### Step 3: Update Encoder, Extractor, And Contract Tests

Goal: implement the new EV64 `.insn.d` layout and update c4c-owned roundtrip
coverage.

Primary target: EV64 `.insn.d` encoder, decoder/extractor, and RV64
roundtrip/object-emission tests.

Actions:

- Update the encoder so all supported EV64 `.insn.d` words have low 7 bits
  `0x3f`.
- Update the extractor/roundtrip path to decode the new field locations.
- Update tests that assert the old low-opcode layout.
- Add or update tests proving malformed or unsupported layouts still reject.
- Preserve vector register operand allocation/substitution behavior.

Completion check: focused EV64 `.insn.d` tests pass and no old low-7-bit c4c
major-opcode compatibility path remains reachable without explicit rationale.

### Step 4: Prove LLVM And c4c Roundtrip Compatibility

Goal: validate the repaired behavior through both external LLVM objdump and
c4c's own extractor.

Primary target: canonical `.insn.d` fixture and object/disassembly artifacts.

Actions:

- Emit the canonical `.insn.d` fixture with c4c.
- Verify the emitted 8-byte word has low 7 bits `0x3f`.
- Verify c4c's own objdump/extractor roundtrips the canonical spelling.
- Verify LLVM RISC-V objdump consumes the EV64 bytes as one 8-byte unknown
  instruction.
- Record proof commands and logs in `todo.md`.

Completion check: `todo.md` records passing c4c roundtrip proof and LLVM
one-instruction disassembly proof for the emitted c4c bytes.

### Step 5: Run Backend Regression And Decide Closure

Goal: confirm the encoding migration did not regress adjacent backend behavior
and route lifecycle disposition.

Primary target: supervisor-selected backend regression scope and idea 385
acceptance criteria.

Actions:

- Run the delegated build and backend regression command.
- Compare against the canonical baseline with the regression guard.
- Confirm all acceptance bullets in idea 385 are satisfied.
- If a later or adjacent boundary appears, record it precisely and split it
  rather than expanding this idea.
- Hand the active plan to the plan owner for closure decision when complete.

Completion check: `todo.md` records regression results and either closure-ready
acceptance evidence for idea 385 or the exact remaining lifecycle blocker.
