Status: Active
Source Idea Path: ideas/open/180_bir_return_chain_prepared_api_contraction.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Inventory Prepared Public Surface

# Current Packet

## Just Finished

Plan Step 2 - Inventory Prepared Public Surface is complete. The prepared
return-chain public surface was inventoried with text search across `src` and
`tests`; no code, test, docs, plan, idea, review, or root-log files were edited.

Contraction inventory:

- `PreparedReturnChainLookups` in
  `src/backend/prealloc/prepared_lookups.hpp:23` is obsolete public surface.
  It is only consumed by `prepared_lookups.cpp` implementation internals and
  by the test-only oracle fixture in
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:54`.
- `PreparedReturnChainLookups::terminal_return_values_by_chain_value` and
  `PreparedReturnChainLookups::next_operand_values_by_chain_value` in
  `src/backend/prealloc/prepared_lookups.hpp:24` expose semantic terminal and
  next-operand answers. They are obsolete public projection fields except for
  test-only duplicate-conflict mutation at
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:10244` and
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:10279`.
- `PreparedFunctionLookups::return_chains` in
  `src/backend/prealloc/prepared_lookups.hpp:33` is obsolete public aggregate
  surface. `make_prepared_function_lookups` still populates it at
  `src/backend/prealloc/prepared_lookups.cpp:1708`, but no searched `src` or
  `tests` consumer reads `.return_chains`.
- `prepared_return_chain_value_key` is public only in
  `src/backend/prealloc/prepared_lookups.hpp:39`. It is a required internal
  implementation detail for `publish_prepared_return_chain` and the find
  helpers in `src/backend/prealloc/prepared_lookups.cpp:1080`,
  `src/backend/prealloc/prepared_lookups.cpp:1385`,
  `src/backend/prealloc/prepared_lookups.cpp:2204`, and
  `src/backend/prealloc/prepared_lookups.cpp:2219`; its only external consumer
  is the test-only duplicate-conflict oracle at
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:10246`.
- `make_prepared_return_chain_lookups` is public only in
  `src/backend/prealloc/prepared_lookups.hpp:43` and implemented at
  `src/backend/prealloc/prepared_lookups.cpp:1401`. It is a required internal
  detail while `make_prepared_function_lookups` builds prepared lookup
  aggregates, but its direct public consumers are test-only oracle setup at
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:89` and
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:9907`.
- `find_prepared_return_chain_terminal_value` and
  `find_prepared_return_chain_next_operand_value` are public only in
  `src/backend/prealloc/prepared_lookups.hpp:55` and
  `src/backend/prealloc/prepared_lookups.hpp:61`, with implementations at
  `src/backend/prealloc/prepared_lookups.cpp:2195` and
  `src/backend/prealloc/prepared_lookups.cpp:2210`. They are obsolete public
  semantic query surface; all external uses are test-only oracle dependencies
  in `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:104`,
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:9907`, and
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:10227`.
- The only diagnostics/assertion text mentioning prepared return-chain helpers
  is test-only text in
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:9933`,
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:9942`, and the
  fail-closed messages from
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:9989` through
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:10302`.

## Suggested Next

Proceed to plan Step 3 by contracting the eligible prepared return-chain API in
`src/backend/prealloc/prepared_lookups.hpp` and
`src/backend/prealloc/prepared_lookups.cpp`, with matching test-oracle
retargeting or helper-localization in
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp`.

Step 3 handoff:

- Remove `PreparedFunctionLookups::return_chains` from
  `src/backend/prealloc/prepared_lookups.hpp:33` and stop initializing it in
  `src/backend/prealloc/prepared_lookups.cpp:1708`; no live consumer should
  require replacement.
- Hide `PreparedReturnChainLookups`, `prepared_return_chain_value_key`, and
  `make_prepared_return_chain_lookups` behind implementation-local scope if
  the prepared oracle still needs them internally; otherwise delete them with
  their builder path.
- Remove public declarations for
  `find_prepared_return_chain_terminal_value` and
  `find_prepared_return_chain_next_operand_value`; if Step 3 keeps internal
  prepared construction, keep any remaining find logic private to
  `prepared_lookups.cpp`.
- Update or split the test-only oracle dependencies in
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:54`,
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:104`,
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:9907`, and
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:10227` so semantic
  terminal and next-operand coverage uses the BIR/Route 8 route rather than
  requiring public prepared helper access.

## Watchouts

- Treat `PreparedFunctionLookups::return_chains` as a still-owned live
  production dependency only in the sense that it is constructed by
  `make_prepared_function_lookups`; there is no live read-side dependency.
- `prepared_return_chain_value_key` and the two maps are intentionally mutated
  by duplicate-conflict tests. Retarget those tests without preserving public
  map access as a compatibility promise.
- `src/backend/mir/aarch64/codegen/alu.cpp:1436` has only a comment saying it
  must not consult prepared return-chain fallback facts; no AArch64 code path
  references the prepared helper symbols.
- Do not weaken tests or downgrade supported-path expectations.
- Keep API cleanup separate from new BIR schema behavior or consumer migration.

## Proof

No build required for this read-only inventory packet; no proof command was
delegated and `test_after.log` was not updated.

Searches run:

```sh
rg -n "PreparedReturnChainLookups|return_chains|prepared_return_chain_value_key|make_prepared_return_chain_lookups|find_prepared_return_chain_terminal_value|find_prepared_return_chain_next_operand_value" src tests
rg -n "prepared[ _-]return[ _-]chain|return-chain helper|return chain helper|PreparedReturnChain|prepared.*helper" src tests
rg -n "return_chains|find_prepared_return_chain|make_prepared_return_chain|PreparedReturnChainLookups|prepared_return_chain_value_key" src/backend tests/backend --glob '!src/backend/prealloc/prepared_lookups.cpp' --glob '!src/backend/prealloc/prepared_lookups.hpp' --glob '!tests/backend/bir/backend_prepared_lookup_helper_test.cpp'
```
