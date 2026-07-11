# Current Packet

Status: Active
Source Idea Path: ideas/open/718_transitive_current_block_incoming_expression_authority_decomposition.md
Source Plan Path: plan.md
Current Step ID: 9
Current Step Title: Compose stable-key owner facts upstream

## Just Finished

- Completed plan Step 9 by composing each direct publication-source routing
  fact with only its proven transitive producer dependencies at the upstream
  preallocation owner-fact boundary.
- Preserved direct source identity and authoritative policy ownership while
  making dependency identity the stable routed key; all four focused probes
  and the broader backend subset are green without target-local construction.

## Suggested Next

- Execute plan Step 10's unchanged integration validation and handoff packet.

## Watchouts

- A composed dependency fact deliberately retains the direct publication's
  source ID/name pair; only its routed ID/name pair changes to the dependency.
- Dependency closure is tracked per direct source so unrelated direct
  publication families are not cross-composed.

## Proof

- Ran `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_' > test_after.log 2>&1` exactly as delegated.
- Build passed; all 324/324 backend tests passed, including the direct identity,
  transitive dependency, stable-key composition, and no-policy/zero-fact
  focused probes. Proof log: `test_after.log`.
