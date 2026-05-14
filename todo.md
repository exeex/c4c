Status: Active
Source Idea Path: ideas/open/229_aarch64_codegen_markdown_shards_to_cpp.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Reconcile Route Boundary And Stale Structural Shards

# Current Packet

## Just Finished

Completed `plan.md` Step 3 ledger reconciliation for the route-boundary and
stale structural shard group: `mod.md`, `records.md`, and `asm_emitter.md`.
Each entry below was checked against current compiled owners or explicit route
boundary files before classification.

Shard: `src/backend/mir/aarch64/codegen/mod.md`
Classification: stale/reject-retire
Current Owner: `src/backend/mir/aarch64/module/module.cpp`,
`src/backend/mir/aarch64/module/module.hpp`,
`src/backend/mir/aarch64/api/api.cpp`,
`src/backend/mir/aarch64/codegen/emit.cpp`,
`src/backend/mir/aarch64/codegen/traversal.cpp`, and the shared MIR printer
boundary in `src/backend/mir/printer.hpp` with the AArch64 target printer in
`src/backend/mir/aarch64/codegen/machine_printer.cpp`
Review Result: The shard is a historical Rust-style module index and is not a
live dependency graph or C++ visibility contract. The current route already has
an explicit public facade, prepared-module build entry point, function/block
lowering traversal, compatibility projection, and terminal printer boundary.
The shard's old list of modules is stale as ownership evidence and must not be
used to recreate same-named translation units, direct text emission, or broad
module-emitter ownership. Its only narrow valid fact is the current boundary
direction: structured target MIR records flow to selected machine nodes, and
terminal assembly text is a printer consumer rather than semantic input.
Proof or Evidence: `module.cpp:7` delegates `module::build` to
`codegen::build_module`; `api.cpp:7` exposes `build_prepared_module`;
`emit.cpp:11` builds the module after handoff validation; `traversal.cpp:59`
lowers prepared functions into `module::MachineFunction` / `MachineModule`;
`backend.cpp:158` builds the prepared module and `backend.cpp:181` prints each
function through `MachineInstructionPrinter`; `printer.hpp:51` and `:101`
define the shared machine function/module printer boundary.
Follow-Up: retire note only; no follow-up idea and no same-named `mod.cpp` /
`mod.hpp` recreation.

Shard: `src/backend/mir/aarch64/codegen/records.md`
Classification: already-converted/reconcile-ledger
Current Owner: `src/backend/mir/aarch64/codegen/instruction.hpp`,
`src/backend/mir/aarch64/codegen/instruction.cpp`,
`src/backend/mir/aarch64/codegen/operands.cpp`,
`src/backend/mir/aarch64/codegen/alu.cpp`,
`src/backend/mir/aarch64/codegen/comparison.cpp`,
`src/backend/mir/aarch64/codegen/returns.cpp`,
`src/backend/mir/aarch64/codegen/dispatch.cpp`,
`src/backend/mir/aarch64/codegen/machine_printer.cpp`,
and the shared MIR container/printer boundary under `src/backend/mir/`
Review Result: Current-owner review proves only the narrow valid record facts:
the compiled backend has typed record-surface vocabulary, operand/resource
records, branch/scalar/memory/spill/return record families, selected
machine-node status, printer mnemonic mapping, and fail-closed terminal
printing for selected machine nodes. The shard's broader roadmap notes about
future asm/encoding streams, object records, linker behavior, and enum growth
remain non-authoritative until a later focused route owns them. Step 3 should
not turn this markdown shard into broad record-pile ownership or a
`records.cpp` / `records.hpp` conversion.
Proof or Evidence: `instruction.hpp:28` defines `RecordSurfaceKind` with
target-MIR, machine-node, printer-output, encoder-input, and external-assembler
input surfaces; `instruction.hpp:70` defines broad instruction families;
`instruction.hpp:93` defines selected machine opcodes; `instruction.hpp:130`
defines selection status; `instruction.cpp:29`, `:122`, `:142`, and `:192`
centralize display names for surfaces, families, opcodes, and printer
mnemonics; `machine_printer.cpp:72` requires selected machine-node surfaces
before printing; `machine_printer.cpp:359` dispatches only the printable
selected subset.
Follow-Up: none for Step 3; future asm/encoding or object-record expansion
needs a separate focused idea only when current compiled owners require it.

