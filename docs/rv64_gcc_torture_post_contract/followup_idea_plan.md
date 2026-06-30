# RV64 Instruction Fragment Follow-Up Ranking

Status: Step 4 follow-up ideas created from the regenerated Step 2 bucket
evidence and Step 3 ranking.

Evidence source:

- `docs/rv64_gcc_torture_post_contract/current_scan_summary.md`
- `docs/rv64_gcc_torture_post_contract/failure_bucket_map.md`
- `build/agent_state/unsupported_instruction_fragment_rows.tsv`
- `build/agent_state/rv64_gcc_torture_backend_current_20260630T032216Z.log`

The source scan recorded `total=1467 passed=404 failed=1063`. The row TSV
contains 190 `unsupported_instruction_fragment` rows. The rankings below use
the disjoint Step 2 owner buckets and keep F128 out of the ordinary non-F128
implementation backlog.

## Ranking Rule

Rank candidates by:

- row count and representative spread across gcc_torture cases
- likelihood that one semantic lowering rule helps many nearby rows
- owning-layer clarity
- prepared-fact completeness
- risk that the route would require producer repair before RV64 lowering

Ordinary non-F128 RV64 lowering work ranks ahead of F128. Producer-owned gaps
are not ranked as RV64 lowering work until their prepared facts are coherent.

## Ordered Implementation Candidates

| Rank | Candidate | Rows | Owning layer | Expected broad impact | Route note |
| ---: | --- | ---: | --- | --- | --- |
| 1 | Select and join materialization | 54 | RV64 object lowering | Highest row count and direct ordinary non-F128 lowering surface; a semantic select/join materialization route should cover multiple comparison and value-merge shapes. | Implementation idea should define legal scalar select lowering and reject branch-fact invention or filename-shaped special cases. |
| 2 | Call-adjacent scalar publication and inline-asm materialization | 38 | RV64 call/inline-asm object lowering | Large ordinary scalar bucket spanning call results, call arguments, and inline asm fragments; likely to unlock several call-near cases without touching ABI aggregates. | Split scalar call/inline-asm publication from aggregate `sret`/`byval` and from F128 helper calls. |
| 3 | Pointer cast and address materialization | 21 | RV64 pointer/address lowering, with producer review for provenance gaps | Material for address-bearing tests and pointer/integer boundary rows; broad if prepared provenance is adequate. | First implementation packet should only cover rows with coherent `inttoptr`/`ptrtoint` facts and route incomplete provenance to producer review. |
| 4 | Integer div/rem lowering | 17 | RV64 integer instruction lowering | Clear instruction-family owner with scalar `udiv`, `sdiv`, `urem`, and `srem`; lower count than pointer work but more implementation-ready. | Prefer as the first pure integer-op packet if pointer provenance review is not ready. Avoid case-specific divisor or width matching. |
| 5 | Aggregate `sret`/`byval` call-storage | 19 | Prepared ABI producer plus RV64 aggregate call lowering | Count is high, but some rows show suspicious prepared aggregate size/alignment facts, so the first impact depends on producer validation. | Should become a producer/ABI-contract idea before an RV64 lowering idea when facts are incoherent. |
| 6 | Integer arithmetic shift right | 11 | RV64 integer instruction lowering | Smaller but clean ordinary integer bucket; useful follow-up after div/rem or as a narrow integer lowering slice. | Add a semantic `ashr` route, not a constants-only or representative-file shortcut. |
| 7 | Large literal and materialization | 10 | RV64 immediate/materialization lowering | Small-to-medium bucket with likely shared literal materialization benefit beyond these rows. | Needs a general constant materialization strategy for out-of-range immediates. |
| 8 | Global memory/addressing residual | 2 | RV64 memory/address lowering | Very small residual; likely best handled after pointer/address and literal materialization clarify addressability. | Keep separate from pointer cast rows until global address facts are inspected. |
| 9 | Scalar F32/F64 conversion/op residual | 2 | RV64 scalar floating-point lowering | Tiny non-F128 residual; preserve as separate scalar-FP work but do not let it steer the main backlog. | Do not mix with F128 runtime-helper or long-double work. |
| 10 | F128 quarantine | 16 | F128/runtime-helper quarantine | Explicitly not the primary route for this runbook despite its row count. | Lowest priority unless a future proof shows F128 blocks a broader ordinary non-F128 owner. |

