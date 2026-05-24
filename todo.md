Status: Active
Source Idea Path: ideas/open/prealloc-stack-layout-contract-boundary.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Stack And Frame Boundaries

# Current Packet

## Just Finished

Step 1 - Inventory Stack And Frame Boundaries completed as a no-code audit.

Candidate boundary map:
- `frame.hpp` owns the durable data contracts: `PreparedStackObject`,
  `PreparedFrameSlot`, `PreparedStackLayout`, register placement enums,
  saved-register slot placement, `PreparedFramePlan`,
  `PreparedDynamicStackPlan`, and stack/frame lookup helpers such as
  `find_prepared_frame_slot(...)` and
  `find_prepared_stack_frame_offset_by_name(...)`.
- `frame_plan.hpp` is not a frame data contract yet; it only declares
  `populate_frame_plan(PreparedBirModule&)` and includes `module.hpp`, so
  moving frame data declarations there would be contract churn rather than a
  cleanup.
- `stack_layout/stack_layout.hpp` currently owns only stack-layout utility
  functions (`fallback_type_size`, `normalize_size`,
  `normalize_alignment`, `align_to`). It does not declare the stack-layout
  phase entry points.
- `module.hpp` remains the aggregate public surface for
  `PreparedBirModule`, `BirPreAlloc`, stack-layout phase declarations, and
  prepared-plan find helpers. Its `namespace stack_layout` declarations are
  implementation phase hooks used by `stack_layout/coordinator.cpp` and
  defined by the sibling stack-layout implementation files.

Direct consumer map:
- The stack-layout phase declarations in `module.hpp`
  (`collect_function_stack_objects`, `apply_*_hints`,
  `summarize_inline_asm`, `assign_frame_slots`) are referenced only from
  `stack_layout/coordinator.cpp`; their definitions live in
  `analysis.cpp`, `alloca_coalescing.cpp`, `copy_coalescing.cpp`,
  `inline_asm.cpp`, `regalloc_helpers.cpp`, and `slot_assignment.cpp`.
- Direct prepared stack/frame data consumers are broad and legitimate:
  prealloc regalloc, liveness, call plans, variadic plans, storage/runtime
  helper plans, publication plans, prepared printer, backend filtering, and
  AArch64/x86 final codegen all read `PreparedStackLayout`,
  `PreparedFrameSlot`, `PreparedStackObject`, frame-plan, or dynamic-stack
  facts.
- Several public headers include `module.hpp` because they need the aggregate
  `PreparedBirModule` view, not just stack/frame types
  (`prepared_printer.hpp`, `prealloc.hpp`, `call_plans.hpp`,
  `dynamic_stack.hpp`, runtime/helper plan headers). This keeps immediate
  header moves risky unless scoped to the private stack-layout phase hooks.

Coordinator phase map:
- Object collection and object annotation are already split out by file:
  `analysis.cpp` collects local/parameter objects and aggregate address
  publication hints; alloca and copy coalescing each have their own helper
  file; `regalloc_helpers.cpp` applies regalloc home-slot pressure; and
  `slot_assignment.cpp` assigns frame slots.
- `run_stack_layout()` still orchestrates per-function state: reset prepared
  stack/addressing, collect and annotate function objects, assign function
  slots, publish global frame size/alignment, create per-function addressing,
  build a frame-slot name map, append direct memory accesses, append address
  materializations, move objects/slots into the aggregate layout, and emit
  notes.
- The large remaining coordinator responsibility is address
  materialization/publication, including direct frame-slot accesses, symbol
  backed accesses, pointer-indirect accesses, direct global/string/label
  address materializations, and missing-fact notes. This is conceptually
  stack/frame publication plus addressing, not object collection or slot
  assignment.
- Glue that should stay in coordinator for now: phase ordering, next object
  and slot ID ownership, aggregate frame-size/alignment reduction, and final
  insertion into `prepared_.stack_layout` and `prepared_.addressing`.

No-code decisions:
- Do not move `PreparedStackObject`, `PreparedFrameSlot`, or
  `PreparedStackLayout` out of `frame.hpp`; they are already the shared
  frame/stack data contract consumed by many unrelated families.
- Do not move `populate_frame_plan(...)` or frame-plan structs in Step 2;
  `frame_plan.hpp` is currently an entry-point header and `frame.hpp` is the
  stable data header.
- Do not create a forwarding header. Existing consumers either need
  `PreparedBirModule` or the real `frame.hpp` contract.
- Do not split `coordinator.cpp` before the phase hook contract is clarified;
  a file split would otherwise preserve the same public leakage through
  `module.hpp`.

Public-contract pressure:
- The clearest contract pressure is that private-ish stack-layout phase hooks
  are declared in aggregate `module.hpp` even though only
  `coordinator.cpp` calls them. Step 2 can move those declarations to
  `stack_layout/stack_layout.hpp` because every defining stack-layout
  implementation file already includes that header and `coordinator.cpp`
  already includes both `../module.hpp` and `stack_layout.hpp`.
- `FunctionInlineAsmSummary` travels with those phase hooks and can move with
  them if Step 2 keeps it under `namespace stack_layout`.
- Moving broader prepared data types is not justified by this audit.

Prepared-printer naming notes:
- `prepared_printer/frame.cpp` prints three stable sections:
  `--- prepared-stack-layout ---`, `--- prepared-frame-plan ---`, and
  `--- prepared-dynamic-stack-plan ---`.
- Stack layout output mirrors `PreparedStackObject` and `PreparedFrameSlot`
  field names: object id, function, prepared object name, source kind, type,
  size/alignment, address exposure, home-slot flags, slot id, object id,
  offset, and fixed location.
- Frame-plan output mirrors frame-plan and saved-register placement fields:
  frame size/alignment, dynamic stack flag, fixed-slots-use-fp,
  saved-register placement, slot placement, and `frame_slot_order`.
- No printer changes are needed until a public data-family or dump field name
  actually changes.

## Suggested Next

Step 2 - Clarify Header Contract Ownership: move only the stack-layout phase
hook declarations and `FunctionInlineAsmSummary` from `module.hpp` to
`stack_layout/stack_layout.hpp`, keeping `PreparedBirModule`, `BirPreAlloc`,
frame data types, and frame-plan contracts unchanged. Touch only
`module.hpp`, `stack_layout/stack_layout.hpp`, and any minimal includes needed
to compile.

## Watchouts

- Keep Step 2 scoped to the private stack-layout phase hook surface; broad
  prepared data consumers make moving frame/stack structs a separate, higher
  risk contract redesign.
- Do not create forwarding headers or broad include churn.
- Do not change stack layout, frame sizing, slot assignment, alignment,
  dynamic address semantics, or prepared frame dump meaning.
- If Step 2 exposes compile pressure from implementation files that relied on
  `module.hpp` transitively, prefer the smallest direct include rather than a
  larger header reshuffle.

## Proof

Ran `git diff --check`; passed. No `test_after.log` was produced because this
was a no-code inventory packet.
