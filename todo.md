Status: Active
Source Idea Path: ideas/open/611_rv64_terminator_fragment_lowering.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Close-Readiness Classification

# Current Packet

## Just Finished

Step 4 from `plan.md` classified close readiness for idea 611 from the current
residual evidence without lifecycle, implementation, test, expectation,
unsupported marker, or allowlist edits.

Classification: idea 611 should be split before source-idea closure. The active
runbook has exhausted the clean RV64 terminator-consumer slice, but the source
idea should not be marked close-ready as a single completed backend effort
because the remaining five direct object residuals still carry
`unsupported_terminator_fragment` while the prepared and asm evidence points to
mixed first owners.

Residual classification by first owner:
- Terminator consumer only: no close-ready follow-up packet emerged. All five
  rows have prepared branch/control-flow evidence, but none form a clean
  standalone Step 4 implementation packet that avoids adjacent ownership.
- Global storage/global data: `src/ieee/20001122-1.c` and `src/991030-1.c`
  have explicit fused-compare branches, but `--codegen asm` stops earlier at
  unsupported prepared global storage layout. Keep this with the existing
  global-storage/global-data follow-up track, including
  `ideas/open/619_bir_aggregate_global_store_handoff.md` and
  `ideas/open/621_rv64_prepared_global_value_location_consumer.md`.
- Join/select publication: `src/921124-1.c` and `src/920710-1.c` involve
  join/select carrier or predecessor-terminator parallel-copy authority before
  the branch consumer can be treated as an isolated terminator-lowering issue.
  Keep this with `ideas/open/616_select_publication_source_wiring.md` and
  `ideas/open/617_scalar_compare_publication.md`.
- Stack-backed condition/freshness: `src/921124-1.c` has branch stack-load
  authority evidence mixed with the join/select shape. Keep freshness and
  stack-backed condition handling separated under
  `ideas/open/615_branch_stack_source_residual_audit.md` or a successor if that
  audit discovers a narrower implementation owner.
- ABI: no current residual should be folded into idea 611 under ABI ownership;
  keep any call-result or stack-frame ABI work under
  `ideas/open/613_abi_call_result_stack_frame_lowering.md`.
- Move-bundle: no move-bundle implementation should be added to this route.
  If a later probe shows predecessor move materialization rather than
  join/select publication is first owner, create or activate a dedicated
  move-bundle follow-up instead of extending idea 611.
- Generic instruction fragments: no generic instruction-fragment work should be
  added to this route; keep that separated under
  `ideas/open/612_rv64_instruction_fragment_consumers.md`.

## Suggested Next

Plan-owner lifecycle packet:

Use this Step 4 classification to decide whether to retire or regenerate the
current runbook and split the remaining residual ownership into the existing
global-storage/global-data, join/select publication, stack-backed
condition/freshness, ABI, generic instruction-fragment, and possible
move-bundle follow-up routes.

## Watchouts

- Do not close idea 611 as fully complete from direct object labels alone. The
  right close-readiness state is split, because the current residual evidence
  names multiple first owners outside the clean RV64 terminator-consumer route.
- Direct object probes alone keep the five rows under
  `unsupported_terminator_fragment`, but prepared-BIR and asm probes show mixed
  root ownership. Do not claim another implementation packet unless the
  supervisor intentionally chooses one semantic subfamily under its proper
  first-owner idea.
- `src/ieee/20001122-1.c` and `src/991030-1.c` belong to the global
  storage/global data track before any terminator-consumer retry.
- `src/921124-1.c` and `src/920710-1.c` belong to join/select publication or
  stack-backed condition/freshness classification before any narrow floating
  compare branch packet.
- The direct probe path is `build/c4cll`, not `build/src/apps/c4cll`.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

Result: passed. Test subset: `^backend_`. Proof log: `test_after.log`.
The delegated proof is sufficient for this todo-only classification packet; no
code changed.

Focused diagnostic probes used direct `build/c4cll --codegen obj --target
riscv64-linux-gnu -o <tmp>.o`, `--dump-prepared-bir`, and targeted
`--codegen asm` commands against the five GCC torture source paths under
`tests/c/external/gcc_torture/`. Object probes confirmed all five still report
`unsupported_terminator_fragment`; prepared-BIR probes provided the
classification evidence above.
