Status: Active
Source Idea Path: ideas/open/269_aarch64_peephole_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish The Current Peephole Boundary

# Current Packet

## Just Finished

Completed plan Step 1, "Establish The Current Peephole Boundary", as an
audit-only packet.

Live AArch64 peephole behavior: none found. There is no
`peephole.{hpp,cpp}`, no compiled AArch64 `peephole_optimize` symbol, no
CMake entry for an AArch64 peephole translation unit, and no call from the
current AArch64 output path into any post-output cleanup pass.

Current output path owners inspected:

- `src/backend/backend.cpp`: AArch64 BIR/LIR entrypoints prepare BIR and call
  `aarch64::codegen::print_prepared_machine_nodes(...)`.
- `src/backend/mir/aarch64/codegen/codegen.hpp` and
  `module_compile.{hpp,cpp}`: public prepared-module compile entry and
  internal coordinator build the compiled target module through traversal,
  dispatch, and family lowerers.
- `src/backend/mir/aarch64/codegen/asm_emitter.{hpp,cpp}`: owns the current
  compiled-module-to-assembly text helper and walks selected machine functions
  through the shared MIR printer.
- `src/backend/mir/aarch64/codegen/machine_printer.{hpp,cpp}`: owns AArch64
  target spelling for selected machine nodes via
  `MachineInstructionPrinter` and
  `print_machine_instruction_line_payloads(...)`.
- `src/backend/CMakeLists.txt`: lists active AArch64 codegen translation units;
  no AArch64 peephole source is currently compiled.

Legacy markdown material status: `src/backend/mir/aarch64/codegen/peephole.md`
describes an obsolete text-first optimizer over already-rendered assembly
lines. Its local/global rewrite inventory, `LineKind` classifier, tombstone
model, shared string-helper dependency, disabled global-store-forwarding notes,
and old unit-test list are deferred/archive material only. They should not be
revived for Step 2, and `prologue.md`, `variadic.md`, and other markdown
shards remain out of scope.

Concrete Step 2 API shape: add a compiled deferred/no-op owner, not an active
text optimizer. Suggested header:

```cpp
#pragma once

#include <string>

namespace c4c::backend::aarch64::codegen {

[[nodiscard]] std::string apply_deferred_peephole_boundary(std::string assembly);

}  // namespace c4c::backend::aarch64::codegen
```

Suggested implementation: return `assembly` unchanged by move. Add only a
short code comment stating that the boundary exists to own the current
"no live AArch64 peephole pass" decision and that the archived text-first
rewrites are intentionally deferred until a structured machine-node or
explicit printer-output cleanup policy is accepted. Add
`peephole.cpp` to `src/backend/CMakeLists.txt`; do not wire it into
`asm_emitter` in Step 2 unless the supervisor explicitly chooses integration.

## Suggested Next

Execute Step 2 from `plan.md`: create `peephole.hpp` and `peephole.cpp` with
the deferred/no-op API above, add the translation unit to
`src/backend/CMakeLists.txt`, and run the supervisor-selected compile proof.

## Watchouts

Do not reintroduce the legacy text optimizer, RISC-V-style `peephole_optimize`
surface, string classifier, global copy propagation, or dead stack-store
passes. The current packet found no already-live AArch64 cleanup behavior to
move, so Step 2 should preserve emitted assembly and should not make
`asm_emitter`, `machine_printer`, or `backend.cpp` hidden peephole owners.

## Proof

No build required for this audit-only packet. No `test_after.log` update was
made because the delegated proof was explicitly "No build required".
