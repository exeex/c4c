# AArch64 Structured ASM Encoder Linker Contract

Status: Closed
Created: 2026-05-14
Closed: 2026-05-14

Depends On:
- `ideas/closed/211_aarch64_machine_instruction_node_contract.md`
- `ideas/closed/216_aarch64_machine_node_asm_printer_external_smoke.md`

## Goal

Clarify and enforce the structured internal contract after AArch64 machine
instruction nodes when the backend is not using the terminal `.s` printer path.

`src/backend/mir/aarch64/codegen/machine_printer.cpp` has the same role as the
LIR printer: it is a terminal text renderer for the `--codegen asm` route. It
may directly render text. If a future mode compiles past machine nodes into an
internal assembler, encoder, object writer, or linker, that later pipeline must
consume structured machine instruction nodes or lower structured encoding
records. It must not consume `.s` text printed by `machine_printer.cpp` and
parse it back into semantics.

The internal assembler/encoder/object/linker path must preserve the complete
semantic payload carried by earlier nodes, the same way BIR must preserve the
semantic payload from frontend/lowering. The project is not trying to become an
external `.ll` or `.s` ingestion tool. External LLVM IR or assembly input is
rejected as the normal internal compilation route because using that route
would discard the structured information this backend is designed to preserve.

## Why This Idea Exists

Idea 216 added:

```text
machine instruction nodes -> machine_printer.cpp -> .s text -> external clang/as
```

That path is valid because `.s` is terminal output for external tooling and
debugging. The printer's job is to render text, just like the LIR printer
renders LLVM IR text from LIR structures.

The remaining risk is a future "compile all the way down" route accidentally
doing this:

```text
machine instruction nodes -> .s text -> internal assembler parser -> encoder/object
```

That route is rejected. The internal route must instead be:

```text
machine instruction nodes
  -> structured asm/encoding operator records
  -> object records carrying sections, symbols, relocations, and provenance
  -> linker records
  -> binary output
```

The structured asm/encoding layer is allowed to look like operator nodes with
operand nodes, but its purpose is not to feed the terminal printer. Its purpose
is to keep machine-node semantics, relocation needs, symbol ids, section
ownership, operand constraints, and provenance alive until binary emission.

The record surface should be readable in the same spirit as LIR's explicit
module/function/block/instruction structs. A generic `OperandRecord` plus enum
may be acceptable as a dispatch wrapper, but the semantic payload should not be
one oversized catch-all struct. Register, memory, symbol, label, relocation,
section, and directive records should each expose fields grouped by purpose.

The structured asm/encoding layer should consume the prepared lifecycle facts
instead of rebuilding them. Existing prepared authority includes
`PreparedLiveness`, `PreparedLiveInterval`, `PreparedRegalloc`,
`PreparedRegallocValue`, `PreparedInterferenceEdge`,
`PreparedMoveResolution`, `PreparedSpillReloadOp`, `PreparedValueLocations`,
`PreparedCallPreservedValue`, and `PreparedClobberedRegister`. These are the
source facts for value homes, live ranges, interference, move authority,
spill/reload placement, and call preservation/clobber policy.

Those prepared facts are not a complete replacement for machine-level
asm/encoding records. The structured asm layer must still add target-machine
effects that only exist after instruction selection, such as implicit register
uses/defs, flags, subregister width effects, selected opcode clobbers, scratch
acquire/release ranges, and section/relocation/object ownership.

## In Scope

- Document the accepted split between:
  - terminal `.s` printer output for `--codegen asm`
  - structured internal encoder/object input for any future compile-through
    route
- Document that external `.ll` and external `.s` are not accepted as normal
  internal compilation inputs for this backend. They may be test fixtures or
  external-toolchain smoke outputs only, not semantic source for codegen,
  encoder, object writer, or linker.
- Audit AArch64 assembler, parser, encoder, and codegen markdown/header
  surfaces for wording that implies internal compilation should parse
  `machine_printer.cpp` output.
- Update relevant contracts and roadmap docs so `machine_printer.cpp` is
  explicitly classified as a terminal renderer, not the assembler/encoder
  semantic input.
- Define the minimum future encoder input contract:
  - machine instruction node input, or
  - deliberately lower structured asm/encoding operator records derived from
    machine instruction nodes
- Define a readable class/struct surface for the structured asm/encoding
  stream, such as:
  - module or translation-unit record
  - section record
  - label record
  - operator/instruction record
  - directive record
  - data record
  - typed operand records
  - relocation-need record
- Keep operand records purpose-oriented. For example, register records should
  separate the physical register reference, use/role, allocation provenance,
  value provenance, and clobber/effect metadata instead of collecting every
  possible register-related fact in one flat unreadable struct.
- Include a concrete register-record sketch in the accepted contract so the
  implementation has a readable target shape. The final names may differ, but
  the separation of concerns should be preserved:

