Status: Active
Source Idea Path: ideas/open/847_lir_universal_model_string_escape_hatch_deletion.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Handoff final valid-LIR disposition to 797

# Current Packet

## Just Finished

Completed Step 4 after the local memory identity and provenance type-ref
repairs.

Accepted Step 4 progress includes:
- `f0427e92e`: local aggregate memory identity now threads LIR type refs into
  local slot and intrinsic aggregate layout paths.
- `c78ff20c5`: scalar subobject provenance now uses metadata-bearing local and
  pointer type refs, and the rendered `TypeDeclMap` scalar-facts bridge is
  fenced to callers with no StructNameId-bearing type ref, such as globals and
  legacy/no-id local or pointer address state.

The remaining Step 4 bridge comments are classified as deliberate no-id,
legacy, or owner-specific compatibility boundaries rather than unmet 847
deletion criteria. `memory/intrinsics.cpp` has stale wording that says
intrinsic helpers do not yet receive LirTypeRef/StructNameId metadata; after
`f0427e92e`, metadata-bearing local aggregate slots do use a type-ref lookup
path and the raw helper is only the no-ref rendered-text bridge. That comment
can be repaired as housekeeping, but it does not require another Step 4
implementation packet.

## Suggested Next

Suggested Next: execute Step 5 by preparing the final valid-LIR disposition
handoff to 797. The handoff should summarize deleted/fenced surfaces, accepted
proof, and residual owners without claiming that 797 itself is complete.

## Watchouts

- Do not reopen already classified call ABI, global, aggregate-parameter,
  memory/addressing, local GEP, local slot, intrinsic, provenance, or central
  types bridge comments unless Step 5 evidence exposes a direct contradiction.
- Optional code-comment cleanup in `memory/intrinsics.cpp` may clarify the
  post-`f0427e92e` state, but it is not Step 4 capability work.

## Proof

Ran exactly:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

Result: passed. `test_after.log` contains `100% tests passed, 0 tests failed
out of 6`.

Supervisor regression guard:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
reported before 6/6, after 6/6, result PASS.
