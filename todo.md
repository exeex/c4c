Status: Active
Source Idea Path: ideas/open/prealloc-responsibility-map-and-layout-plan.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Build The Responsibility Map

# Current Packet

## Just Finished

Completed Step 2 from `plan.md`: classified all 109 `src/backend/prealloc`
`.cpp` / `.hpp` files by primary durable responsibility, target-shape, mixed
ownership boundary, and prepared-printer mirror behavior. No implementation
files were changed.

### Responsibility Map

All file paths in the table are relative to `src/backend/prealloc/`.

Target-shape legend: `neutral` means independent of concrete target profiles;
`parameterized` means valid prealloc ownership using `TargetProfile`, ABI facts,
or stable target register identities; `specific-watch` means architecture names
or final target-shaped details are present and should be watched during layout
cleanup.

| Primary responsibility | Files | Target shape | Mixed boundary notes |
| --- | --- | --- | --- |
| Pipeline coordination and phase ordering | `prealloc.cpp`, `prealloc.hpp`, `legalize.cpp`, `module.hpp`, `names.hpp` | `prealloc.cpp`, `prealloc.hpp`, `module.hpp`, and `names.hpp` are mostly neutral; `legalize.cpp` is parameterized through `TargetProfile` legalization and ABI inference. | `module.hpp` is deliberately mixed as the aggregate prepared-module contract and inline lookup host; do not split it mechanically in Step 3. `prealloc.cpp` is the phase-order coordinator. `legalize.cpp` mutates BIR before later planning and is a semantic phase, not layout-only cleanup. |
| Control-flow normalization, out-of-SSA, and move bundles | `control_flow.hpp`, `out_of_ssa.cpp`, `label_identity.cpp`, `label_identity.hpp` | neutral. | `control_flow.hpp` is the largest mixed contract: route name, branch conditions, join transfers, parallel copies, authoritative join/branch sources, invariants, and label metadata all live together. The mixed shape is durable at the contract level but a future package model should separate control-flow facts from route/invariant diagnostics before any physical file split. |
| Liveness and register-allocation planning | `liveness.cpp`, `liveness.hpp`, `regalloc.cpp`, `regalloc.hpp`, `regalloc/assignment.cpp`, `regalloc/assignment.hpp`, `regalloc/call_moves.cpp`, `regalloc/call_moves.hpp`, `regalloc/call_return_abi.cpp`, `regalloc/call_return_abi.hpp`, `regalloc/classification.cpp`, `regalloc/classification.hpp`, `regalloc/consumer_moves.cpp`, `regalloc/consumer_moves.hpp`, `regalloc/intervals.cpp`, `regalloc/intervals.hpp`, `regalloc/move_records.cpp`, `regalloc/move_records.hpp`, `regalloc/phi_moves.cpp`, `regalloc/phi_moves.hpp`, `regalloc/pointer_carriers.cpp`, `regalloc/pointer_carriers.hpp`, `regalloc/runtime_helpers.cpp`, `regalloc/runtime_helpers.hpp`, `regalloc/spill_reload.cpp`, `regalloc/spill_reload.hpp`, `regalloc/stack_slots.cpp`, `regalloc/stack_slots.hpp`, `regalloc/storage.cpp`, `regalloc/storage.hpp`, `regalloc/value_homes.cpp`, `regalloc/value_homes.hpp`, `regalloc/values.cpp`, `regalloc/values.hpp`, `regalloc_placement_identity.cpp`, `regalloc_placement_identity.hpp` | liveness is neutral; core regalloc is parameterized; `regalloc/call_return_abi.cpp` is `specific-watch` because it contains AArch64/x86/RISC-V ABI branches and concrete register sequences; `regalloc_placement_identity.*` is parameterized target-register identity publication. | `regalloc.cpp` is a durable coordinator over split helper files, stack layout, dynamic stack, target register profile, and value homes. `regalloc.hpp` mixes public allocation state, move resolution, ABI bindings, constraints, and target-register identities. `regalloc/call_moves.cpp` and `regalloc/value_homes.cpp` deliberately bridge call ABI/profile facts into value-home planning. |
| Stack layout and frame planning | `frame.hpp`, `frame_plan.cpp`, `frame_plan.hpp`, `stack_layout/alloca_coalescing.cpp`, `stack_layout/analysis.cpp`, `stack_layout/coordinator.cpp`, `stack_layout/copy_coalescing.cpp`, `stack_layout/inline_asm.cpp`, `stack_layout/regalloc_helpers.cpp`, `stack_layout/slot_assignment.cpp`, `stack_layout/stack_layout.hpp` | mostly parameterized; `frame_plan.cpp` uses target register pools and target-sized save slots; `stack_layout/coordinator.cpp` is `specific-watch` for fallback size/address paths but still owns target-parameterized planning rather than final emission. | `stack_layout/coordinator.cpp` is the main mixed boundary: object collection, slot assignment orchestration, address materialization publication, and fallback address facts. `module.hpp` currently declares stack-layout helper APIs, so Step 3 should decide whether those declarations stay with the aggregate contract or move under stack-layout ownership. |
| Call/return ABI movement plans | `calls.hpp`, `call_plans.cpp`, `call_plans.hpp`, `formal_publications.cpp`, `formal_publications.hpp` | parameterized; call plans consume ABI and target register profile facts but do not emit target instructions. | `calls.hpp` is deliberately mixed: argument/result storage encodings, clobbers, preservation routes, indirect callee plans, memory return plans, call-boundary classification, and boundary effects are one call ABI contract. `call_plans.cpp` spans direct/indirect calls, preservation, clobbers, and boundary-effect derivation; future cleanup should treat it as phase extraction, not arbitrary file movement. |
| Value homes, storage encodings, and publication plans | `addressing.hpp`, `decoded_home_storage.cpp`, `decoded_home_storage.hpp`, `prepared_lookups.cpp`, `prepared_lookups.hpp`, `publication_plans.cpp`, `publication_plans.hpp`, `storage.hpp`, `storage_plans.cpp`, `storage_plans.hpp`, `value_locations.hpp` | mostly parameterized; `storage_plans.cpp` resolves typed register placement from target profile; `decoded_home_storage.*` is target-aware diagnostic decoding; `addressing.hpp` carries target-shaped TLS/address materialization models but not final instruction selection. | `decoded_home_storage.hpp` intentionally crosses prepared lookups, regalloc, storage, and value locations as a reader-facing decode contract. `publication_plans.hpp` mixes scalar and store-source publication because both serve codegen-facing publication. `value_locations.hpp` bridges regalloc homes and move bundles; its bridge comments mark compatibility boundaries to preserve until consumers converge. |
| Runtime helper carriers for i128/f128/atomics/intrinsics/inline asm | `atomics.cpp`, `atomics.hpp`, `f128_runtime_helpers.cpp`, `f128_runtime_helpers.hpp`, `i128_runtime_helpers.cpp`, `i128_runtime_helpers.hpp`, `inline_asm.cpp`, `inline_asm.hpp`, `intrinsics.cpp`, `intrinsics.hpp`, `runtime_helpers.hpp`, `special_carriers.cpp`, `special_carriers.hpp` | parameterized overall; `intrinsics.cpp` is `specific-watch` for AArch64 CRC/NEON feature checks; `inline_asm.cpp` is `specific-watch` for target inline-asm register/home identity; i128/f128 helpers use target register profile facts. | `runtime_helpers.hpp` is a broad carrier contract that mixes helper families, ABI transitions, marshal directions, ownership, and value homes. `special_carriers.hpp` is another broad carrier contract for i128/f128/atomics/intrinsics/inline asm. These mixed headers are durable as data-family aggregates, but Step 3 should decide whether runtime helper contracts and special carrier contracts remain separate public surfaces. |
| Dynamic stack and variadic entry plans | `dynamic_stack.cpp`, `dynamic_stack.hpp`, `variadic.hpp`, `variadic_entry_plans.cpp`, `variadic_entry_plans.hpp` | `dynamic_stack.*` is neutral except LLVM intrinsic names; `variadic*` is `specific-watch` because AAPCS64 HFA/helper resource authority is currently explicit. | `variadic_entry_plans.cpp` mixes AAPCS64 ABI facts, helper scratch requirements, operand-home authority, and storage authority. That is target-parameterized prealloc ownership, but the AAPCS64-specific names are a candidate for Step 3 package boundaries rather than a generic variadic-only file. |
| Target register profile facts | `target_register_profile.cpp`, `target_register_profile.hpp` | `specific-watch` by design: it centralizes x86, AArch64, and RISC-V caller/callee/ABI register facts. | This is the correct home for concrete register pool facts inside prealloc. It should remain isolated from final register spelling/emission and should be reused by call, frame, storage, and regalloc helpers instead of duplicating architecture tables. |
| Prepared printer/debug dump support | `prepared_printer.cpp`, `prepared_printer.hpp`, `prepared_printer/private.hpp`, `prepared_printer/addressing.cpp`, `prepared_printer/atomics.cpp`, `prepared_printer/calls.cpp`, `prepared_printer/control_flow.cpp`, `prepared_printer/frame.cpp`, `prepared_printer/functions.cpp`, `prepared_printer/inline_asm.cpp`, `prepared_printer/intrinsics.cpp`, `prepared_printer/regalloc.cpp`, `prepared_printer/runtime_helpers.cpp`, `prepared_printer/special_carriers.cpp`, `prepared_printer/storage.cpp`, `prepared_printer/value_locations.cpp`, `prepared_printer/variadic.cpp` | neutral debug formatting over prepared data; output includes target/profile fields but does not own target policy. | The directory is already family-split. `private.hpp` deliberately imports `module.hpp`, so each mirror file sees the aggregate module contract. Printer files should follow future data-family moves rather than lead them. |

