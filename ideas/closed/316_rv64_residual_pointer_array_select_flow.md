# RV64 Residual Pointer Array Select Flow

## Goal

Classify and repair the remaining RV64 pointer/local-array/select runtime
residuals that were deliberately split out of idea 312 closure because they are
not one local frame-slot address materialization rule.

## Why This Exists

The idea 312 Step 5 sweep left four runtime failures that are adjacent to local
address work but no longer match the completed local frame-slot publication,
local array pointer-step, or byte element access repairs. They need a separate
triage route so fixes do not overfit one candidate or silently expand aggregate
and function-pointer work.

## In Scope

- Pointer-to-pointer local chains after the first dereference.
- Array parameter to local array pointer flow.
- Indexed local array select/update chains when local array addressing is only
  one part of the failure.
- Pointer `select`, `inttoptr`, and `ptrtoint` conditional flow when it is the
  first bad runtime mechanism.
- Reclassification into narrower ideas if these candidates split further.

## Out Of Scope

- Core local frame-slot address publication already closed by idea 312.
- Aggregate-local subobject/byval repair tracked separately.
- Function-pointer address, return, or indirect-call repair tracked separately.
- Broad switch/control-flow repair unless it is required to prove a selected
  pointer/array value flow.

## Candidate Evidence

- `src/00005.c`: pointer-to-pointer local residual after the first dereference.
- `src/00077.c`: array parameter/local array pointer flow.
- `src/00143.c`: broad indexed local array select/update chains plus
  switch-shaped control flow.
- `src/00144.c`: pointer select, `inttoptr`, and `ptrtoint` conditional flow.

## Acceptance Criteria

- The route first separates these candidates into coherent first-bad
  mechanisms using prepared BIR, emitted assembly, and qemu evidence.
- Any implementation packet repairs a semantic pointer/array/select lowering
  rule and proves nearby same-feature cases.
- Remaining broad control-flow or ABI residue is reclassified into its own
  durable idea instead of absorbed into this route.

## Completion Note

Closed after Step 5 evidence in
`build/rv64_c_testsuite_probe_latest/triage_316_step5/summary.md` and
`probe_results.tsv`.

The landed idea 316 repairs cover:

- pointer-to-pointer local frame-address materialization, with `src/00005.c`
  now emitting, linking, and running under qemu with exit status `0`;
- pointer/integer select-chain materialization and publication focused
  coverage;
- pointer-typed select publication into local pointer slots focused coverage.

The remaining candidates are evidence-backed separate residual families, not
unfinished pointer/local-array/select work for this source idea:

- `src/00077.c` and `src/00143.c` are stack-homed fused
  compare/control-flow emission residuals. Their first bad facts are
  stack-homed fused compare values feeding branch/loop control before the
  later pointer/array bodies are reached.
- `src/00144.c` is a deeper nested select-chain/store-source publication
  residual. The focused pointer-typed select publication repair reaches later
  nested select chains, where prepared store-source records show
  `missing_destination_access`.

Follow-up ideas were opened for those separate residuals:

- `ideas/open/319_rv64_stack_homed_fused_compare_control_flow.md`
- `ideas/open/320_rv64_nested_select_chain_store_source_publication.md`

Close-time guard: `cmake --build --preset default -j` passed, then
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
matched the rolled-forward `test_before.log` backend subset. The regression
guard passed with `--allow-non-decreasing-passed`: 254 passed, 1 accepted
existing failure (`backend_riscv_prepared_edge_publication`), 0 new failures.

## Reviewer Reject Signals

- A patch targets only one candidate filename or one observed stack offset
  without proving a semantic pointer/array/select lowering rule.
- `00143` is claimed fixed by skipping switch-shaped flow or by weakening the
  runtime contract.
- `00144` is treated as ordinary local address materialization while
  pointer-integer conversion or select value flow remains unhandled.
- The route mixes aggregate byval or function-pointer indirect-call work into
  this idea without a lifecycle split.
- Evidence-only classification is claimed as capability progress without a
  matching backend or runtime proof.
