Status: Active
Source Idea Path: ideas/open/258_aarch64_emit_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Delete Reconciled Emit Markdown Shard

# Current Packet

## Just Finished

Step 4: Delete Reconciled Emit Markdown Shard completed the reconciliation
ledger for `src/backend/mir/aarch64/codegen/emit.md` and deleted the shard.

Durable content covered by compiled owners:

- Public prepared-module entry responsibility is covered by
  `codegen/emit.hpp`, `codegen/emit.cpp`, `module/module.hpp`, and
  `module/module.cpp`: `build_module(const PreparedBirModule&)` is the thin
  accepted handoff and `module::build(...)` delegates to it.
- Target-profile normalization and prepared-module handoff rejection are
  covered by `abi::resolve_target_profile(...)` and
  `abi::validate_prepared_module_handoff(...)`; these are invoked by
  `codegen::build_module(...)` before lowering.
- Prepared function/block traversal is covered by `codegen/traversal.*`:
  `lower_prepared_functions(...)` constructs machine functions/blocks from
  prepared control-flow facts and inserts simple fixed-frame nodes from
  prepared frame facts.
- Prepared instruction and terminator dispatch is covered by
  `codegen/dispatch.*` and the family-specific codegen shards
  (`alu.*`, `calls.*`, `comparison.*`, `globals.*`, `memory.*`,
  `returns.*`): dispatch classifies BIR/prepared-operation families, appends
  selected machine instructions, and records structured diagnostics for
  unsupported or missing prepared facts.
- Structured record ownership is covered by `codegen/instruction.*` and
  `module/module.hpp`: target MIR records, machine instruction nodes, operand
  roles, instruction families, selection status, effects, and compatibility
  projections are typed records rather than raw assembly text.
- Assembly spelling for selected target records is covered by
  `codegen/machine_printer.*`; the compatibility projection comments keep
  terminal assembly printing routed through the MIR/printer path, not the
  legacy flat record projection.
- The accepted rebuild direction from the markdown shard is covered by the
  current compiled route: prepared-module target records feed MIR/module
  records, while direct text emission and parsed text are not restored as the
  semantic backend path.

Stale or out-of-scope markdown content:

- The old `emit.hpp` API inventory (`emit_direct_bir_module`,
  `emit_module(...)`, `emit_prepared_lir_module`, `assemble_module(...)`) is
  stale for this MIR AArch64 shard; the compiled owner exposes only the
  prepared-module `build_module(...)` route.
- Historical BIR/LIR direct shape recognizers, minimal assembly helpers,
  general string-driven LIR rendering, direct dispatch-order fallback lists,
  constant-folding fallbacks, and old unsupported direct-emitter error strings
  describe removed transition-era direct assembly behavior. Reintroducing them
  here would violate the current ownership direction and testcase-overfit
  guardrails.
- Notes about local parsing of LIR textual type strings, old stack/regalloc
  helper dependencies, ad-hoc global/string rendering, Darwin/GNU text
  directives, and object-assembly handoff are stale for the compiled
  prepared-record path or belong to narrower future ABI/data/printer/assembler
  work, not `emit.cpp`.
- The future rebuild pressure notes are preserved as classification rather than
  markdown: new work should extend typed prepared facts, machine records, and
  family-specific codegen/printer owners, not recreate a monolithic direct text
  emitter.

## Suggested Next

Next coherent packet: hand back to the supervisor for lifecycle/closure
decision on the source idea now that the final markdown shard has been
reconciled and removed.

## Watchouts

- No implementation or test behavior changed in this packet.
- `emit.cpp` remains thin and continues to coordinate only the prepared-module
  build flow.
- `emit.md` had no durable content left that was both current and uncovered by
  compiled owners/current contracts.

## Proof

Passed. Supervisor-selected proof command:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_aarch64_(instruction_dispatch|module_skeleton_contract|function_traversal|machine_printer|target_record_core_contract)$' > test_after.log 2>&1
```

`test_after.log` contains the canonical proof output. The selected subset ran
5 tests and all passed after this packet.
