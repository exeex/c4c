Status: Active
Source Idea Path: ideas/open/310_rv64_c_testsuite_qemu_nonzero_runtime_triage.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Create Focused Follow-Up Ideas

# Current Packet

## Just Finished

Step 5: Rank Repair Families converted the Step 4 runtime buckets into an
ordered repair recommendation list. Ranking favors focused semantic backend
repairs over broad policy/linkage work, so the largest bucket is not first.

| Order | Bucket | Impact | Confidence | Implementation risk | Representative cases | Rationale |
| ---: | --- | ---: | --- | --- | --- | --- |
| 1 | `incomplete_control_or_expression_emission` | 23 | High | Medium | `src/00006.c`, `src/00008.c`, `src/00030.c`, `src/00105.c`, `src/00143.c` | Best first fix: many rows stop in ordinary loop, condition, arithmetic, call-result, branch, epilogue, or `ret` emission. This is core RV64 lowering, has clear assembly tails, and should improve nearby non-runtime emit paths without depending on libc policy. |
| 2 | `stack_pointer_local_slot_materialization` | 21 | High | Medium-high | `src/00005.c`, `src/00019.c`, `src/00032.c`, `src/00072.c`, `src/00130.c` | Nearly the same yield as control/expression emission and evidence is strong, but pointer/array/aggregate/function-pointer address materialization has higher aliasing and ABI risk. Take after the control tail because it likely shares final value/branch completion paths but needs more careful semantic proofs. |
| 3 | `wide_or_narrow_scalar_storage_lowering` | 7 | Medium-high | Medium | `src/00081.c`, `src/00082.c`, `src/00086.c`, `src/00104.c`, `src/00135.c` | Lower impact, but a compact and focused scalar-storage family. A fix here should be easier to scope than globals or externals and can prove integer width/sign/slot handling before broader aggregate work. |
| 4 | `unmaterialized_value_or_comparison_result` | 5 | Medium-high | Medium | `src/00042.c`, `src/00064.c`, `src/00066.c`, `src/00112.c`, `src/00113.c` | Small count but high diagnostic value. It directly exposes missing value/comparison materialization, including the status `112` scratch-return exemplar, and should be repaired semantically rather than patched per test. |
| 5 | `global_storage_or_global_address_flow` | 8 | Medium | Medium-high | `src/00033.c`, `src/00051.c`, `src/00090.c`, `src/00147.c`, `src/00151.c` | More rows than the scalar/comparison buckets, but the mechanism overlaps global object addressing, aggregate reads, switches, and control completion. Do after local/control value paths are stronger so a follow-up can isolate true global-address defects from generic unfinished lowering. |
| 6 | `unresolved_external_or_empty_stub_emission` | 59 | High | High | `src/00025.c`, `src/00056.c`, `src/00125.c`, `src/00179.c`, `src/00220.c` | Highest raw count but not the best first implementation target. Bodyless libc/libm/string/extern labels are a real mechanism, yet fixing them likely needs a runtime/linker/builtin policy decision, external-call ABI behavior, and negative tests for unsupported library calls. Start by defining policy or a separate follow-up idea, not by adding testcase-shaped stubs. |

Suggested repair ordering:

1. Complete ordinary control/expression emission tails.
2. Repair local stack slot and address materialization.
3. Tighten scalar width storage/load/compare lowering.
4. Materialize missing value/comparison results.
5. Revisit global storage/address flow after local value paths are stronger.
6. Treat external empty stubs as a separate policy/runtime initiative despite
   the 59-row impact.

No bucket should be treated as proof by expectation rewrite. In particular, the
59-row external family should not be converted into blanket unsupported tests or
named fake libc bodies; it needs an explicit semantic policy before
implementation.

## Suggested Next

Start Step 6 from `plan.md`: create focused follow-up ideas from the ranked
families. The first follow-up should target ordinary RV64 control/expression
completion; a second can target local stack/address materialization. The
external empty-stub family should become a separate policy/runtime idea rather
than the first backend repair packet.

## Watchouts

This active idea remains triage-only. Step 6 should create follow-up ideas, not
implementation patches. The ranking intentionally prefers semantic backend
families with focused proof surfaces over the largest external/stub count.
Several external rows also exercise arrays, loops, or pointers, so fixing
empty-stub emission first may hide rather than repair core RV64 lowering
defects.

## Proof

No build or CTest proof required for this triage-only ranking update. Proof was
read-only inspection of the Step 4 bucket summary in this file plus
`build/rv64_c_testsuite_probe_latest/triage_step4/summary.md` and
`build/rv64_c_testsuite_probe_latest/triage_step4/classification.tsv`. The
classification artifact still provides the impact counts used for ranking:
`59`, `23`, `21`, `8`, `7`, and `5` across 123 runtime-failure rows. No
`test_after.log` was produced because the delegated proof explicitly did not
require build/ctest proof.