### Mixed Ownership Boundaries

- Durable aggregate boundaries: `module.hpp`, `calls.hpp`, `runtime_helpers.hpp`,
  `special_carriers.hpp`, `regalloc.hpp`, and `control_flow.hpp` are broad
  because they define cross-family prepared data consumed by codegen and debug
  printing. Step 3 should mark which are aggregate contracts instead of treating
  size alone as a defect.
- Temporary or compatibility boundaries: bridge/legacy comments in
  `control_flow.hpp`, `value_locations.hpp`, and
  `prepared_printer/calls.cpp` look like consumer-transition markers. They are
  not proof of wrong ownership, but they should become named package notes or
  follow-up ideas if Step 3 finds a stable split.
- Phase coordinator boundaries: `prealloc.cpp`, `regalloc.cpp`,
  `stack_layout/coordinator.cpp`, `call_plans.cpp`, `out_of_ssa.cpp`,
  `i128_runtime_helpers.cpp`, `f128_runtime_helpers.cpp`, and
  `variadic_entry_plans.cpp` are large because they perform phase orchestration.
  Extraction should be by durable sub-phase, not by line-count trimming.
- Cross-family decode/publication boundaries: `decoded_home_storage.*`,
  `publication_plans.*`, `prepared_lookups.*`, `storage_plans.cpp`, and
  `formal_publications.*` intentionally translate between planning families and
  codegen-facing views. These are good candidates for a stable
  "publication/accessor" package in Step 3.
