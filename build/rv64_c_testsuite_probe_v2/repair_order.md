# RV64 Undefined-Main Repair Order

Input classification:

- `build/rv64_c_testsuite_probe_v2/classification.md`
- `build/rv64_c_testsuite_probe_v2/classification.tsv`
- `build/rv64_c_testsuite_probe_v2/classification_work/buckets/`

## Ordering Principle

Repair the shared output contract first, then split the secondary feature
families into focused follow-up initiatives. The classification evidence says
all 93 undefined-main cases preserve `prepared.func @main`, but the existing
RV64 assembly output is `.text` only. That makes
`rv64_text_only_fail_closed` the mandatory first repair: a prepared function
selected for RV64 emission must produce `.globl main`, a `main:` label, and an
executable body instead of a successful empty assembly file.

The secondary buckets should not be merged into one giant c-testsuite repair.
They are separate feature hazards that become diagnosable after the fail-closed
function-emission path is repaired.

## Follow-Up Sequence

| Order | Bucket | Count | Rationale | Suggested scope |
| ---: | --- | ---: | --- | --- |
| 1 | `rv64_text_only_fail_closed` | 93 | Highest impact and lowest ambiguity. It affects every undefined-main case, including the no-storage control `src/00094.c`, so it is not caused only by globals, strings, or calls. | Repair RV64 prepared-function emission so supported prepared functions fail loudly or emit a real symbol and body. Prove with the trivial/control case before exercising feature-heavy cases. |
| 2 | `unused_extern_no_storage` | 1 | Best guard against overfitting feature buckets. This case has no prepared storage/addressing demand and should pass once the basic function body contract is real. | Keep as the minimal canary for Step 1. It should not require global-storage, string, pointer, or call support. |
| 3 | `string_literals_and_extern_calls` | 59 | Largest secondary bucket and broadest user-visible impact, but it mixes string address materialization and direct external calls. It should follow function emission because the current `.text`-only output hides its real failures. | Create a focused idea for string constants, external-call lowering, and libc symbol calls. Keep string address metadata and call ABI proof explicit. |
| 4 | `aggregate_global_storage` | 19 | Second-largest secondary bucket and a clear storage/addressing capability gap. It needs data emission and global-symbol address materialization, but it should not absorb pointer or floating storage details. | Create a focused idea for aggregate/array global data layout, symbol emission, offset accesses, loads, and stores. |
| 5 | `pointer_global_storage` | 8 | Pointer and function-pointer globals add relocation and address-initializer semantics beyond ordinary aggregate storage. The bucket is smaller but riskier than scalar storage. | Create a focused idea for pointer-valued global storage, address initializers, function pointer references, and indirect-address materialization. |
| 6 | `floating_global_storage` | 2 | Low count but distinct representation and lowering risk. Folding it into aggregate or scalar storage would blur integer data emission with floating data semantics. | Create a focused idea for global `double` storage representation and RV64 load/use paths. |
| 7 | `scalar_global_storage` | 2 | Smallest ordinary global-storage bucket and likely a prerequisite-sized slice for validating data-section basics without aggregate offsets. It remains separate because its exact failures can be cleaner than aggregate/pointer cases. | Use as a tight scalar global storage idea or as a proving subset for the broader global-data route if the supervisor chooses to combine only scalar mechanics, not all feature buckets. |
| 8 | `aggregate_or_control_only_shape` | 2 | Lowest immediate signal. These cases have aggregate/control source shape without observed global or string address demand in the prepared evidence, so they are best revisited after the main output contract and concrete feature buckets are repaired. | Keep as a reclassification/checkup packet after earlier repairs, not as an initial implementation target. |

## Boundaries

- Do not treat the full 220-case c-testsuite sweep as the next repair target.
- Do not claim progress by weakening tests, downgrading supported paths, or
  accepting empty `.text` output as a valid unsupported result.
- Do not combine strings/calls, aggregate globals, pointer globals, floating
  globals, scalar globals, and control-only shapes into one implementation
  plan.
- After Step 1, rerun a narrow representative matrix before selecting the next
  feature bucket; some secondary cases may move from undefined-main linker
  failures into more specific compile, link, or runtime failures.

## Step 5 Recommendation

Step 5 should create follow-up idea documents from this ordering. Start with a
single idea for `rv64_text_only_fail_closed` output-contract cleanup. Then add
separate feature ideas for `string_literals_and_extern_calls`,
`aggregate_global_storage`, `pointer_global_storage`,
`floating_global_storage`, and the smaller scalar/control follow-ups as needed.
The first idea should explicitly use `unused_extern_no_storage` as the minimal
control case.
