Status: Active
Source Idea Path: ideas/open/847_lir_universal_model_string_escape_hatch_deletion.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Remove expired adapters and prove compile-time separation

# Current Packet

## Just Finished

Completed Step 4 residual classification through the call ABI, global
aggregate, and memory projection families.

Accepted classifications:
- `aggregate.cpp`, `call_abi.cpp`, `global_initializers.cpp`, `globals.cpp`,
  `memory/addressing.cpp`, and `memory/local_gep.cpp` residual comments now
  document deliberate no-id, legacy, or target-policy compatibility paths after
  metadata-bearing callers have already been forced through structured
  `LirTypeRef` lookup and fail-closed behavior.
- The latest focused backend proof passed, and the supervisor accepted the
  full-suite baseline candidate at 3038/3038 after the global aggregate packet.

Step 4 is not ready to advance yet. The remaining comments in
`memory/intrinsics.cpp`, `memory/local_slots.cpp`, `memory/provenance.cpp`, and
`types.cpp` still describe active valid-lowering paths where local aggregate,
pointer-derived leaf, byte-offset, provenance, or central aggregate layout
state can reach raw rendered aggregate type text because structured type
identity is not threaded to that boundary. That is still 847-owned because the
source idea requires deleting expired universal/string escape hatches and
proving compile-time separation in valid LIR authority paths, not merely
labeling a missing metadata carrier as compatibility.

## Suggested Next

Suggested Next: keep Step 4 active and execute one bounded in-scope packet:
thread structured aggregate type identity, or prove an explicit no-id boundary,
across the local aggregate memory chain that feeds
`memory/intrinsics.cpp`, `memory/local_slots.cpp`, `memory/provenance.cpp`, and
the central `types.cpp` raw `TypeDeclMap` fallback.

The packet should start at the local-slot/address state that owns aggregate
object identity, then update only the directly connected intrinsic fill/copy,
scalar byte-offset, and provenance lookups needed to stop metadata-bearing
valid LIR from recovering through rendered type spelling. If a subpath is
proven deliberately no-id only, leave a narrow comment saying so; if it still
lacks a metadata carrier, repair that carrier instead of advancing to Step 5.

## Watchouts

- Keep this packet narrow. Do not reopen already classified call ABI, global,
  aggregate-parameter, memory/addressing, or local GEP comments unless the
  memory aggregate identity repair exposes a direct contradiction.
- Do not silently expand 847 into unrelated owners. If a residual bridge is not
  an 847 deletion target, preserve the current Step 4 return point and ask
  plan-owner to create or switch to the correct separate initiative.
- Keep legitimate no-id text fallbacks explicit and narrow; do not replace the
  deleted universal model with a renamed compatibility bag.

## Proof

Ran exactly:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

Result: passed. `test_after.log` contains `100% tests passed, 0 tests failed
out of 6`.

Supervisor regression guard:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
reported before 6/6, after 6/6, result PASS.
