# Prepared Select Chain Dump Contract Coverage

## Goal

Add prepared-printer and contract-test visibility for target-significant
select-chain facts that prealloc already publishes for backend consumers.

## Why This Exists

The BIR/prealloc control publication lookup boundary audit found no duplicated
BIR control authority in the scalar select-chain and direct-global dependency
paths. Those facts are legitimate prepared target-facing publication facts:
backends consume them to decide whether select-chain materialization or
direct-global dependency handling already covers a value.

The gap is contract visibility. `find_prepared_scalar_select_chain_materialization`
and `find_prepared_direct_global_select_chain_dependency` expose facts such as
availability, root value name, `root_is_select`, root instruction index, and
direct-global dependency. Store/source and edge publication paths also carry
directly supporting source-producer kind, block label, and instruction index
facts. Current prepared dumps do not make those facts visible, which leaves
future x86/RISC-V rebuild work dependent on implicit lookup behavior.

## Owned Files

- `src/backend/prealloc/prepared_printer.*`
- `src/backend/prealloc/prepared_printer/*`
- `tests/backend/bir/backend_prepared_printer_test.cpp`
- Narrow supporting fixture files only if the existing prepared-printer tests
  need one more case to exercise the contract.

## In Scope

- Print scalar select-chain materialization facts that are already available to
  prepared lookup consumers, including availability, root value, whether the
  root is a select, and the root instruction index.
- Print direct-global select-chain dependency facts used by call, indirect
  callee, call-argument, and store-source publication consumers.
- Print directly supporting source-producer kind, source block label, and
  source instruction index facts where they explain the same scalar
  select-chain or direct-global dependency contract.
- Add focused prepared-printer tests that fail if those facts disappear from
  the dump.

## Out Of Scope

- Rewriting prepared lookup APIs.
- Moving select-chain materialization or target emission policy into BIR.
- Changing AArch64, x86, or RISC-V lowering behavior except as needed to keep
  existing tests compiling.
- Creating separate compare-join continuation or select-materialization
  join-transfer work; the audit found adequate lookup and dump coverage there.

## Proof Expectations

- Build the affected backend/prepared-printer test target.
- Run the prepared-printer test subset, including a case that demonstrates:
  - scalar select-chain materialization dump visibility,
  - direct-global select-chain dependency dump visibility, and
  - source-producer kind/block/index visibility when it directly supports the
    same contract.
- The proof must not rely on weakening existing dump expectations or marking a
  supported path unsupported.

## Acceptance Criteria

- Prepared dumps expose the select-chain and direct-global facts consumed by
  backend prepared lookup users.
- Contract tests cover both scalar select-chain materialization and
  direct-global select-chain dependency visibility.
- Source-producer kind/block/index appears only as supporting provenance for
  those consumer-facing facts, not as a broad unrelated dump expansion.

## Reviewer Reject Signals

- The change treats missing dump text as a reason to rewrite lookup authority
  or target lowering policy.
- It adds broad prepared-printer output for unrelated publication facts without
  tying them to scalar select-chain or direct-global dependency visibility.
- It proves only one named expression shape while nearby select-chain and
  direct-global dependency consumers remain unrepresented.
- It weakens existing prepared-printer expectations, hides fields behind
  unstable formatting, or marks supported cases unsupported.
- It creates new compare-join continuation or select-materialization
  join-transfer work despite the audit's no-action classification.

## Close Note

Closed on 2026-06-03.

The source idea is complete. Prepared dumps now expose the selected
consumer-facing select-chain and direct-global facts without duplicating lookup
authority in the printer. The final route uses `select_chain_lookups.cpp` as
shared helper authority and `prepared_printer/select_chains.cpp` as formatting
only, after the rejected printer-local scalar traversal was removed.

Scalar select-chain materialization visibility is provided by the helper-backed
`--- prepared-select-chain-materializations ---` section. Rows expose
`function`, `block`, `value`, `root_is_select`, and `root_inst`. Supporting
source-producer provenance appears on scalar rows as `source_producer`,
`source_producer_block`, and `source_producer_inst`.

Direct-global select-chain dependency visibility is covered in
`prepared-call-plans` argument lines and scalar materialization rows. Call
argument lines carry `direct_global_select_chain=yes`,
`direct_global_source=<value>`, `direct_global_root_is_select=yes|no`, and
`direct_global_root_inst=<index>`. Store-source dump visibility remains
deferred because this route did not introduce a bounded prepared-module
store-source carried fact; this close does not create separate store-source,
compare-join continuation, or select-materialization join-transfer work.

Proof status: final backend validation passed with `169/169` backend tests, and
the close-time backend regression guard passed with `169/169` before and after,
no new failures, and no resolved failures. Focused prepared-printer tests cover
the `load_global -> select -> call arg` path, including call-argument
direct-global labels, scalar materialization rows, and source-producer
provenance.
