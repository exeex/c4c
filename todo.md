Status: Active
Source Idea Path: ideas/open/547_bir_local_memory_call_metadata_boundary_review.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Inspect Producer Surfaces For Retained Groups

# Current Packet

## Just Finished

Step 3 inspected producer and consumer surfaces for the retained Step 1 local
memory group and Step 2 direct-call semantic-family group. No implementation
files, tests, plan files, source ideas, review artifacts, or root logs were
changed.

Latest summary-file evidence remains separate from retained work-root evidence:
- `build/agent_state/rv64_gcc_c_torture_backend_summary.tsv` and
  `build/agent_state/rv64_gcc_c_torture_backend_failed.txt` still describe only
  the later `src/20001026-1.c` single-case state, not the retained local-memory
  or direct-call groups.
- The classifications below are for retained per-case logs under
  `build/rv64_gcc_c_torture_backend/*/case.log`, not for latest summary-file
  rows.

Retained group classifications:
- `prepared_local_address_base_plus_offset_missing`: 41 retained
  `unsupported_local_memory_access` rows. Classification: concrete evidence
  gap at the prepared/RV64 boundary, not BIR-owned on current evidence.
  Retained representative logs (`src/20000519-1.c`, `src/20070212-3.c`,
  `src/pr35800.c`, `src/pr65369.c`) reach the RV64 object route and report
  that RV64 requires a prepared frame-slot or pointer-value base-plus-offset
  local-memory address. That means semantic BIR lowering reached a prepared
  local-memory consumer surface. Source inspection shows the RV64 local-memory
  consumer accepts only specific prepared facts: `PreparedMemoryAccess` rows
  whose address is default-space, non-volatile, frame-slot or pointer-value
  based, size/alignment-compatible, `can_use_base_plus_offset`, and immediate
  encodable. The retained case logs do not include focused prepared dumps for
  these representatives, so Step 3 cannot honestly classify the first owner as
  prepared-owned or RV64-owned without fresh proof of whether the prepared fact
  is missing/incomplete or present and rejected.
- `semantic_lir_to_bir_direct_call_family`: 26 retained rows. Classification:
  BIR-owned semantic call-lowering boundary, with a remaining concrete evidence
  gap inside that boundary. Retained representative logs (`src/20000419-1.c`,
  `src/20000717-1.c`, `src/20040703-1.c`, `src/pr67226.c`) fail before the
  prepared object handoff with `semantic call family 'direct-call semantic
  family'`. Source inspection places the failure in
  `BirFunctionLowerer::lower_call_inst`, which owns direct-call callee
  classification, signature/return ABI, argument lowering, byval/sret handling,
  and `CallArgumentSourceRelationship` publication before prepared lowering.
  Existing BIR route6 and printer surfaces can expose `call_arg_source`,
  producer, publication, and call-result rows, but the retained case logs do
  not name which direct-call fact is absent. This is BIR-owned as a first
  boundary, but not implementation-ready until a focused dump names the missing
  call fact.

Focused proof surfaces for future implementation ideas:
- Local-memory follow-up proof should rerun one retained representative such as
  `src/20000519-1.c` through semantic BIR, prepared BIR, and RV64 object routes
  in the same workspace snapshot. It should inspect BIR `load_local`/`store_local`
  address records, prepared `memory_access` and `address_materialization` rows,
  `PreparedAddressBaseKind`, `can_use_base_plus_offset`, size/alignment,
  pointer-value register homes, and RV64 rejection at
  `fragment_for_prepared_store_local` / `fragment_for_prepared_load_local`.
  The owner is prepared-owned only if the required prepared BPO fact is absent
  or incoherent; it is RV64-owned only if the required prepared fact is present
  and the RV64 consumer still rejects it.
- Direct-call follow-up proof should rerun one retained representative such as
  `src/20000717-1.c` or `src/pr67226.c` through semantic BIR and, if it reaches
  prepared, prepared BIR. It should inspect `lower_call_inst` failure notes,
  `CallInst` argument/return ABI rows, `call_arg_source` annotations, route6
  call argument source/producer/publication records, call-result records, and
  any byval/sret aggregate layout facts. The owner remains BIR-owned only for
  a named missing semantic fact; otherwise the row should stay an evidence gap.

Producer surfaces inspected:
- BIR local-memory semantic dispatcher:
  `src/backend/bir/lir_to_bir/memory/coordinator.cpp`
- BIR memory address/provenance route records:
  `src/backend/bir/bir_route3_memory.cpp`
- BIR direct-call semantic lowering and failure note:
  `src/backend/bir/lir_to_bir/calling.cpp`
- BIR call publication/lookup records and printer annotations:
  `src/backend/bir/bir_route6_call_publication.cpp`,
  `src/backend/bir/bir_printer.cpp`
- Prepared contract/verifier and lookup surfaces:
  `src/backend/prealloc/prepared_contract_verifier.hpp`,
  `src/backend/prealloc/prepared_lookups.cpp`
- RV64 prepared local-memory consumer:
  `src/backend/mir/riscv/codegen/prepared_local_memory_emit.cpp`

## Suggested Next

Execute Step 4 as a lifecycle/classification packet: decide whether to split a
new local-memory evidence-reproduction idea for the prepared/RV64 boundary, a
new direct-call BIR evidence-reproduction idea, or close this review as not yet
implementation-ready.

## Watchouts

- Do not implement RV64 local-memory lowering from the retained
  `unsupported_local_memory_access` logs alone; they do not prove whether the
  required prepared BPO fact exists.
- Do not implement direct-call metadata repair from the retained semantic
  family rows alone; they prove the BIR boundary but not the exact missing
  call fact.
- Keep latest summary-file rows separate from retained work-root case logs
  until a fresh full RV64 torture scan replaces the mixed summary state.
- Keep `review/557_step13_vector_local_memory_review.md` untouched unless the
  supervisor explicitly brings it into scope.

## Proof

Step 3 evidence-classification validation:

```sh
git diff --check -- todo.md && scripts/plan_review_state.py show
```

Result: passed. `scripts/plan_review_state.py show` reported
`current_step_id` = `3` and `current_step_title` =
`Inspect Producer Surfaces For Retained Groups`.
