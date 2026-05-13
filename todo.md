Status: Active
Source Idea Path: ideas/open/216_aarch64_machine_node_asm_printer_external_smoke.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inspect Printer Inputs And CLI Route Shape

# Current Packet

## Just Finished

Step 1 inspected the accepted idea 215 machine-node surfaces, current `c4cll`
route shape, focused test locations, and local external AArch64 toolchain
capability before implementation edits.

Accepted idea 215 subset:
- Record-level selected machine-node constructors cover branch nodes
  (`branch`, `conditional_branch`, `compare_branch`), integer scalar ALU
  (`add`, `sub`, `and`, `or`, `xor`), simple integer casts (`sign_extend`,
  `zero_extend`, `truncate`), memory load/store nodes for prepared frame-slot
  or pointer-value bases, and prepared spill/reload pseudo nodes
  (`spill_to_slot`, `reload_from_slot`).
- The AArch64 module build currently populates
  `FunctionRecord::machine_nodes` only through
  `build_spill_reload_machine_nodes(...)`, so the public source-to-machine-node
  route can observe selected spill/reload nodes today; scalar, branch, and
  memory constructor tests exist, but the module pipeline does not yet append
  those node families into `machine_nodes`.
- Unsupported/unsafe boundaries are explicit: non-`MachineInstructionNode`
  surfaces, non-`Selected` statuses, assembler external-input records, object
  encoder-input records, global/string/register/none memory bases, rematerialize
  spill/reload pseudo ops, missing slot/scratch/identity facts, and call/return
  placeholder families should fail closed in the first printer.

Planned public route shape:
- Closest existing route references are `--dump-bir`, `--dump-prepared-bir`,
  `--dump-mir`, and `--trace-mir` in `src/apps/c4cll.cpp`; these start from
  source, run preprocessing/lex/parse/sema/HIR, lower HIR to LIR, then call
  `backend::dump_module(...)`.
- The closest emission-boundary shape is existing `--codegen asm -o out.s`,
  which already reserves backend-native assembly output and rejects LLVM/no-asm
  fallback. For AArch64, the new route should use
  `c4cll --codegen asm --target aarch64-linux-gnu input.c -o out.s` or a
  narrowly named equivalent if the supervisor chooses not to overload current
  `--codegen asm` behavior.
- The route target should be frontend `.c` -> HIR -> LIR -> backend semantic
  BIR -> prepared BIR -> `aarch64::module::build(...)` -> selected
  `FunctionRecord::machine_nodes` -> machine-node `.s` printer. It must not use
  rendered BIR, target-MIR display strings, hand-authored fixture assembly, or
  an assembly parser as semantic input.

Test placement:
- Focused printer-format and fail-closed unit coverage fits under
  `tests/backend/`, next to `backend_aarch64_target_instruction_records`,
  `backend_aarch64_memory_operand_records`,
  `backend_aarch64_scalar_alu_records`,
  `backend_aarch64_scalar_cast_records`,
  `backend_aarch64_branch_compare_records`, and
  `backend_aarch64_prepared_frame_control`.
- Public route smoke coverage fits in `tests/backend/CMakeLists.txt` using the
  existing backend CLI route/dump CMake helper pattern, with a new helper if it
  needs to emit `.s`, invoke external `clang` / `as`, and optionally run the
  result. Small C inputs can live under `tests/backend/case/`.

Local toolchain capability:
- `clang --version` reports Debian clang 19.1.7, target
  `aarch64-unknown-linux-gnu`.
- Host execution is native AArch64: `uname -m` reports `aarch64`; no
  `qemu-aarch64`, `qemu-aarch64-static`, or `qemu-system-aarch64` was found.
- `/usr/bin/clang`, `/usr/bin/as`, `/usr/bin/aarch64-linux-gnu-as`,
  `/usr/bin/readelf`, `/usr/bin/llvm-readelf`, `/usr/bin/objdump`, and
  `/usr/bin/llvm-objdump` are available; `/usr/bin/file` is not installed.
- `clang --target=aarch64-linux-gnu -c` on a tiny assembly file succeeds and
  `readelf -h` reports an ELF64 AArch64 relocatable.
- `aarch64-linux-gnu-as` on the same tiny assembly file succeeds and
  `readelf -h` reports an ELF64 AArch64 relocatable.
- `clang --target=aarch64-linux-gnu` can compile and link a tiny C program to
  an ELF64 AArch64 PIE, and the executable runs successfully on this host.

## Suggested Next

Implement Step 2 as a minimal printer over selected structured machine nodes,
starting with the currently pipeline-populated spill/reload pseudo nodes and
adding record-level selected scalar/branch/memory print support only where the
node payload has enough structured operands to emit GNU/LLVM-compatible AArch64
without guessing.

## Watchouts

- The `.s` printer is output-only; do not parse printed assembly back into
  backend semantics.
- Do not bypass BIR/prepared BIR, AArch64 target MIR, or machine-node
  selection when serving the public CLI route.
- Do not implement an internal assembler, encoder, object writer, linker, or
  `.s` parser in this plan.
- Keep smoke proof rooted in the public `c4cll` route and the external
  toolchain when available.
- Distinguish record-level constructor coverage from the source-to-module
  pipeline: `aarch64::module::build(...)` currently appends only spill/reload
  machine nodes.
- Fail closed for selected node families whose structured operands do not yet
  determine exact assembly spelling, width, label, or address form.

## Proof

No build required by the delegated packet. Inspection/probe commands run:
- `git status --short`
- `sed -n '1,260p' plan.md`
- `sed -n '1,260p' todo.md`
- `sed -n '1,220p' ideas/open/216_aarch64_machine_node_asm_printer_external_smoke.md`
- `sed -n '1,240p' ideas/closed/215_aarch64_first_machine_node_selection_slice.md`
- `sed -n '1,260p' src/backend/mir/aarch64/MACHINE_INSTRUCTION_NODE_CONTRACT.md`
- `sed -n '1,620p' src/backend/mir/aarch64/codegen/records.hpp`
- `rg -n 'make_.*instruction|MachineInstructionNode|MachineOpcode|MachineNodeSelectionStatus|select.*machine|machine.*select|select_.*instruction|select_.*node' src/backend/mir/aarch64/codegen src/backend/mir/aarch64/module tests/backend --glob '*.cpp' --glob '*.hpp'`
- `sed -n '240,860p' src/apps/c4cll.cpp`
- `rg -n 'backend_cli_dump|dump-prepared-bir|dump-bir|dump-mir|trace-mir|backend_codegen_route' tests/backend/CMakeLists.txt tests/c/internal/CMakeLists.txt`
- `ctest --test-dir build -N | rg -n 'backend_|c4cll|hir|lir|bir|mir|aarch64|machine|printer|dump'`
- `clang --version`
- `which clang || true; which as || true; which aarch64-linux-gnu-as || true; which qemu-aarch64 || true; which qemu-aarch64-static || true; which qemu-system-aarch64 || true; uname -m`
- `clang --target=aarch64-linux-gnu -c` on a tiny `/tmp` assembly file, then
  `readelf -h`
- `aarch64-linux-gnu-as` on a tiny `/tmp` assembly file, then `readelf -h`
- `clang --target=aarch64-linux-gnu` compile/link/run of a tiny `/tmp` C
  program
