Status: Active
Source Idea Path: ideas/open/104_hir_safe_legacy_lookup_demotion.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Preserve Or Retire Parity Proof Deliberately

# Current Packet

## Just Finished

Completed `plan.md` Step 5 by classifying the parity/fallback state left after
the completed HIR-only demotions from Steps 2 through 4. No implementation file
changed in this packet.

Audit classification quotes from
`review/103_hir_post_dual_path_legacy_readiness_audit.md`:
`Module::classify_*_decl_lookup`, `resolve_*_decl`, and `decl_lookup_*` parity
records are `legacy-proof-only`: they compare structured and rendered lookup,
record authority/mismatches, and intentionally return legacy on mismatch during
proof. `Module::fn_index` and `global_index` backing `find_*_by_name_legacy`
are `bridge-required` because callers still start from `Node::name`, synthesized
mangled names, or downstream fallback names. `Module::struct_def_owner_index`
and `find_struct_def_by_owner_structured` are `safe-to-demote` targets where
callers have `HirRecordOwnerKey`, while `Module::struct_defs` rendered tag map
is `bridge-required` for `TypeSpec::tag`, layout, base traversal, field lookup,
initialization normalization, and HIR-to-LIR layout selection. Lowerer struct
method owner mirrors and parity counters are `legacy-proof-only`, while the
rendered method/static-member maps remain `bridge-required`.

Step 5 classification:
- Removed authority: HIR-internal owner-aware record-existence checks in
  `Lowerer::lower_struct_def` now prefer `find_struct_def_by_owner_structured`
  when complete owner metadata exists; direct method/member probe bypasses from
  earlier packets were routed through parity-recording helpers instead of
  continuing as independent direct-map authority.
- Retained fallback: rendered `find_*_by_name_legacy`, `Module::struct_defs`,
  method/static-member rendered maps, and base-tag traversal remain in place
  where metadata is incomplete or where callers still enter through
  `Node::name`, synthesized mangled names, `TypeSpec::tag`, trait/static
  evaluation, or HIR-to-LIR bridge seams.
- Retained proof-only instrumentation: decl lookup hit/parity records,
  owner/rendered struct-def parity mismatches, and Lowerer method/member parity
  counters remain as proof infrastructure for the demoted HIR-only call sites.
- Bridge-required rendered lookup: broad `TypeSpec::tag` consumers,
  `Module::struct_defs` layout/base/member paths, HIR-to-LIR function/global
  fallback names, and emitted ABI/layout spelling are outside Step 5 and belong
  to later bridge work.

Parity retirement decision:
No parity proof can be retired safely in this packet. The review-backed
classifications support keeping `legacy-proof-only` instrumentation until
authority flips are broader and proven, and the remaining rendered lookups are
classified as `bridge-required` or `needs-more-parity-proof`, not
deletion-ready.

## Suggested Next

Proceed to `plan.md` Step 6. The next packet should run the supervisor-selected
final validation/handoff pass and decide whether the runbook is complete,
blocked on bridge-required work, or ready for plan-owner lifecycle handling.

## Watchouts

- Do not retire `decl_lookup_*` records, owner parity mismatches, or Lowerer
  parity counters without a review-backed proof that the corresponding rendered
  authority is no longer needed.
- Do not delete or weaken rendered fallbacks that serve `Node::name`,
  synthesized mangled names, `TypeSpec::tag`, trait/static evaluation,
  diagnostics/printers, ABI/link spelling, or HIR-to-LIR layout/codegen seams.
- Step 5 made no code change because any stronger removal would cross into
  bridge work or remove proof-only observation before the audit says it is
  deletion-ready.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -L hir > test_after.log`
passed 73/73.

Proof log: `test_after.log`.
