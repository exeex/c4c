# RV64 gcc_torture Failure Bucket Map

Status: Step 2 classification for the `470/1467` RV64 gcc_torture backend
scan.

## Evidence And Method

This map uses the current July 8 row artifacts documented in
`current_scan_summary.md`:

- `1467` total rows
- `470` passed rows
- `997` failed rows
- `0` missing rows

The first-owner buckets below were derived from
`build/agent_state/rv64_gcc_c_torture_backend_failed.txt` and the matching
per-case logs under `build/rv64_gcc_c_torture_backend/<case-id>/case.log`.
The classifier used the first stopping diagnostic, not the testcase name alone.
Representative cases are examples of the diagnostic family, not claimed
implementation targets.

## First-Owner Rollup

| First owner | Count | Capability family |
| --- | ---: | --- |
| BIR semantic producer | 311 | `lir_to_bir` semantic lowering stops before prepared handoff |
| RV64/MIR consumer | 252 | prepared module is present, but RV64 object lowering lacks a consumer for a BIR/MIR fragment or move shape |
| prepared/prealloc authority | 146 | move-bundle source/destination authority is ambiguous or incomplete before target consumption |
| runtime | 72 | object emits and links but c4c runtime behavior aborts, segfaults, or mismatches clang |
| ABI/RV64 consumer | 60 | call, return, or stack-frame ABI shape is not accepted by RV64 object route |
| prepared/RV64 authority | 48 | shared authority exists or is close, but an RV64-side use still needs selected source wiring |
| prepared/global authority | 40 | global memory or selected object-data authority is missing before RV64 emission |
| RV64/global consumer | 30 | RV64 global-symbol or global-access-width emission gap |
| policy/unsupported | 18 | inline asm carrier fragments intentionally unsupported for this route |
| timeout/policy | 9 | c4c object compile timeout |
| architecture research | 4 | pointer arithmetic selected-authority question remains unsettled |
| runtime/policy | 3 | emitted object run timeout |
| prepared authority | 3 | scalar compare publication authority missing |
| runtime/link | 1 | link failure after object generation |
| **Total** | **997** | current failed population |

## BIR Semantic Producer Buckets

These `311` rows stop before prepared object handoff, so their first owner is
semantic `lir_to_bir` production rather than RV64 target lowering.

| Capability family | Count | Representative case | Notes |
| --- | ---: | --- | --- |
| local-memory load semantics | 82 | `src/20041124-1.c` | Largest BIR producer gap; many cases fail in `load local-memory semantic family`. |
| local-memory store semantics | 56 | `src/20010605-2.c` | Same owner as load, but store-source and address authority should be split from load repair. |
| local-memory GEP semantics | 43 | `src/ieee/pr72824-2.c` | High-yield ordinary-C path, but should respect the pointer/address research boundary. |
| global initializer bootstrap | 38 | `src/20040302-1.c` | Bootstrap accepts only selected global initializer shapes; aggregate and byte-address cases need producer work. |
| scalar/local-memory mixed semantics | 26 | `src/20020411-1.c` | Likely overlap with local-memory producer coverage, but the diagnostic is broader than load/store/GEP. |
| memcpy runtime/intrinsic semantics | 17 | `src/pr79354.c` | Explicit intrinsic family; policy-heavy unless it unlocks ordinary-C clusters. |
| memset runtime/intrinsic semantics | 14 | `src/pr30778.c` | Same lane as memcpy; useful but not first unless counts grow after local-memory repair. |
| alloca local-memory semantics | 12 | `src/20180921-1.c` | Local stack allocation producer gap. |
| unordered floating compare semantics | 11 | `src/ieee/compare-fp-1.c` | Floating-point semantic lane, lower priority for the first `1000+` route. |
| string-pool global initializer bootstrap | 2 | `src/20010325-1.c` | Small global/string constant producer gap. |
| direct-call semantics | 3 | `src/complex-1.c` | Small semantic call family tail. |
| function signature, scalar cast/binop, vector binop | 7 | `src/20071029-1.c`, `src/20050316-3.c`, `src/20050604-1.c`, `src/20080502-1.c` | Low-count semantic tails; vector and unusual scalar forms should not drive the first route. |

