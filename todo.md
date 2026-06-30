Status: Active
Source Idea Path: ideas/open/468_carrier_alias_identity_publication_api.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Route Identity API Packet

# Current Packet

## Just Finished

Completed Step 3 implementation for idea 468.

Implemented a mutable pre-consumer prepared identity publication stage:
`populate_select_carrier_alias_identity(PreparedBirModule&)`, called from
`BirPreAlloc::publish_contract_plans()` after control-flow, value-location,
and edge-publication inputs exist and before prepared dump/RV64 const
consumers observe the module.

The stage interns synthesized carrier-alias result names into canonical
`prepared.names.value_names` only from semantic prepared facts: available
select-materialization edge publication, binary source producer whose result is
the selected source, matching join transfer, successor-block candidate, final
select, distinct named alias result, payload use of the selected source, no
condition use of the selected source, and candidate feeding the final select.

Focused coverage proves the repaired API is const-correct and shared:

- A representative duplicate carrier-alias fixture starts with candidates but
  unavailable authority because `%alias0` / `%alias1` are absent from the
  canonical prepared name table.
- `populate_select_carrier_alias_identity` publishes those alias identities into
  the canonical module, after which
  `collect_prepared_select_carrier_alias_authorities` returns an available
  record without requiring alias value ids/homes.
- An extra non-carrier source use remains fail-closed as
  `non_carrier_source_use`, proving identity publication does not bypass the
  later source-use closure authority check.

Fresh representative probe for `20010329-1`:

- prepared dump status `0`
- object route status `0`
- `%t46 -> %t50` now prints
  `select_carrier_alias_authority ... status=available ... carrier_aliases=2
  source_use_closure=yes`

Artifacts:

- `build/agent_state/468_step3_shared_identity_publication/summary.md`
- `build/agent_state/468_step3_shared_identity_publication/probe_status.tsv`
- `build/agent_state/468_step3_shared_identity_publication/evidence_snippets.txt`
- `build/agent_state/468_step3_shared_identity_publication/20010329-1.prepared.out`
- `build/agent_state/468_step3_shared_identity_publication/20010329-1.prepared.err`
- `build/agent_state/468_step3_shared_identity_publication/20010329-1.object.out`
- `build/agent_state/468_step3_shared_identity_publication/20010329-1.object.err`

## Suggested Next

Execute Step 4: residual disposition and close-readiness review for idea 468.

Recommended owned files:

- `todo.md`
- `build/agent_state/468_step4_residual_disposition/*`

Suggested Step 4 proof:

```sh
git diff --check
```

Step 4 should classify whether the shared identity publication API is
close-ready, whether `20010329-1` advancing to object status `0` leaves any
idea-468 residual, and whether any remaining work belongs to a later
non-identity owner.

## Watchouts

- This slice intentionally did not touch RV64 lowering.
- Identity publication is still not authority; RV64 consumers must continue to
  require available carrier-alias authority records.
- Source-use closure remains in the later authority collector, not the identity
  population stage.
- Preserve fail-closed behavior for wrong edge/source/final result, duplicate
  candidates, raw alias-name inference, stale stack-loads, and generic move
  cases.
- Do not modify `test_baseline.new.log`, `test_baseline.log`,
  `test_before.log`, or `review/`.

## Proof

Step 3 proof:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Result: passed.

Proof log: `test_after.log`.