- Target-specific watch points: `target_register_profile.cpp`,
  `regalloc/call_return_abi.cpp`, `variadic_entry_plans.cpp`,
  `intrinsics.cpp`, `inline_asm.cpp`, `frame_plan.cpp`, `storage_plans.cpp`,
  and `stack_layout/coordinator.cpp` contain concrete architecture or ABI
  branches. None appear to own final instruction emission, but Step 3 should
  prevent those details from spreading into generic data contracts.

### Target Classification Notes

- Target-neutral: naming tables, control-flow facts, label identity,
  out-of-SSA planning, liveness intervals, prepared lookups, most printer
  formatting, and dynamic-stack intrinsic detection.
- Target-parameterized: frame planning, storage plans, call plans, regalloc
  assignment/value homes, runtime helper facts, decoded home storage, and
  address materialization facts. These use `TargetProfile`, ABI metadata, or
  prepared register identity as inputs while still producing target-independent
  prepared contracts for codegen.
- Accidentally target-specific candidates: no file clearly moves final target
  instruction emission or register spelling into prealloc. The closest watch
  surfaces are architecture-named ABI helpers in `regalloc/call_return_abi.cpp`
  and `variadic_entry_plans.cpp`, AArch64 feature validation in
  `intrinsics.cpp`, inline-asm home identity in `inline_asm.cpp`, and fallback
  address/size paths in `stack_layout/coordinator.cpp`. These should be
  contained, not erased.

### Prepared-Printer Mirror Notes

- Clean mirrors: `prepared_printer/addressing.cpp`,
  `prepared_printer/atomics.cpp`, `prepared_printer/control_flow.cpp`,
  `prepared_printer/frame.cpp`, `prepared_printer/inline_asm.cpp`,
  `prepared_printer/intrinsics.cpp`, `prepared_printer/regalloc.cpp`,
  `prepared_printer/storage.cpp`, `prepared_printer/value_locations.cpp`, and
  `prepared_printer/variadic.cpp` each mirror a data family directly.
- Aggregate mirrors: `prepared_printer.cpp`, `prepared_printer.hpp`, and
  `prepared_printer/private.hpp` own dump entry point and shared print helpers;
  `prepared_printer/functions.cpp` prints top-level function/module structure;
  `prepared_printer/runtime_helpers.cpp` and
  `prepared_printer/special_carriers.cpp` mirror broad carrier contracts.
- Follow future moves: if Step 3 splits call boundary effects, runtime helper
  families, or publication/accessor packages, the matching printer functions
  should follow those data-family boundaries after the data contracts move.

## Suggested Next

Execute Step 3 from `plan.md`: propose the stable package model. Start with
durable aggregate contracts (`module.hpp`, `control_flow.hpp`, `regalloc.hpp`,
`calls.hpp`, `runtime_helpers.hpp`, `special_carriers.hpp`), then group
implementation files into phase coordinators, helper packages,
publication/accessor packages, target-profile facts, and prepared-printer
mirrors. Produce a prioritized cleanup candidate table with slice type and
reviewer reject signals.

## Watchouts

- Do not turn the Step 3 package model into implementation movement; this
  remains an audit/planning runbook until follow-up ideas are created.
- Preserve `target_register_profile.*` as the central target fact owner unless
  Step 3 finds duplicated architecture tables elsewhere.
- Keep final target instruction selection and assembly emission outside
  prealloc. Target-parameterized ABI planning is acceptable; target codegen is
  not.
- Treat printer files as mirrors. They should follow data-family movement and
  should not be used to justify splitting data contracts first.

## Proof

Delegated proof: `git diff --check`. This audit-only packet has no build or
test subset. Result: passed.
