Status: Active
Source Idea Path: ideas/open/500_semantic_global_static_gep_admission_producer.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Or Route The Global/Static GEP Contract

# Current Packet

## Just Finished

Step 2 audited the current global/static producer surfaces and recorded the
contract route under `build/agent_state/500_step2_global_static_gep_contract/`.
Current surfaces are not sufficient for final semantic admission because no
durable lower certificate ties global object identity, layout path, dynamic
range, element byte range, derivation/provenance, and LIR producer coordinate
into one matched authority record.

## Suggested Next

Execute Step 3 as a prerequisite-route packet: publish production
`global_static_gep_authority` certificates from existing LIR-to-BIR global
address/layout/range/coordinate inputs. Do not implement final semantic
global/static GEP admission until that lower certificate exists.

## Watchouts

- This is a BIR semantic producer packet, not RV64/MIR lowering.
- Do not infer global/static GEP authority from raw shape, names, labels, final
  homes, object files, relocations, lowered target behavior, or route-local
  slots.
- Keep `src/20051104-1.c` fail-closed or route it to string/global-pointer
  provenance first; direct global-object GEP authority does not prove the
  pointed string-literal object behind `s.name[s.len]`.
- Keep `src/ieee/copysign2.c` fail-closed or route it through runtime/string
  intrinsic consumer authority before admitting its static-array GEP use.
- Existing `bir::Global`, `LoadGlobalInst`/`StoreGlobalInst`, and lowerer
  `GlobalAddress` helpers are useful lower surfaces, but there is not yet a
  durable certificate tying global object, layout path, dynamic range, element
  byte range, and LIR producer coordinate together.
- Preserve final admission fail-closed for all rows until a matching available
  `global_static_gep_authority` record exists.

## Proof

Step 2 contract/routing proof:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed > test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed. Output preserved in `test_after.log`.
