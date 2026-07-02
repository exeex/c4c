Status: Active
Source Idea Path: ideas/open/520_bir_render_owner_preservation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Render Helper Consumers

# Current Packet

## Just Finished

Step 1 - Audit Render Helper Consumers is complete for idea 520.

Clang-tools availability:

- `command -v c4c-clang-tool` -> `/home/vscode/.local/bin/c4c-clang-tool`
- `command -v c4c-clang-tool-ccdb` -> `/home/vscode/.local/bin/c4c-clang-tool-ccdb`

Commands run and relevant results:

- `c4c-clang-tool-ccdb list-symbols /workspaces/c4c/src/backend/bir/bir.cpp build/compile_commands.json`
  succeeded; the target helper definitions are in `bir.cpp`.
- `c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/bir/bir.cpp build/compile_commands.json`
  succeeded; `render_type(TypeKind)`, `render_binary_opcode(BinaryOpcode)`,
  and `render_cast_opcode(CastOpcode)` are non-inline public definitions.
- `c4c-clang-tool-ccdb list-symbols /workspaces/c4c/src/backend/bir/bir_printer.cpp build/compile_commands.json`
  and `function-signatures` for `bir_printer.cpp` succeeded; printer helpers
  are private except public `print(Module)`.
- `c4c-clang-tool function-signatures src/backend/bir/bir.hpp -- --std=c++17 -I/workspaces/c4c/src -I/workspaces/c4c/src/codegen/lir -I/workspaces/c4c/src/frontend/parser`
  succeeded; public declarations are `bir.hpp:6068-6070`.
- `c4c-clang-tool-ccdb find-definition` / `find-declaration` on
  `render_type`, `render_binary_opcode`, and `render_cast_opcode` succeeded:
  definitions are `bir.cpp:2589`, `bir.cpp:2623`, and `bir.cpp:2675`;
  declarations are `bir.hpp:6068-6070`.
- `c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/backend/bir/bir.cpp <helper> build/compile_commands.json`
  reported no in-TU callers for all three helpers.
- `c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/bir/bir.cpp <helper> build/compile_commands.json`
  reported the helpers have no repo-local callees in `bir.cpp`.
- `c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/backend/bir/bir_printer.cpp render_type build/compile_commands.json`
  found printer callers: `render_call_type_name`, `render_phi_observation`,
  and `render_function`.
- `c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/backend/bir/bir_printer.cpp render_binary_opcode build/compile_commands.json`
  and the same query for `render_cast_opcode` reported no target callers, but
  `c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/bir/bir_printer.cpp render_function build/compile_commands.json`
  confirmed `render_function` calls `render_type`; targeted source inspection
  confirms `render_function` also calls both opcode renderers.
- Representative cross-TU caller queries found non-printer users:
  `render_type` in `src/backend/bir/lir_to_bir/memory/local_slots.cpp`,
  `src/backend/prealloc/legalize.cpp`,
  `src/backend/prealloc/prepared_printer/frame.cpp`,
  `src/backend/prealloc/prepared_printer/control_flow.cpp`,
  `src/backend/mir/x86/debug/debug.cpp`, and
  `src/backend/mir/riscv/codegen/object_emission.cpp`;
  `render_binary_opcode` in
  `src/backend/prealloc/prepared_printer/control_flow.cpp` and
  `src/backend/prealloc/prepared_printer/runtime_helpers.cpp`;
  `render_cast_opcode` in
  `src/backend/prealloc/prepared_printer/runtime_helpers.cpp`.
- `c4c-clang-tool-ccdb function-callees` on
  `append_prepared_control_flow`, `append_f128_runtime_helpers`, and
  `rv64_prepared_move_bundle_classification_failure_diagnostic` confirmed
  those functions call the public BIR render helpers through `bir.hpp`.
- Targeted `rg` plus minimal raw inspection was used where the TU-scoped caller
  query missed calls inside lambdas/member functions. Current source-hit map
  outside `bir.cpp`/`bir.hpp`: `render_type` appears in BIR printer, BIR
  lowering memory/calling/module files, prealloc, MIR x86 debug, and MIR RISC-V
  object emission; `render_binary_opcode` appears in BIR printer, prealloc
  prepared printer, and unrelated LIR render helpers; `render_cast_opcode`
  appears in BIR printer and prealloc prepared printer.

Consumer map:

- `render_type`: mixed public model user. Printer uses it heavily, but so do
  BIR lowering diagnostics/state text, prealloc legalize/prepared-printer
  diagnostics, x86 debug, and RISC-V object emission diagnostics.
- `render_binary_opcode`: mixed public render user. Printer uses it for BIR
  instruction text, and prealloc prepared-printer control-flow/runtime-helper
  diagnostics use the BIR helper. LIR files also contain their own same-named
  LIR helper and are not BIR consumers.
- `render_cast_opcode`: mixed public render user. Printer uses it for BIR cast
  instruction text, and prealloc prepared-printer runtime-helper diagnostics
  use the BIR helper.

Disposition:

- Moving these bodies into `bir_printer.cpp` is unsafe: the helpers are public
  APIs with non-printer consumers, so printer ownership would make unrelated
  backend diagnostics/object-emission code link through a printer-owned TU.
- If Step 2 chooses implementation movement, use a narrow public render TU
  rather than `bir_printer.cpp`. Preserving the bodies in `bir.cpp` remains a
  behavior-preserving fallback if adding a render TU is not worth the build
  metadata churn.

## Suggested Next

Delegate Step 2 - Choose Preserve, Printer Move, Or Narrow Render TU. Based on
the audit, choose between preserving bodies in `bir.cpp` or moving only these
public render helper definitions into a new narrow render translation unit.

## Watchouts

Do not move these helpers into `bir_printer.cpp` without an explicit route
change: the consumer map is mixed, not printer-only. Any implementation move
should preserve `bir.hpp` declarations and avoid route declarations, route
records, validation behavior, printer output spelling, tests, expectations, and
build behavior unrelated to a possible narrow render TU.

## Proof

Audit-only packet. No build or ctest proof was required or run. Relevant
clang-tools commands and results are recorded above; no `test_after.log` was
created for this no-code audit.
