Status: Active
Source Idea Path: ideas/open/258_aarch64_emit_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Move Emit/Build Orchestration Into Emit Owners

# Current Packet

## Just Finished

Step 2: Move Emit/Build Orchestration Into Emit Owners completed for
`src/backend/mir/aarch64/codegen/emit.cpp` and
`src/backend/mir/aarch64/codegen/emit.hpp`.

`build_module(const PreparedBirModule&)` remains the public prepared-module
entry coordinator and now routes through private emit-owned helpers for
handoff-error result construction, module shell assembly, MIR lowering plus
compatibility projection, and accepted `BuildResult` assembly. The existing
call order is preserved: target profile resolution, prepared-module handoff
validation, module shell construction, prepared-function MIR lowering,
compatibility function-record derivation, compatibility projection, and success
result construction.

No ABI policy, prepared-function traversal bodies, instruction dispatch,
instruction-record helpers, printer spelling, tests, plan files, source ideas,
or markdown shards were moved or edited.

## Suggested Next

Next coherent packet: Step 3 should inspect the adjacent compiled owners
(`instruction.*`, `dispatch.*`, and `machine_printer.*`) and tighten or record
their boundaries without moving instruction family semantics or printer bodies
into `emit.cpp`.

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

Passed. Supervisor-selected proof command:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_aarch64_(instruction_dispatch|module_skeleton_contract|function_traversal|machine_printer|target_record_core_contract)$' > test_after.log 2>&1
```

`test_after.log` contains the canonical proof output. The selected subset ran
5 tests and all passed.
