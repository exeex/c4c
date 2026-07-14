# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 6.3
Current Step Title: Select the next authority-backed terminator or inline-assembly row

## Just Finished

- Plan Step 6.2 extends the existing source-result registry receipt to the
  exact i64 `ordinary_results[0]` row: valid current-function `LirValueId`,
  `Output` role, constraint index zero, and one same-ID i64 direct-global
  `Store`. The Step 6.1 i32 row remains accepted. Raw and Canonical tests now
  cover both widths plus missing/invalid/duplicate bindings, role/index/type,
  unknown/cross-function/mismatched Store uses, and transactional rollback.

## Suggested Next

- Select only the next source-authorized Step 6.3 terminator or inline-assembly
  row with evidenced typed destination; do not infer CFG labels or semantic
  facts from compatibility text.

## Watchouts

- The accepted rows are only non-explicit-register i32/i64 `Output` index-zero
  bindings and one same-ID direct-global Store. Inputs, read/write/tied values,
  memory/address/immediate forms, clobber/explicit-register meaning,
  vectors/aggregates/multiple results, `insn_r`, other widths, all opaque-text-
  derived facts, and all CFG successor labels remain fail-closed. The preserved
  payload is not semantic value or target authority.

## Proof

- Plan Step 6.2 passed:
  `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^(backend_lir_to_bir_interface|frontend_lir_call_type_ref)$'
  > test_after.log`.
  The focused subset passed; `test_after.log` is the proof log.
