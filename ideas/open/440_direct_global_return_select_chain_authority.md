# Direct-Global Return And Select-Chain Authority

Status: Open
Type: Producer/consumer authority idea
Parent: `ideas/closed/433_rv64_global_select_pointer_memory_residuals.md`
Source Evidence: `build/agent_state/433_step4_residual_disposition/`
Owning Layer: Direct-global return and select-chain prepared authority

## Goal

Define and consume explicit authority for direct-global return values and
direct-global select chains before RV64 lowering materializes them.

## Why This Exists

Idea 433 identified direct-global select/return residuals, especially
`20041112-1`, after completing the selected global object-data packet. The
remaining evidence includes raw `bir.ret ptr @global`, global loads/stores, and
direct-global select-chain facts. RV64 must not infer return authority or
select-chain roots from raw BIR.

## In Scope

- Audit direct-global return and select-chain evidence from
  `build/agent_state/433_step4_residual_disposition/`.
- Define prepared facts that authorize returning a global pointer and
  materializing direct-global select chains.
- Separate producer gaps from coherent RV64 consumer work.
- Add focused coverage for accepted facts and fail-closed missing/incoherent
  authority.

## Out Of Scope

- Selected global object-data support completed by idea 433.
- Store-source/global-memory publication and pointer-value memory authority.
- General terminator publication unrelated to direct-global return/select
  authority.
- Inferring return authority from raw `bir.ret ptr @global` or select roots
  from raw instruction shape.

## Acceptance Criteria

- Direct-global return/select-chain residuals are classified by first owner
  with representative evidence.
- Required prepared authority for return values and select-chain roots is
  explicit and covered by tests.
- Missing/incoherent authority remains fail-closed.
- RV64 consumer work, if any, is bounded to explicit prepared facts.

## Reviewer Reject Signals

- Reject RV64 code that treats raw `bir.ret ptr @global`, global symbol names,
  select instruction shape, or testcase names as sufficient return/select-chain
  authority.
- Reject testcase-shaped fixes keyed to `20041112-1`, one selected block, one
  function name, or one dump layout.
- Reject expectation rewrites, unsupported-marker downgrades, allowlists, or
  pass/fail accounting changes claimed as progress.
- Reject broad control-flow or memory rewrites that hide the missing
  direct-global authority behind a new helper name.

