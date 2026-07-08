Status: Active
Source Idea Path: ideas/open/608_prepared_global_data_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory prepared/global authority blockers

# Current Packet

## Just Finished

- Completed Step 1 from `plan.md`: inventoried the prepared/global authority
  blockers in the current RV64 gcc-torture backend artifacts and selected the
  first narrow implementation target.
- Exact Step 1 evidence set:
  - `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`
  - `docs/rv64_gcc_torture_1000_pass_recovery/dependency_order_to_1000.md`
  - `build/agent_state/rv64_gcc_c_torture_backend_failed.txt`
  - `build/rv64_gcc_c_torture_backend/src_20010924-1.c/case.log`
  - `build/rv64_gcc_c_torture_backend/src_strlen-7.c/case.log`
  - `build/rv64_gcc_c_torture_backend/src_pr79737-2.c/case.log`
  - `build/rv64_gcc_c_torture_backend/src_20020118-1.c/case.log`
  - `build/rv64_gcc_c_torture_backend/src_ieee_fp-cmp-2.c/case.log`
  - generated row lists under
    `build/agent_state/608_step1_prepared_global_inventory/`
- Prepared/global authority candidate rows selected for this plan:
  - selected global object-data contract, `17` current rows:
    `src/20010924-1.c`, `src/20030224-2.c`, `src/20030828-1.c`,
    `src/20080522-1.c`, `src/20101025-1.c`, `src/20181120-1.c`,
    `src/921019-1.c`, `src/921110-1.c`, `src/930608-1.c`,
    `src/builtin-prefetch-5.c`, `src/const-addr-expr-1.c`,
    `src/pr53084.c`, `src/pr57860.c`, `src/pr57876.c`,
    `src/pr57877.c`, `src/pr61517.c`, `src/pr61682.c`.
  - prepared global memory facts, `12` current rows:
    `src/20000703-1.c`, `src/20041218-1.c`, `src/20060930-2.c`,
    `src/20140212-1.c`, `src/20190228-1.c`, `src/pr36034-1.c`,
    `src/pr57861.c`, `src/pr58431.c`, `src/pr58662.c`,
    `src/pr64756.c`, `src/pr91137.c`, `src/strlen-7.c`.
  - direct global-symbol base-plus-offset, `11` current rows:
    `src/990326-1.c`, `src/pr42721.c`, `src/pr57568.c`,
    `src/pr58385.c`, `src/pr58564.c`, `src/pr66556.c`,
    `src/pr68624.c`, `src/pr68911.c`, `src/pr70602.c`,
    `src/pr79737-2.c`, `src/pr82387.c`.
- RV64/global consumer rows separated for
  `ideas/open/609_rv64_global_data_consumer.md`:
  - global symbol emission, `18` current rows in the logs:
    `src/20020118-1.c`, `src/20020201-1.c`, `src/20040629-1.c`,
    `src/20040705-1.c`, `src/20040705-2.c`, `src/20050224-1.c`,
    `src/20060110-2.c`, `src/20090814-1.c`, `src/921123-2.c`,
    `src/931018-1.c`, `src/pr37924.c`, `src/pr39240.c`,
    `src/pr40493.c`, `src/pr52209.c`, `src/pr57130.c`,
    `src/pr57875.c`, `src/pr66757.c`, `src/pr71550.c`.
  - global access width, `13` current rows:
    `src/20020213-1.c`, `src/20080117-1.c`, `src/941021-1.c`,
    `src/align-2.c`, `src/floatunsisf-1.c`, `src/ieee/20000320-1.c`,
    `src/ieee/20010114-2.c`, `src/ieee/20030331-1.c`,
    `src/ieee/920518-1.c`, `src/ieee/fp-cmp-2.c`,
    `src/ieee/mzero3.c`, `src/loop-ivopts-1.c`, `src/pr49218.c`.
- Existing prepared facts/helpers that should own the first missing facts:
  - selected object data: `PreparedGlobalObjectData` and
    `populate_prepared_object_data_plans()` in
    `src/backend/prealloc/object_data.cpp`; verification flows through
    `verify_prepared_selected_object_data_contract()`.
  - global memory facts: `PreparedMemoryAccess`, `PreparedAddress`,
    `prepared_global_symbol_memory_has_publication_authority()`,
    `find_prepared_global_load_access()`, and the `LoadGlobalInst` /
    `StoreGlobalInst` `build_direct_symbol_backed_access()` paths in
    `src/backend/prealloc/stack_layout/coordinator.cpp`.
  - direct global-symbol base-plus-offset:
    `resolve_prepared_global_symbol_address()`,
    `build_direct_symbol_backed_address()`,
    `publish_scalar_global_layout_authority()`, and
    `publish_integer_array_global_layout_authority()` in the prepared
    addressing/coordinator path.
- First narrow implementation target recommendation: start with Step 3,
  prepared global memory facts, using `src/strlen-7.c` plus neighboring
  `supported prepared global memory facts` rows. This crosses the prepared
  global-memory and direct-address helpers without touching RV64 emission, and
  it can fail closed when `build_direct_symbol_backed_address()` cannot publish
  a `GlobalSymbol` base, in-bounds range, layout authority, size, or alignment.

## Suggested Next

- Implement the first narrow Step 3 packet: repair prepared publication for
  supported `LoadGlobalInst` / `StoreGlobalInst` global-symbol memory accesses
  so `prepared_global_symbol_memory_has_publication_authority()` succeeds for
  semantically known direct global-symbol base-plus-offset accesses, proving
  with `src/strlen-7.c`, at least one neighboring prepared global memory-facts
  row, and one direct base-plus-offset row such as `src/pr79737-2.c`.

## Watchouts

- Keep RV64 global symbol emission and access-width lowering out of scope.
- Do not change expectations, unsupported markers, allowlists, timeouts,
  runtime behavior, accounting, or testcase-specific matching.
- Preserve RV64/global consumer rows for `ideas/open/609_rv64_global_data_consumer.md`.
- The bucket map records `17` global-symbol emission rows, while the current
  log inventory finds `18`; keep that discrepancy in the 609 handoff lane.
- `src/pr79737-2.c` appeared in older instruction-fragment evidence, but the
  current `case.log` first stop is prepared direct global-symbol
  base-plus-offset memory addressing, so use the current first diagnostic for
  this plan.

## Proof

- No build/test proof required by this inventory-only packet.
- Focused artifact checks run:
  - `sed -n '1,260p' plan.md`
  - `sed -n '1,260p' todo.md`
  - `sed -n '1,260p' ideas/open/608_prepared_global_data_authority.md`
  - `sed -n '1,260p' docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`
  - `sed -n '1,260p' docs/rv64_gcc_torture_1000_pass_recovery/dependency_order_to_1000.md`
  - `sed -n '1,200p' build/agent_state/rv64_gcc_c_torture_backend_failed.txt`
  - `rg -n 'object-data contract|supported prepared global memory facts|prepared direct global-symbol base-plus-offset|emit prepared global symbol|only 1-, 2-, 4-, and 8-byte prepared global memory accesses' build/rv64_gcc_c_torture_backend -g 'case.log'`
  - row-list generation into
    `build/agent_state/608_step1_prepared_global_inventory/*.txt`
- `test_after.log` intentionally not created or modified because the delegated
  proof contract required no build/test run and forbade root-level proof logs.
