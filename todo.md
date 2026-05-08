Status: Active
Source Idea Path: ideas/open/152_hir_lir_rendered_owner_compatibility_retirement.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Retire Aggregate TypeSpec Layout Fallbacks

# Current Packet

## Just Finished

Completed Plan Step 4 by retiring rendered compatibility-tag recovery in shared aggregate layout lookup after a complete structured owner miss.

`find_typespec_aggregate_layout` now treats a complete `record_def` owner key as authoritative: it returns the structured layout on a hit and stops on a miss before consulting rendered compatibility tags. Complete non-`record_def` owner metadata also stops rendered fallback after a structured miss, while incomplete metadata still preserves the legacy compatibility path. Focused coverage now proves an indexed stale rendered compatibility tag cannot recover a layout after the complete owner key misses, and the existing no-owner compatibility test continues to prove incomplete metadata keeps the compatibility shape.

## Suggested Next

Next coherent packet: choose the next rendered-owner authority family from the Step 1 inventory outside shared aggregate layout lookup, unless supervisor review wants more direct coverage around this helper path.

## Watchouts

- `typespec_aggregate_owner_key(ts, mod)` still performs rendered spelling repair for owner-key canonicalization; this packet only prevents `find_typespec_aggregate_layout` from using rendered tags as a fallback layout authority after complete structured owner misses.
- Existing parser-owned collision fixtures remain intentionally spelling-repaired through `record_def` canonicalization before the miss decision.
- Keep the next packet separate from parser owner-probe code, member typedef recovery, and LIR lowering unless the supervisor explicitly selects one of those families.

## Proof

Supervisor-selected proof passed and wrote `test_after.log`:

`cmake --build --preset default --target c4cll frontend_hir_lookup_tests && ctest --test-dir build -R '^frontend_hir_lookup_tests$' --output-on-failure > test_after.log`

Supervisor acceptance guard also passed on the broader aggregate/codegen-adjacent subset:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_|positive_sema_ok_call_variadic_aggregate_runtime_c|abi_)' > test_after.log`