Shard: `src/backend/mir/aarch64/codegen/asm_emitter.md`
Classification: stale/reject-retire
Current Owner: none for the archived inline-assembly emitter semantics;
related live boundaries are the structured record vocabulary in
`src/backend/mir/aarch64/codegen/instruction.hpp`, the selected-node printer in
`src/backend/mir/aarch64/codegen/machine_printer.cpp`, and the external
assembler compatibility surface under `src/backend/mir/aarch64/assembler/`
Review Result: The shard describes a removed inline-assembly emitter with
target-local constraint classification, scratch allocation, operand movement,
template substitution, memory formatting, and immediate validation. Those
details are stale relative to the current compiled route. The live backend does
not own a prepared inline-asm lowering path here, and the external assembler
parser/encoder surface is explicitly not an internal bridge from printed
`--codegen asm` back into semantic backend input. Recreating this shard would
reopen standalone scratch allocation, callee-save repair, and text-first
inline-asm emission outside the current prepared authority boundary.
Proof or Evidence: `instruction.hpp:153` has only side-effect vocabulary for
`InlineAssembly`; `assembler/parser.hpp:11` says the parser is for external
assembler text and must not recover backend semantics from printed
`machine_printer.cpp` output; `assembler/mod.hpp:8` marks the assembler API as
external text compatibility, not the compile-through bridge; `assembler/encoder/mod.hpp:11`
keeps encoder helpers downstream of parsed external assembler operands;
`c4cll.cpp:333` states AArch64 asm output is selected machine-node printer
output and `c4cll.cpp:334` says it is not parsed back, encoded, or linked.
Follow-Up: retire note only; no follow-up idea from this packet because current
owners do not prove a narrow missing inline-asm feature to repair here.

## Suggested Next

Execute `plan.md` Step 4 by reconciling only the ABI, call, frame, and global
shard group: `calls.md`, `prologue.md`, `variadic.md`, and `globals.md`.

## Watchouts

- Do not mechanically convert markdown shards into same-named C++ files.
- Do not reopen the shared MIR printer boundary closed by idea 224.
- Treat `records.md` as already represented only for the narrow facts proven by
  current compiled owners; its future structured asm/object roadmap text is not
  a Step 3 implementation mandate.
- Do not recreate the stale `asm_emitter.md` inline-asm emitter, scratch
  allocator, template substitution path, or text-first assembler handoff.
- Do not classify a missing feature before checking current compiled owners.
- Keep compatibility projection out of terminal assembly printing; terminal
  assembly must walk shared `module::MachineModule` through
  `mir::print_machine_module` plus the AArch64 `MachineInstructionPrinter`.
- The Step 3 ledger deliberately does not promote every archived route-boundary
  or inline-asm detail into missing-feature work. Step 4 should still check
  current ABI/call/frame/global owners before deciding whether any real missing
  feature exists.

## Proof

Ledger-only classification; no build or test proof required, and no broad
validation was run. No `test_after.log` was produced because the delegated
proof explicitly required ledger-only classification.

Evidence inspected with focused `rg`, `sed`, and `nl` reads of:

- `ideas/open/229_aarch64_codegen_markdown_shards_to_cpp.md`
- `src/backend/mir/aarch64/codegen/mod.md`
- `src/backend/mir/aarch64/codegen/records.md`
- `src/backend/mir/aarch64/codegen/asm_emitter.md`
- `src/backend/mir/aarch64/codegen/emit.cpp` / `.hpp`
- `src/backend/mir/aarch64/codegen/traversal.cpp` / `.hpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp` / `.hpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp` / `.hpp`
- `src/backend/mir/aarch64/codegen/compatibility_projection.cpp` / `.hpp`
- `src/backend/mir/aarch64/module/module.cpp` / `.hpp`
- `src/backend/mir/aarch64/api/api.cpp` / `.hpp`
- `src/backend/mir/printer.cpp` / `.hpp`
- `src/backend/backend.cpp`
- `src/apps/c4cll.cpp`
- `src/backend/mir/aarch64/assembler/parser.hpp`
- `src/backend/mir/aarch64/assembler/mod.hpp`
- `src/backend/mir/aarch64/assembler/encoder/mod.hpp`
