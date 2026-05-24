Status: Active
Source Idea Path: ideas/open/subsystem-entropy-reduction-refactor-generator.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Build The Entropy Map

# Current Packet

## Just Finished

Step 2 built an evidence-backed entropy map for
`src/backend/mir/aarch64/codegen/` from the Step 1 baseline plus targeted
inspection of the named route, record, compatibility, call, fallback, and
printer files.

Entropy map:
- Data contract hub: `instruction.hpp` / `instruction.cpp`. Evidence:
  `instruction.hpp` defines the core record vocabulary and payload variants
  (`OperandKind`, `RecordSurfaceKind`, `InstructionFamily`, `MachineOpcode`,
  `MachinePrinterMnemonicKind`, `MachineNodeSelectionStatus`, many
  `Prepared*RecordError` enums, `RegisterOperand`, `MemoryOperand`,
  `CallBoundaryMoveInstructionRecord`, `AssemblerInstructionRecord`,
  `ObjectInstructionRecord`, and `InstructionRecord`). `instruction.cpp`
  mixes enum spelling, printer mnemonic mapping, effect derivation, selection
  status, and record constructors such as `make_scalar_instruction`,
  `make_frame_instruction`, `make_assembler_instruction`, and
  `make_unsupported_machine_instruction`. This is justified as a contract hub,
  but high entropy because it also owns policy-like selection/effect logic.
- Phase logic hub: `dispatch.cpp`. Evidence: 43 includes in the Step 1
  baseline and local helpers around BIR/prepared lookup, same-block producer
  queries, select/materialization paths, store retargeting, edge producer
  context, branch-fusion hooks, and the exported `dispatch_prepared_block`.
  It calls family lowerers while also owning cross-family publication,
  retargeting, and materialization decisions, so this is the broadest route
  hub and the highest-risk extraction target.
- Phase-plus-emission hotspot: `alu.cpp` / `alu.hpp`. Evidence:
  `alu.hpp` has 28 includers and exposes state (`BlockScalarLoweringState`),
  record builders, scalar operand conversion, emitted-register tracking, and
  `lower_scalar_instruction`. `alu.cpp` combines scalar record construction,
  prepared home lookup, fallback operand selection (`make_scalar_fallback_operand`),
  control publication materialization, emitted-register cache maintenance, and
  printer-facing helpers (`make_scalar_alu_print_lines`). This is a justified
  combination today, but its fallback/control-publication helpers are concrete
  extraction candidates.
- Call-boundary phase hub: `calls_moves.cpp` plus broad `calls.hpp`. Evidence:
  `calls_moves.cpp` lowers before-call, after-call, before-return, value moves,
  stack copies, byval lanes, immediate bindings, and callee-saved
  republication via helpers such as `lower_before_call_move`,
  `lower_after_call_move`, `lower_before_call_moves`, `lower_after_call_moves`,
  and `lower_value_moves`. `calls.hpp` exposes ABI sizing, prepared-call-plan
  lookup, call-boundary lowering, preservation analysis, byval helpers, and
  call printing declarations. This is broad phase logic with some adapter and
  emission leakage through inline asm payload construction.
- Adapter bridge hotspot: `calls_dispatch_bridge.cpp` /
  `calls_dispatch_bridge.hpp`. Evidence: includes dispatch and call/move
  surfaces, exports bridge entry points such as `lower_call_instruction`,
  `lower_select_chain_call_arguments`, `record_call_result_source_register`,
  and materialization helpers, and contains recursive scalar call-argument
  materialization plus preserved-value/local-load fallback paths. This is a
  good Step 3 candidate because it is route-specific glue rather than a core
  data contract.
- Compatibility bridge: `compatibility_projection.cpp` /
  `compatibility_projection.hpp` and `module_compile.cpp`. Evidence:
  `module_compile.cpp` derives compatibility records after
  `lower_prepared_functions`, while `compatibility_projection.cpp` filters
  selected target records through `selected_compatibility_nodes` and exports
  `derive_compatibility_function_records` /
  `derive_compatibility_projection`. This is intentionally narrow adapter
  logic and a low-risk follow-up candidate, not the first high-value entropy
  reduction.
- Emission/debug/print boundary: `machine_printer.cpp`,
  `machine_printer.hpp`, `asm_emitter.cpp`, `asm_emitter.hpp`, and family
  print helpers such as `calls_printing.cpp`, `f128.cpp`, `intrinsics.cpp`,
  and `variadic.cpp`. Evidence: `machine_printer.cpp` owns
  `MachineInstructionPrinter::print_instruction` and dispatches through
  `print_machine_instruction_line_payloads` to `print_branch`, `print_memory`,
  `print_frame`, `print_call`, `print_scalar`, `print_assembler`, etc.;
  `asm_emitter.cpp` owns `print_prepared_machine_nodes` and global/string
  object text emission. These are large but lower-risk when kept as printer
  consumers of structured nodes.
- Text-emission leakage watchlist: several phase files still build
  `AssemblerInstructionRecord` / `inline_asm_template` payloads directly,
  including `dispatch.cpp`, `alu.cpp`, `calls_moves.cpp`,
  `comparison_branch_fusion.cpp`, `memory_store_sources.cpp`,
  `memory_dynamic_stack.cpp`, `prologue_entry_formals.cpp`, and `cast_ops.cpp`.
  This is not one file's fault; it marks a cross-cutting adapter/emission
  boundary to avoid widening Step 3 beyond one coherent extraction.

## Suggested Next

For Step 3, prefer a bounded extraction around `calls_dispatch_bridge.*` and
its call-argument/materialized-address bridge helpers, with explicit
non-goals for `dispatch.cpp`, `instruction.hpp`, and broad `calls_moves.cpp`
rewrites. The next-best low-risk candidate is documenting or tightening the
`compatibility_projection.*` adapter boundary. Avoid picking `dispatch.cpp` or
`instruction.hpp` first unless Step 3 is explicitly a design-only split,
because both are broad hubs whose first implementation slice would likely
touch too many families.

## Watchouts

`dispatch.cpp`, `alu.cpp`, `calls_moves.cpp`, and `instruction.hpp` are real
hotspots, but they are not equally good first refactor targets. Treat
`instruction.hpp` as a durable contract until a smaller adapter split proves
where the contract is leaking. Treat printer/debug files as follow-up cleanup
unless a concrete extraction needs to move text-emission leakage out of phase
logic.

## Proof

`git diff --check` passed. No build, test run, or `test_after.log` was needed
for this planning-only packet.
