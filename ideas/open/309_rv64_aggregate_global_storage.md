# RV64 Aggregate Global Storage

## Goal

Implement a focused RV64 foundation for aggregate, array, union, and struct
global storage so prepared global-symbol accesses can emit data symbols and
offset loads/stores without collapsing into undefined-main or generic
unsupported behavior.

## Why This Exists

The undefined-main triage found 19 cases in the secondary
`aggregate_global_storage` bucket. These cases currently share the
`rv64_text_only_fail_closed` output-contract failure, but representative
prepared BIR already exposes the next capability gap: RV64 must emit aggregate
global data and lower offset accesses against global symbols.

Representative control:

```c
typedef struct { int x; int y; } s;
s v;
int main() { v.x = 1; v.y = 2; return 3 - v.x - v.y; }
```

For `src/00024.c`, prepared addressing records global-symbol accesses to
`v` at offsets `0` and `4` for both stores and loads.

Evidence to reuse:

- `build/rv64_c_testsuite_probe_v2/representative_evidence.md`
- `build/rv64_c_testsuite_probe_v2/classification.md`
- `build/rv64_c_testsuite_probe_v2/repair_order.md`
- `build/rv64_c_testsuite_probe_v2/classification_work/buckets/aggregate_global_storage.txt`

## In Scope

- Emit RV64 global data for simple aggregate/array storage layouts that appear
  in prepared BIR.
- Lower prepared global-symbol loads and stores with byte offsets, starting
  with integer-sized fields like `src/00024.c`.
- Preserve symbol names, section placement, alignment, and zero-initialized
  storage semantics needed by the focused aggregate bucket.
- Add backend coverage for aggregate global storage and offset field
  load/store behavior.
- Recheck representative bucket cases after the shared output-contract fix so
  aggregate-specific failures are visible.

## Out Of Scope

- Pointer-valued global initializers, function-pointer storage, or relocation
  semantics beyond aggregate data layout.
- Floating global representation and floating load/use lowering.
- String literal storage/address materialization and external calls.
- Claiming the entire 19-case bucket passes without proving the underlying
  data layout and offset access mechanism.
- Repairing the shared `.text`-only output contract; that belongs to the
  prerequisite output-contract idea.

## Candidate Cases

- Minimal representative: `src/00024.c`
- Additional bucket candidates:
  - `src/00047.c`
  - `src/00048.c`
  - `src/00050.c`
  - `src/00091.c`
  - `src/00115.c`
  - `src/00146.c`
  - `src/00148.c`
  - `src/00163.c`
  - `src/00176.c`
- Full secondary bucket:
  `build/rv64_c_testsuite_probe_v2/classification_work/buckets/aggregate_global_storage.txt`

## Acceptance Criteria

- A focused RV64 backend test proves global aggregate data emission for a
  zero-initialized struct or array symbol.
- A focused RV64 backend test proves loads and stores to aggregate fields via
  prepared global-symbol offsets.
- `src/00024.c` no longer fails because aggregate global data or offset
  accesses are dropped after the output-contract repair has landed.
- The implementation leaves pointer globals, floating globals, and string
  constants as separate follow-up scopes unless a small shared helper is needed
  and tested through aggregate-only behavior.
- Proof uses a narrow representative subset and keeps the full c-testsuite
  sweep as non-blocking triage evidence.

## Reviewer Reject Signals

- The patch special-cases symbol `v`, offsets `0` and `4`, or c-testsuite
  filenames instead of lowering prepared aggregate global storage generally.
- It treats aggregate cases as passing by weakening expectations, accepting
  missing data symbols, or allowing empty `.text` output to remain successful.
- It bundles pointer global initializers, floating global representation,
  string constants, or external calls into the same acceptance claim.
- It only rewrites classification artifacts, test labels, or helper names
  while global data emission or offset load/store lowering is still missing.
- It proves only `src/00024.c` while nearby aggregate cases from the bucket are
  neither examined nor deliberately deferred with evidence.
