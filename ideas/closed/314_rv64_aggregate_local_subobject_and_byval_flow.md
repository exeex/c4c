# RV64 Aggregate Local Subobject And Byval Flow

## Goal

Repair RV64 prepared lowering for aggregate-local subobject access, nested
aggregate field stores/loads, and aggregate copy/byval flows that remain after
local frame-slot address materialization was completed.

## Why This Exists

Idea 312 repaired the core local stack/address materialization path and closed
with concrete residuals from the Step 5 sweep. `src/00019.c`, `src/00046.c`,
and `src/00140.c` still emit and link but fail at runtime because aggregate
subobject and aggregate copy behavior is broader than local address
publication.

## In Scope

- Aggregate-local subobject address/value flow for structs and unions.
- Nested aggregate field offset stores and loads on RV64.
- Aggregate copy and byval/local aggregate transfer when it is the first bad
  runtime mechanism.
- Focused backend coverage for semantic aggregate-local behavior rather than
  c-testsuite filenames.

## Out Of Scope

- Generic local frame-slot address publication already covered by idea 312.
- Function pointer storage, return, or indirect-call policy.
- Vararg or floating aggregate ABI repair beyond classification unless it is
  inseparable from a narrow aggregate-local proof.
- Global aggregate storage and global address materialization.

## Candidate Evidence

- `src/00019.c`: aggregate self-pointer chain; stops after publishing `s.p =
  &s` and before chained aggregate pointer loads.
- `src/00046.c`: nested union/struct field offset stores and loads stop on
  nested subobject store flow.
- `src/00140.c`: aggregate byval copy and local aggregate pointer flow mixed
  with vararg/float aggregate ABI surface.

## Acceptance Criteria

- Focused backend tests cover nested aggregate-local subobject stores/loads and
  aggregate copy/byval transfer where supported.
- Representative candidates emit, link, and run under qemu, or residual ABI
  gaps are reclassified with concrete emitted-code evidence.
- Repairs use aggregate layout, local memory, and RV64 prepared emission rules
  rather than filename, offset, or field-name matching.

## Completion Note

Closed after Step 5 evidence in
`build/rv64_c_testsuite_probe_latest/triage_314_step5/summary.md` and
`probe_results.tsv`.

The aggregate-local subobject repair is complete for this idea's narrow scope:
`src/00019.c` now emits, links, and exits under qemu with status `0`. Focused
aggregate-local backend coverage passed `6/6` in the executor proof.

The two remaining candidates are no longer aggregate-local subobject emission
failures:

- `src/00046.c` emits and links, aggregate stores/reloads are present, and the
  remaining qemu exit `2` is classified as a separate select-chain /
  short-circuit runtime lowering residual.
- `src/00140.c` emits and links, but assembly remains truncated at the `f1`
  byval aggregate handling boundary. This is classified as a separate RV64
  prepared byval aggregate call ABI residual, including aggregate-address /
  payload transport and floating aggregate lane handling.

Follow-up ideas were opened for those separate residuals:

- `ideas/open/317_rv64_select_chain_short_circuit_runtime_lowering.md`
- `ideas/open/318_rv64_byval_aggregate_call_abi.md`

Close-time guard: `cmake --build --preset default -j` passed, then
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
matched the rolled-forward `test_before.log` backend subset. The regression
guard passed with `--allow-non-decreasing-passed`: 239 passed, 1 accepted
existing failure (`backend_riscv_prepared_edge_publication`), 0 new failures.

## Reviewer Reject Signals

- The route special-cases `00019`, `00046`, `00140`, fixed field offsets, or
  struct/union names instead of repairing aggregate-local lowering.
- Progress is claimed through expectation rewrites, unsupported downgrades, or
  skipped qemu checks.
- A patch only republishes frame-slot base addresses while nested aggregate
  subobject loads/stores still use missing or stale values.
- A byval/ABI residual is hidden by a fake copy, fake epilogue, or generic
  fall-through that does not move the aggregate payload correctly.
- Broad ABI rewrites outside the aggregate-local evidence are mixed into this
  route without focused proof.