```cpp
struct AsmRegisterRef {
  c4c::backend::aarch64::abi::RegisterReference reg;
  c4c::backend::aarch64::abi::RegisterView view;
};

enum class AsmRegisterUseKind {
  Input,
  Output,
  InOut,
  Scratch,
  Clobber,
  AddressBase,
  CallArgument,
  ReturnValue,
};

struct AsmRegisterUse {
  AsmRegisterUseKind kind;
  bool implicit = false;
};

struct AsmValueProvenance {
  std::optional<c4c::backend::prepare::PreparedValueId> value_id;
  c4c::ValueNameId value_name = c4c::kInvalidValueName;
  c4c::backend::bir::TypeKind type = c4c::backend::bir::TypeKind::Void;
};

struct AsmAllocationProvenance {
  c4c::backend::prepare::PreparedRegisterClass prepared_class =
      c4c::backend::prepare::PreparedRegisterClass::None;
  c4c::backend::prepare::PreparedRegisterBank prepared_bank =
      c4c::backend::prepare::PreparedRegisterBank::None;
  std::size_t contiguous_width = 1;
  std::vector<c4c::backend::aarch64::abi::RegisterReference> occupied_registers;
};

struct AsmRegisterOperand {
  AsmRegisterRef reg;
  AsmRegisterUse use;
  std::optional<AsmValueProvenance> value;
  std::optional<AsmAllocationProvenance> allocation;
};
```

- Add explicit enum-to-string mapping functions for the structured
  asm/encoding stream. The terminal printer and diagnostics should call these
  mappings instead of scattering spelling switches through lowering code.
  Required mappings include every new enum kind introduced for sections,
  labels, directives, operator/opcode categories, operand kinds, register use
  kinds, relocation kinds, and record surfaces.
- Define the minimum semantic payload that must survive toward object/linker
  work, including:
  - typed operands
  - register and immediate facts
  - prepared liveness intervals, use/definition points, and interference facts
    where they are relevant to validation or later optimization
  - prepared move, spill/reload, call-preserved, and call-clobbered facts
  - symbol ids and link-name ids
  - relocation needs
  - section/data ownership
  - branch/data references
  - source provenance back to BIR/prepared facts
  - side effects, clobbers, and volatility/address-space facts where relevant
- Preserve the `--codegen asm` behavior established by idea 216.
- Add focused guard tests or compile checks only if existing code exposes a
  path that could accidentally parse emitted `.s` as internal backend input.

## Out Of Scope

- Refactoring `machine_printer.cpp` merely because it uses string rendering.
- Adding an assembly token-node arena for the terminal printer.
- Implementing the full internal assembler, parser, encoder, ELF writer,
  object writer, linker, or binary emitter.
- Accepting external `.ll` or `.s` as a supported backend input language.
- Removing the external clang/as smoke validation from idea 216.
- Expanding instruction selection or printer opcode coverage.

## Acceptance Criteria

- AArch64 docs/contracts clearly state that `machine_printer.cpp` is a terminal
  renderer for the `--codegen asm` path, analogous to the LIR printer.
- Future internal compile-through routes are documented to consume structured
  machine instruction nodes or lower structured asm/encoding records, never
  printed `.s` text parsed back into operands.
- The structured asm/encoding record surface is defined as a readable series
  of classes/structs, not only as a broad enum plus an oversized operand
  payload.
- The accepted contract includes a concrete register operand decomposition
  separating register reference, register use, value provenance, and allocation
  provenance.
- The contract identifies prepared liveness/regalloc/move/spill/call facts as
  the upstream authority and states which machine-level effects must be added
  after instruction selection.
- Printer/diagnostic spelling for structured asm/encoding enums is centralized
  through enum-kind-to-string mapping functions.
- The internal asm/encoding/object/linker roadmap preserves complete upstream
  node semantics through structured records instead of reducing the route to
  external `.ll` or `.s` text.
- External `.ll` and `.s` ingestion are explicitly rejected as the normal
  backend compilation route.
- Existing assembler/parser/encoder markdown no longer suggests that codegen
  should feed printed assembly to an internal parser as the normal backend
  pipeline.
- The idea 216 external `.c -> .s -> clang/as -> run` path remains valid.

## Completion Note

Closed after the active runbook completed the AArch64 structured asm contract
slice. The accepted contract now separates terminal `--codegen asm` text from
future internal structured asm/encoding, object, and linker records; records the
required register operand decomposition; ties structured records to prepared
liveness/regalloc/move/spill/call authority; requires centralized enum spelling
for future structured enums; rejects external `.ll` and `.s` as normal backend
inputs; and records that no current live AArch64 route feeds
`machine_printer.cpp` output back into parser/encoder/object/linker semantics.

Close-time regression guard used the accepted full-suite baseline in
`test_before.log` and a current full-suite run in `test_after.log`; both had
3162 passing tests and no failures. The docs-only close used the
non-decreasing-pass regression-guard mode.

## Reviewer Reject Signals

- The route treats direct string rendering in `machine_printer.cpp` as a bug by
  itself.
- The route introduces a token arena only to rewrap terminal printer text
  without serving a real internal contract.
- Any internal assembler, encoder, or object route consumes `.s` emitted by
  `machine_printer.cpp` and reparses it for semantics.
- The route accepts external `.ll` or `.s` as a normal way to enter the
  backend, encoder, object writer, or linker.
- The structured record design copies the current over-broad operand shape and
  makes register, memory, symbol, label, relocation, and provenance data hard
  to read or validate.
- Enum spelling is hand-coded opportunistically in multiple printer/lowering
  sites instead of being centralized in mapping helpers.
- Structured facts such as symbol ids, relocation needs, volatility,
  address-space, side effects, or provenance are dropped and replaced with
  recover-from-text behavior.
- The update weakens or removes machine instruction nodes as the semantic
  boundary after target MIR.
