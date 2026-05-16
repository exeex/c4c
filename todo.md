Status: Active
Source Idea Path: ideas/open/258_aarch64_emit_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Emit Shard Content

# Current Packet

## Just Finished

Step 1: Inventory Emit Shard Content completed for
`src/backend/mir/aarch64/codegen/emit.md` without implementation, test, plan,
idea, or log changes.

`emit.md` classification against current compiled owners:

- Compiled-owned: the accepted prepared-module entry path exists as
  `api::build_prepared_module(...) -> module::build(...) ->
  codegen::build_module(...)`; current `emit.cpp` owns the thin
  `build_module(const PreparedBirModule&)` coordinator.
- Compiled-owned: target-profile resolution and handoff validation are owned by
  `abi::{resolve_target_profile, validate_prepared_module_handoff}` and called
  from `emit.cpp`; this is valid top-level emit/build orchestration, but the
  ABI-specific policy itself belongs in `abi`.
- Compiled-owned: prepared-function traversal is owned by
  `traversal.cpp` through `lower_prepared_functions(...)` and
  `make_function_lowering_context(...)`; `emit.cpp` correctly delegates rather
  than owning function/block traversal.
- Compiled-owned: instruction family routing and selected-node construction are
  owned by `dispatch.cpp` and narrower shard helpers; `dispatch.cpp` currently
  routes scalar, select, call, memory, intrinsic, inline-asm, branch/control,
  frame, and special i128/f128 cases into instruction-record owners.
- Compiled-owned: family-neutral instruction record names, status records,
  operand/effect metadata, and record constructors are owned by
  `instruction.cpp`/`instruction.hpp`.
- Compiled-owned: terminal instruction spelling is owned by
  `machine_printer.cpp`/`machine_printer.hpp` through
  `MachineInstructionPrinter` and
  `print_machine_instruction_line_payloads(...)`; this is the right owner for
  print bodies, not `emit.cpp`.
- Compiled-owned: compatibility projection from MIR functions into legacy flat
  function records is owned by `compatibility_projection.cpp` and called from
  `emit.cpp`.
- Stale: legacy `emit.hpp` APIs in the shard
  (`emit_direct_bir_module`, `emit_module` overloads,
  `emit_prepared_lir_module`, and `assemble_module`) are not current AArch64
  prepared-module API surface and should not be restored by this idea.
- Stale: direct BIR/LIR shape recognizers, constant-fold fallback, dispatch
  ordering among old text-rendering fallbacks, and rejection strings describe
  the removed transition-era emitter rather than current compiled ownership.
- Stale: raw `emit_minimal_*_asm` helpers, manual function preludes, manual
  stack adjustment, Darwin symbol spelling, global/string data text emission,
  and the broad string-driven general LIR renderer should not move into
  `emit.cpp`.
- Out of scope: future rebuild notes for ABI, frame, data, call, memory,
  scalar, control-flow, module-data, object assembly, and semantic backend
  expansion are valid pressure notes but not implementation work for this
  source-ownership redistribution packet.
- Gap: `emit.cpp` is already thin, but it still directly sequences target
  resolution, handoff validation, MIR lowering, compatibility record
  derivation, compatibility projection, and `Module` assembly inline. The next
  narrow packet should make that orchestration more explicit in
  `emit.cpp`/`emit.hpp` without moving instruction, printer, dispatch, or ABI
  policy bodies into emit.

## Suggested Next

First narrow implementation packet: Step 2 should only tighten the
`emit.cpp`/`emit.hpp` prepared-module build orchestration surface. Keep the
public compiled surface as `build_module(const PreparedBirModule&)`, preserve
the existing call order and behavior, and factor only thin emit-owned assembly
of `module::Module`/`BuildResult` if it makes the shard boundary clearer.
Do not move ABI policy, traversal, dispatch, instruction records, or printer
spelling into `emit.cpp`.

## Watchouts

- Preserve behavior; this idea is source ownership redistribution.
- Do not use testcase-shaped shortcuts, expectation rewrites, or unsupported
  downgrades as progress.
- Keep `emit.cpp` thin and avoid turning it into a catch-all for instruction
  spelling, selected-node families, or backend semantics.
- Do not delete `emit.md` until its durable valid content is reconciled or
  classified as stale/out of scope for close.
- The `emit.md` direct-text emitter inventory is mostly stale; only the
  prepared-module entry shim, target handoff, module build coordination, and
  delegation boundaries are relevant to the active route.

## Proof

Not run; inventory-only packet per supervisor instruction. No `test_after.log`
was written.

Supervisor-selected proof command recorded for the first code-moving packet:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_aarch64_(instruction_dispatch|module_skeleton_contract|function_traversal|machine_printer|target_record_core_contract)$'
```
