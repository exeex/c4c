Status: Active
Source Idea Path: ideas/open/108_prepared_select_chain_dump_contract_coverage.md
Source Plan Path: plan.md
Current Step ID: 7
Current Step Title: Final Validation And Close Readiness

# Current Packet

## Just Finished

Step 7 final validation passed and the prepared select-chain dump contract is
close-ready for plan-owner review.

Close-readiness notes:
- Scalar select-chain materialization dump visibility is implemented through
  the helper-backed `--- prepared-select-chain-materializations ---` section.
- Scalar rows cover root fields: `function`, `block`, `value`,
  `root_is_select`, and `root_inst`.
- Direct-global select-chain dependency coverage is present in both surfaces:
  `prepared-call-plans` argument lines and scalar materialization rows.
- Supporting source-producer provenance coverage is present on scalar rows via
  `source_producer`, `source_producer_block`, and `source_producer_inst`.
- Focused prepared-printer tests prove the `load_global -> select -> call arg`
  path keeps call-argument direct-global labels and scalar row labels visible.
- Store-source visibility remains deferred because this route did not introduce
  a bounded prepared-module store-source carried fact; the current accepted
  dump surfaces are call arguments and scalar select-chain materializations.
- The route did not change target lowering policy and did not model synthetic
  dump facts in MIR/AArch64/x86/RISC-V lowering.
- Lookup authority remains centralized in prealloc helper APIs. The work split
  select-chain helper implementation into `select_chain_lookups.cpp` and added
  a narrow root-producer helper, but did not restore printer-local traversal or
  rewrite lookup policy.

## Suggested Next

Plan-owner closure review for
`ideas/open/108_prepared_select_chain_dump_contract_coverage.md`.

## Watchouts

- Keep store-source dump visibility deferred unless a future route introduces a
  bounded carried prepared-module fact.
- Do not collapse the helper-backed scalar section back into printer-local
  reconstruction.

## Proof

Passed. Ran:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

Proof log: `test_after.log`.

Result: 169 backend tests passed, 0 failed.
