Status: Active
Source Idea Path: ideas/open/258_aarch64_emit_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Tighten Adjacent Owner Boundaries

# Current Packet

## Just Finished

Step 3: Tighten Adjacent Owner Boundaries completed as an ownership
classification pass for `instruction.*`, `dispatch.*`, and
`machine_printer.*`.

The adjacent owners already retain their proper responsibilities relative to
the emit shard: `instruction.*` owns structured instruction records, prepared
fact validation, record factories, surface/status names, and record-derived
metadata; `dispatch.*` owns prepared block traversal, BIR/prepared-operation
classification, diagnostics, and appending selected machine instructions to
machine blocks; `machine_printer.*` owns target instruction printing and
assembly line payload spelling. No narrow boundary defect required a code
change, and no instruction family semantics, printer bodies, tests, markdown
shards, plan files, or source ideas were moved into `emit.cpp`.

## Suggested Next

Next coherent packet: Step 4 should reconcile the remaining durable `emit.md`
content against the compiled emit/dispatch/instruction/printer boundaries and
classify stale direct-text-emitter inventory without changing tests or moving
semantics into `emit.cpp`.

## Watchouts

- Preserve behavior; this idea is source ownership redistribution.
- Do not use testcase-shaped shortcuts, expectation rewrites, or unsupported
  downgrades as progress.
- Keep `emit.cpp` thin; it should coordinate prepared-module build flow and
  delegate to the adjacent owners instead of absorbing instruction spelling,
  selected-node families, or backend semantics.
- Do not delete `emit.md` until its durable valid content is reconciled or
  classified as stale/out of scope for close.
- The `emit.md` direct-text emitter inventory is mostly stale; Step 4 should
  compare that markdown to the now-confirmed compiled boundaries instead of
  reviving direct text emission in `emit.cpp`.

## Proof

Passed. Supervisor-selected proof command:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_aarch64_(instruction_dispatch|module_skeleton_contract|function_traversal|machine_printer|target_record_core_contract)$' > test_after.log 2>&1
```

`test_after.log` contains the canonical proof output. The selected subset ran
5 tests and all passed.
