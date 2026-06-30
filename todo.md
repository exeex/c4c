Status: Active
Source Idea Path: ideas/open/447_immediate_global_store_source_encoding.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Residual Disposition And Close Readiness

# Current Packet

## Just Finished

Completed Step 4: re-probed representative immediate global-store evidence and
recorded residual disposition under
`build/agent_state/447_step4_residual_disposition/`.

Representative immediate-source verdict:

- `20041112-1 main.entry.0` remains the only bounded immediate-valued
  global-store candidate.
- Fresh prepared evidence shows `bir.store_global @global, i32 1` with
  `store_source function=main block=entry inst=0 source=<none> ... source_producer=immediate`.
- The matching destination row remains coherent:
  `symbol=global offset=0 size=4 align=4 layout_authority=scalar_layout
  range_verdict=proven_in_bounds`.

Residual classification:

| residual | evidence | disposition |
| --- | --- | --- |
| immediate global-store source encoding | `20041112-1 main.entry.0` has `source_producer=immediate` | complete for idea 447 |
| non-immediate global stores | `foo.block_1.2 source=%t4 source_producer=binary`; `bar.block_5.2 source=%t10 source_producer=binary` | out of scope for immediate-source encoding |
| local/frame-slot store | `bar.entry.2 intent=store_local_publication source_producer=unknown` | out of scope local publication |
| direct-global call/select-chain facts | `direct_global_select_chain=yes` rows remain in prepared dump | out of scope direct-global/select-chain authority |
| RV64 object route | object probe reports `unsupported_terminator_fragment` | out of scope target terminator lowering; not an immediate-source residual |

Lifecycle recommendation: close idea 447 as complete for immediate global-store
source encoding. No exact remaining in-scope immediate-source packet is
identified.

## Suggested Next

Plan-owner close-readiness review for idea 447. If the supervisor wants to
continue object-route progress for `20041112-1`, route it through the
appropriate non-447 owner, such as RV64 terminator lowering or direct-global
select/return authority, not immediate-source encoding.

## Watchouts

- Keep this plan limited to immediate global-store source encoding.
- Do not reopen scalar or integer-array global layout authority; those are
  closed by ideas 446 and 448.
- Do not infer immediate values or source publication in RV64 from raw BIR
  store shape, testcase names, block labels, symbol names, or one dump layout.
- Keep `source=<none>` and `source_producer=unknown` fail-closed until
  producer facts are explicit.
- Keep non-immediate global stores (`source=%t4`, `%t10`, `%t1`), local stores,
  pointer-value memory, direct-global return/select-chain rows, and target
  lowering outside any 447 close claim.
- Preserve idea 439 store-publication predicates; do not weaken
  `prepared_store_global_publication_has_authority` to accept unknown sources.
- No RV64 target lowering changed in Step 3; Step 4 should not claim object
  route completion beyond the prepared immediate-source fact.
- Do not accept or modify `test_baseline.new.log`.

## Proof

Step 4 residual-disposition validation:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Result: passed. Proof log: `test_after.log`.