Largest current BIR producer gap: local-memory semantics (`82 + 56 + 43 + 26 +
12 = 219` rows before counting global initializers). This is the clearest
shared-producer candidate, but it should be split into load/store/GEP/alloca
ownership instead of one mixed implementation idea.

## RV64/MIR Consumer Buckets

These `252` rows reach a prepared module and then fail in RV64 object lowering
for unsupported BIR/MIR fragments or target move shapes.

| Capability family | Count | Representative case | Notes |
| --- | ---: | --- | --- |
| out-of-SSA move-bundle target shape | 75 | `src/20020206-2.c` | Broad RV64 consumer gap. Some rows already carry `out_of_ssa_parallel_copy` authority but still lack target materialization. |
| unsupported terminator fragment | 70 | `src/20030910-1.c` | Branch/terminator lowering gap. Must be separated from branch stack-source freshness rows that already have selected-authority contracts. |
| unsupported instruction fragment, binary/pointer op | 42 | `src/20040709-2.c` | Likely includes integer-width, pointer, and unsupported binary forms. First owner is RV64/MIR consumer after prepared handoff. |
| unsupported instruction fragment, other | 32 | `src/20030221-1.c` | Mixed instruction tail; needs sub-bucketing before implementation. |
| unsupported instruction fragment, cast | 23 | `src/20010604-1.c` | RV64 cast lowering gap distinct from BIR semantic cast producer failures. |
| unsupported instruction fragment, select | 6 | `src/20000815-1.c` | Small select consumer lane; related to, but not identical with, select-carrier freshness authority. |
| floating cast | 2 | `src/20040709-1.c` | Low priority unless needed by non-F128 ordinary-C coverage. |
| select publication large immediate | 2 | `src/pr29695-1.c` | Architecture mostly exists; target immediate materialization/range handling is the visible blocker. |

Largest current RV64/MIR consumer gap: move-bundle target shapes plus
terminator fragments (`145` rows). This is high-yield, but it must not bypass
prepared authority checks when a move source, stack slot, or branch operand is
not selected.

## Prepared And Prealloc Authority Buckets

These buckets are close to the architecture from ideas `587` through `600`
because their diagnostics already speak in selected freshness, publication,
branch stack-source, pointer base-plus-offset, or move-bundle authority terms.

| Capability family | Count | Representative case | First owner | Recent architecture context |
| --- | ---: | --- | --- | --- |
| non-parallel multi-source stack destination | 125 | `src/pr43236.c` | prepared/prealloc authority | Ideas `587` and `588` made move-bundle source freshness queryable, but this family is destination fan-in authority, not merely source freshness. It needs an explicit ordering or mutually-exclusive authority rule. |
| local memory frame-slot or pointer base+offset | 27 | `src/20000722-1.c` | prepared/RV64 authority | Ideas `599` and `600` define selected pointer base-plus-offset and pointer-value memory-use authority. These rows look close if the missing part is wiring selected authority into the RV64 local-memory consumer. |
| generic move-bundle target shape | 16 | `src/20000422-1.c` | prepared/prealloc authority | Mixed generic target-shape failures; needs split between producer authority and RV64 materialization. |
| branch stack-load authority | 7 | `src/20080519-1.c` | prepared/RV64 authority | Ideas `590`, `592`, `593`, `594`, and `596` closed narrow branch stack-source contracts for selected pointer `Lhs`/`Rhs`. Remaining rows likely need exact-role audit rather than another broad branch fix. |
| select publication stack-offset source wiring | 7 | `src/20000706-1.c` | prepared/RV64 authority | Ideas `598` and `589` settled alias/source freshness boundaries; these rows show published select evidence but reject `unsupported_source_stack_offset`. |
| select publication move-bundle wiring | 4 | `src/pr45034.c` | prepared/RV64 authority | Close to the select-carrier and edge-publication contracts, but still needs a single owner for source freshness vs target consumption. |
| branch stack-load source freshness | 3 | `src/930930-1.c` | prepared/RV64 authority | Directly names source freshness; should be audited against the closed `593/594/596` branch-stack-source queue. |
| scalar compare publication | 3 | `src/20011217-1.c` | prepared authority | Small prepared publication gap; likely adjacent to branch/compare consumers. |
| malformed join transfer carrier | 3 | `src/20000818-1.c` | prepared/prealloc authority | Join-transfer carrier completeness gap. |
| ambiguous move-bundle source freshness | 2 | `src/930123-1.c` | prepared/prealloc authority | Already in the `587` vocabulary; likely close if ambiguity can be resolved semantically. |

