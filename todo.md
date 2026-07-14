# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 6.2
Current Step Title: Receive the checked i64 output-only inline-assembly binding

## Just Finished

- Plan Step 6.1 receives only `ordinary_results[0]` with a valid native
  current-function `LirValueId`, exact i32 type, `Output` role, and constraint
  index zero. The existing `InlineAsmNode` result is now source-backed and
  registered by that ID, and exactly one same-ID i32 direct-global `Store` use
  is accepted. Raw and Canonical tests cover presentation-text independence,
  malformed/missing/duplicate/cross-function IDs, role/index/count/type,
  Store mismatch, and transactional rollback.

## Suggested Next

- Implement only Plan Step 6.2: receive the Step-7.25 i64 output-only binding
  and its one same-ID i64 direct-global Store use through the existing
  `InlineAsmNode` source-result registry. Preserve Step 6.1 i32 receipt; do
  not use compatibility spelling or opaque payload as authority.

## Watchouts

- This row is only the non-explicit-register i64/`Output`/index-zero binding
  and one same-ID i64 direct-global Store. Inputs, read/write/tied values,
  memory/address/immediate forms, clobber/explicit-register meaning,
  vectors/aggregates/multiple results, `insn_r`, other widths, all opaque-text-
  derived facts, and all CFG successor labels remain fail-closed. The preserved
  payload is not semantic value or target authority.

## Proof

- Plan Step 6.1 passed:
  `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^(backend_lir_to_bir_interface|frontend_lir_call_type_ref)$'
  > test_after.log`.
  The focused subset passed; `test_after.log` is the proof log.
- Plan Step 6.2 must run a fresh build and the same focused receipt selection:
  `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^(backend_lir_to_bir_interface|frontend_lir_call_type_ref)$'`.
