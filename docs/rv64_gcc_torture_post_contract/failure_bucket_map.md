# RV64 gcc_torture Failure Bucket Map

Status: Step 3 ranking complete for the regenerated row set.

## Evidence Source For `unsupported_instruction_fragment`

Use `docs/rv64_gcc_torture_post_contract/current_scan_summary.md` as the
evidence-source decision for this bucket. Step 2 classified rows from:

- `build/agent_state/unsupported_instruction_fragment_rows.tsv`
- `build/agent_state/rv64_gcc_torture_backend_current_20260630T032216Z.log`

The scan recorded `total=1467 passed=404 failed=1063`. The row TSV contains
190 `unsupported_instruction_fragment` rows from that same scan. Local
prepared-BIR inspection dumps under `build/agent_state/421_step2_prepared/`
were used to classify the rows by BIR opcode, operand/result type, prepared
fact surface, and likely first owner.

## Classification Rule

Rows are assigned to one first-owner bucket using this priority order:

- F128 rows are quarantined before ordinary non-F128 work.
- Aggregate ABI rows with `sret` or `byval` are producer/ABI-contract owned
  before ordinary scalar call rows.
- Pointer casts and pointer address materialization are separated before
  integer arithmetic buckets.
- Scalar F32/F64 rows stay separate from integer lowering.
- Remaining ordinary non-F128 rows are grouped by the first high-signal BIR
  opcode family visible in prepared BIR.

The counts below are therefore disjoint and sum to 190.

## Owner Buckets

| Bucket | Count | Owning layer | Prepared fact completeness | Representative rows |
| --- | ---: | --- | --- | --- |
| Select and join materialization | 54 | RV64 object lowering | BIR select instructions and prepared join-transfer facts are present; lowering must reject unsupported ordinary selects without inventing branch facts. | `src/pr43236.c`, `src/pr51933.c`, `src/pr68328.c`, `src/pr82954.c`, `src/pr84524.c` |
| Call-adjacent scalar publication and inline-asm materialization | 38 | RV64 call/inline-asm object lowering | Prepared calls are present, but the object route lacks support for these scalar result, argument, or inline-asm fragments. | `src/pr38533.c`, `src/pr40657.c`, `src/pr45695.c`, `src/pr49279.c`, `src/pr56982.c` |
| Pointer cast and address materialization | 21 | RV64 pointer/address lowering, with producer review for pointer provenance | Prepared BIR exposes `inttoptr` or `ptrtoint`; do not paper over missing provenance by guessing RV64 addresses. | `src/930930-1.c`, `src/20000622-1.c`, `src/20010329-1.c`, `src/20011019-1.c`, `src/20041112-1.c` |
| Aggregate `sret`/`byval` call-storage | 19 | Prepared ABI producer plus RV64 aggregate call lowering | ABI attributes are present in BIR, but some rows show suspicious prepared aggregate sizes or alignments; treat incoherent facts as producer-owned. | `src/pr88904.c`, `src/20000917-1.c`, `src/20010224-1.c`, `src/20020206-1.c`, `src/20020506-1.c` |
| Integer div/rem lowering | 17 | RV64 integer instruction lowering | BIR contains scalar `udiv`, `sdiv`, `urem`, or `srem`; classify as object lowering only when operand/result types and prepared homes are coherent. | `src/20021120-3.c`, `src/20030105-1.c`, `src/20090113-2.c`, `src/20090113-3.c`, `src/20100416-1.c` |
| F128 quarantine | 16 | F128/runtime-helper quarantine, not primary route | BIR contains `f128` values or long-double helper calls; keep out of the ordinary non-F128 backlog. | `src/20040709-2.c`, `src/20040709-3.c`, `src/20000910-1.c`, `src/20000910-2.c`, `src/20050203-1.c` |
| Integer arithmetic shift right | 11 | RV64 integer instruction lowering | BIR contains scalar `ashr`; this is distinct from already broader `shl`/`lshr` and must not be handled by testcase-shaped constants. | `src/pr78438.c`, `src/pr79737-2.c`, `src/20000815-1.c`, `src/20020402-3.c`, `src/20051110-1.c` |
| Large literal and materialization | 10 | RV64 immediate/materialization lowering | BIR contains integer constants outside small-immediate paths; lowering needs a semantic literal materialization rule, not filename matching. | `src/20071211-1.c`, `src/pr78726.c`, `src/950221-1.c`, `src/960321-1.c`, `src/960402-1.c` |
| Global memory/addressing residual | 2 | RV64 memory/address lowering | BIR has global load/store rows with prepared memory facts needing separate addressability checks. | `src/pr59387.c`, `src/pr77766.c` |
| Scalar F32/F64 conversion/op residual | 2 | RV64 scalar floating-point lowering | Non-F128 floating rows remain after quarantine; they should not be mixed with F128 runtime work. | `src/ieee/930529-1.c`, `src/ieee/pr72824.c` |

## Representative Evidence

- Select/join: `src/pr43236.c` contains
  `%t13.store0 = bir.select eq i64 %t12, 0, i8 0, %t13.elt0`.
- Call-adjacent scalar/inline asm: `src/pr38533.c` contains
  `%t0 = bir.call i32 llvm.inline_asm(i32 0)`.
- Pointer/address: `src/930930-1.c` contains
  `%t7 = bir.inttoptr i64 %t6 to ptr`.
- Aggregate ABI: `src/pr88904.c` declares
  `bir.func @foo(ptr sret(size=8, align=4) %ret.sret) -> void`.
- Integer div/rem: `src/20021120-3.c` contains
  `%t1 = bir.udiv i32 %p.x, %p.y`.
- F128 quarantine: `src/20040709-2.c` contains
  `bir.load_local f128 %lv.param.p.x.0`.
- Arithmetic shift right: `src/pr78438.c` contains
  `%t1 = bir.ashr i32 %t0, 11`.
- Large literal/materialization: `src/950221-1.c` contains
  `%t0 = bir.ne i64 %p.a, 3735928559`.
- Global memory/addressing: `src/pr59387.c` contains
  `bir.store_global @a, i32 %t0`.
- Scalar F32/F64: `src/ieee/pr72824.c` contains a function parameter
  `float %p.value`.

## Step 3 Ranking Notes

- Highest-impact ordinary non-F128 candidates are select/join materialization
  (54), call-adjacent scalar/inline-asm fragments (38), pointer/address
  materialization (21), aggregate ABI call-storage (19), and integer div/rem
  (17).
- F128 remains quarantined at 16 rows and should not displace ordinary
  non-F128 implementation ideas.
- Producer-owned review is required before implementation ideas for aggregate
  rows with suspicious prepared size/alignment facts and pointer rows where
  prepared provenance is incomplete.
- The ordered Step 3 follow-up backlog is recorded in
  `docs/rv64_gcc_torture_post_contract/followup_idea_plan.md`.