Recent-architecture-close buckets with the best leverage are:

- local memory frame-slot or pointer base+offset (`27` rows), because ideas
  `599` and `600` already define pointer/address freshness boundaries;
- branch stack-load authority/source freshness (`10` rows), because ideas
  `590`, `592`, `593`, `594`, and `596` already closed selected branch
  stack-source producer and RV64 consumer paths for narrow pointer cases;
- select publication stack-offset and move-bundle wiring (`11` rows), because
  ideas `589` and `598` already separate source freshness, alias evidence, and
  destination legality;
- ambiguous move-bundle source freshness (`2` rows), because idea `587` made
  selected freshness a first-class query.

The `125` non-parallel multi-source stack-destination rows are larger, but
they expose an architecture weak point rather than a simple extension of the
closed source-freshness work: destination fan-in needs an ordering,
mutual-exclusion, or merge-authority model before a durable implementation
idea should be opened.

## Global Data Buckets

| Capability family | Count | Representative case | First owner | Notes |
| --- | ---: | --- | --- | --- |
| selected global object-data contract | 17 | `src/20010924-1.c` | prepared/global authority | Prepared selected object-data reports `unsupported_but_coherent`; producer contract exists but does not emit bytes. |
| global symbol emission | 17 | `src/20020118-1.c` | RV64/global consumer | RV64 object route cannot emit the prepared global symbol. |
| global access width | 13 | `src/ieee/fp-cmp-2.c` | RV64/global consumer | RV64 supports only 1-, 2-, 4-, and 8-byte prepared global accesses. |
| prepared global memory facts | 12 | `src/strlen-7.c` | prepared/global authority | Missing supported prepared global memory facts. |
| direct global-symbol base+offset | 11 | `src/pr79737-2.c` | prepared/global authority | Prepared direct global-symbol base-plus-offset memory addressing gap. |

Global data accounts for `70` rows when prepared authority and RV64 global
consumption are combined. This is likely a high-yield ordinary-C lane after
local-memory producer work, but it should be split by producer authority
versus target emission.

## ABI, Runtime, Unsupported, And Policy Buckets

| Capability family | Count | Representative case | First owner | Notes |
| --- | ---: | --- | --- | --- |
| ordinary call ABI/result lowering | 46 | `src/20000603-1.c` | ABI/RV64 consumer | Calls reach prepared shape but ordinary same-module call ABI/result lowering is unsupported. |
| stack frame layout | 12 | `src/20020314-1.c` | ABI/RV64 consumer | Requires supported prepared stack frame. |
| return move-bundle target | 2 | `src/20001130-2.c` | ABI/RV64 consumer | Return ABI move materialization tail. |
| runtime abort | 51 | `src/pr38533.c` | runtime | c4c object runs but aborts where clang exits `0`. Needs runtime-family triage before implementation. |
| runtime segfault | 21 | `src/20000706-5.c` | runtime | c4c object segfaults where clang exits `0`; likely mixed miscompile/ABI/layout causes. |
| runtime timeout | 3 | `src/20000224-1.c` | runtime/policy | Object route emits but execution exceeds the 20s harness limit. |
| link failure | 1 | `src/vfprintf-1.c` | runtime/link | Single post-object link failure. |
| inline asm carrier | 18 | `src/pr40022.c` | policy/unsupported | Unsupported inline asm fragments; quarantine unless fresh evidence shows broad ordinary-C leverage. |
| object compile timeout | 9 | `src/930106-1.c` | timeout/policy | Compile-time policy/performance lane, not semantic progress by itself. |

