# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 6.2
Current Step Title: Select the next authority-backed terminator or inline-assembly row

## Just Finished

- Plan Step 6.1 receives only `ordinary_results[0]` with a valid native
  current-function `LirValueId`, exact i32 type, `Output` role, and constraint
  index zero. The existing `InlineAsmNode` result is now source-backed and
  registered by that ID, and exactly one same-ID i32 direct-global `Store` use
  is accepted. Raw and Canonical tests cover presentation-text independence,
  malformed/missing/duplicate/cross-function IDs, role/index/count/type,
  Store mismatch, and transactional rollback.

## Suggested Next

- Select only the next authority-backed Plan Step 6.2 row; do not infer CFG
  successor or inline-asm semantics from labels, text, constraints, clobbers,
  or target interpretation.

## Watchouts

- Inline asm remains closed to i64, inputs, read/write/tied values,
  memory/address/immediate forms, clobber/explicit-register meaning,
  vectors/aggregates/multiple results, `insn_r`, and all opaque-text-derived
  facts. The preserved payload is not semantic value authority.

## Proof

- Plan Step 6.1 passed:
  `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^(backend_lir_to_bir_interface|frontend_lir_call_type_ref)$'
  > test_after.log`.
  The focused subset passed; `test_after.log` is the proof log.