## High-Impact Non-F128 Ideas

The evidence supports at least these implementation-ready follow-up ideas:

1. RV64 select and join materialization for ordinary scalar values.
   - Evidence: 54 rows, the largest bucket, with representative rows such as
     `src/pr43236.c`, `src/pr51933.c`, and `src/pr68328.c`.
   - Owner: RV64 object lowering.
   - Acceptance shape: lower prepared scalar select/join fragments through a
     semantic rule that works across comparison/value-merge forms.
   - Reject signals: branch-fact invention, expectation downgrades, or
     testcase/file-name matching.

2. RV64 call-adjacent scalar and inline-asm fragment materialization.
   - Evidence: 38 rows, with representative rows such as `src/pr38533.c`,
     `src/pr40657.c`, and `src/pr45695.c`.
   - Owner: RV64 call/inline-asm object lowering.
   - Acceptance shape: publish scalar call results, scalar call arguments, and
     inline-asm fragments without absorbing aggregate ABI or F128 helper cases.
   - Reject signals: treating aggregate `sret`/`byval` rows as scalar calls,
     broadening inline asm through named-case shortcuts, or changing pass/fail
     accounting.

3. RV64 pointer cast and address materialization with producer-gap boundaries.
   - Evidence: 21 rows, with representative rows such as `src/930930-1.c`,
     `src/20000622-1.c`, and `src/20010329-1.c`.
   - Owner: RV64 pointer/address lowering when prepared provenance is coherent;
     producer review when it is not.
   - Acceptance shape: lower coherent `inttoptr`/`ptrtoint` materialization and
     explicitly reject rows whose prepared facts do not describe safe address
     provenance.
   - Reject signals: guessing addresses, bypassing missing provenance, or
     routing producer gaps through RV64 inference.

4. RV64 integer div/rem scalar lowering.
   - Evidence: 17 rows, with representative rows such as `src/20021120-3.c`,
     `src/20030105-1.c`, and `src/20090113-2.c`.
   - Owner: RV64 integer instruction lowering.
   - Acceptance shape: lower scalar signed/unsigned div/rem over coherent
     operand and result types.
   - Reject signals: divisor-specific handling, width-specific shortcuts not
     grounded in the type system, or expectation rewrites.

5. Prepared aggregate ABI contract review for `sret`/`byval` call-storage.
   - Evidence: 19 rows, but Step 2 found suspicious prepared aggregate sizes or
     alignments in some rows.
   - Owner: prepared ABI producer first, then RV64 aggregate call lowering.
   - Acceptance shape: separate coherent aggregate call-storage rows from
     producer-fact defects before writing RV64 lowering.
   - Reject signals: treating incoherent size/alignment as RV64 object-lowering
     work or merging aggregate ABI with scalar call publication.

## F128 Quarantine

F128 remains quarantined and lowest priority for this runbook. The F128 bucket
has 16 rows, but its owner is runtime-helper/long-double support rather than
ordinary RV64 instruction-fragment lowering. It should not displace the larger
ordinary non-F128 buckets above unless future evidence proves it blocks a
broader non-F128 owner.

## Suggested Step 4 Idea Order

Create follow-up ideas in this order unless supervisor review chooses a
different route:

1. RV64 scalar select/join materialization.
2. RV64 call-adjacent scalar and inline-asm materialization.
3. RV64 pointer/address materialization with explicit producer-gap boundaries.
4. RV64 integer div/rem lowering, or use this as the first pure-integer packet
   if pointer provenance review needs more evidence.
5. Prepared aggregate ABI contract review before aggregate RV64 lowering.

## Step 4 Created Ideas

- `ideas/open/427_rv64_scalar_select_join_materialization.md`
- `ideas/open/428_rv64_call_adjacent_scalar_inline_asm_materialization.md`
- `ideas/open/429_rv64_pointer_address_materialization.md`
- `ideas/open/430_rv64_integer_div_rem_lowering.md`
- `ideas/open/431_prepared_aggregate_abi_contract_review.md`