Runtime rows are acceptance-critical because they are past object generation,
but the current logs only show abort, segfault, or timeout symptoms. They need
a separate runtime mismatch family map before choosing implementation owners.

## Architecture Weak Points Needing Research Or Discussion

- Destination fan-in authority: `125` non-parallel multi-source
  stack-destination rows are too large to ignore, but current selected
  freshness work mostly proves source validity. These rows need a rule for
  ordering, exclusivity, or merge authority before target materialization.
- Pointer/address semantics beyond the closed narrow contracts: `4`
  `unsupported_pointer_arithmetic` rows and the `27` local-memory
  frame-slot/pointer base+offset rows should be checked against ideas `597`,
  `599`, and `600` before opening target-local fixes. Range, layout, and target
  encodability are support facts, not semantic freshness.
- Select publication and alias/source ownership: `11` rows look close after
  ideas `589` and `598`, but stack-offset and move-bundle publication must
  preserve the rule that alias evidence or destination legality is not source
  freshness.
- Branch stack-source completeness: `10` branch stack-load authority/freshness
  rows remain after ideas `590`, `592`, `593`, `594`, and `596`. These need an
  exact audit of whether they are new roles, aggregate-adjacent shapes, or
  missing RV64 consumption of already selected authority.
- Runtime mismatch ownership: `72` runtime mismatches and `3` runtime timeouts
  may be ABI, layout, memory, call, or true runtime support failures. The first
  owner cannot be assigned from exit symptom alone.

## Deferred Or Quarantined Lanes

- Inline asm carrier fragments (`18`) are unsupported and should remain
  quarantined unless a future policy decision makes inline asm a goal for this
  backend route.
- Floating-point-heavy rows, including unordered float compare (`11`) and
  floating cast (`2`), should not lead the first `1000+` route unless later
  classification shows they unblock broad non-F128 ordinary-C coverage.
- String/library and intrinsic families, including `memcpy` (`17`), `memset`
  (`14`), string-pool constants (`2`), and the `vfprintf-1.c` link failure
  (`1`), should be deferred behind local-memory, global-data, ABI, and
  prepared-authority families with clearer first ownership.
- Timeout rows (`9` compile, `3` run) should not be counted as compiler
  capability progress without separate performance or harness-policy review.

## Step 3 Inputs

The high-yield follow-up plan should consider these as the first candidate
families:

1. BIR local-memory producer repair, split into load/store/GEP/alloca and
   global initializer bootstrap, with an estimated visible population of at
   least `219` local-memory rows plus `40` initializer rows.
2. RV64/MIR consumer repair for move-bundle target shapes and terminator or
   instruction fragments, with `252` rows total but requiring prepared
   authority guardrails.
3. Prepared destination fan-in authority research or design, covering `125`
   non-parallel multi-source stack-destination rows.
4. Global data producer/consumer split, covering `70` rows across prepared
   global authority and RV64 global consumption.
5. ABI/RV64 call, return, and stack-frame lowering, covering `60` rows.
6. Recent-architecture-close wiring from ideas `587` through `600`, especially
   pointer local-memory authority, branch stack-source residuals, and select
   publication wiring, covering at least `48` rows plus adjacent small tails.
7. Runtime mismatch investigation, covering `72` mismatches and `3` runtime
   timeouts, only after enough compile-time blockers are cleared to avoid
   mixed-owner debugging.

This map does not claim implementation progress. It only classifies the
current `997` RV64 gcc_torture backend failures by first owner and capability
family for Step 3 planning.
