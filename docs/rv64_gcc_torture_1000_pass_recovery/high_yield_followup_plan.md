# RV64 gcc_torture High-Yield Follow-Up Plan

Status: Step 3 ranked follow-up plan for the `470/1467` RV64 backend scan.

## Evidence Basis

This plan uses the July 8 evidence recorded in `current_scan_summary.md` and
the first-owner classification in `failure_bucket_map.md`:

- `1467` total rows
- `470` passed rows
- `997` failed rows
- `0` missing rows

The goal is not to claim deterministic pass-count gains. The goal is to
generate the smallest durable follow-up queue whose combined breadth can
plausibly move the backend-object route toward `1000+` passing rows without
mixing owners or using testcase-overfit tactics.

## Ranking Model

Candidate follow-up families are ranked by:

1. expected breadth in the current `997` failures;
2. first-owner clarity;
3. producer-before-consumer dependency order;
4. reuse of recent authority/freshness architecture from ideas `587` through
   `600`;
5. proof surface that can show same-family progress beyond one named case.

Each implementation follow-up below is framed as not testcase-overfit because
it targets a classified capability family with many nearby rows, a named owning
layer, and an acceptance surface based on re-running the same family through
the RV64 gcc_torture backend-object route. Representative cases from the
bucket map are examples only, not implementation keys.

## Generate As High-Yield Implementation Ideas

| Rank | Follow-up family | Owning layer | Evidence breadth | Decision |
| ---: | --- | --- | ---: | --- |
| 1 | BIR local-memory producer repair | BIR semantic producer | at least `219` local-memory rows | Generate first |
| 2 | RV64/MIR consumer lowering for prepared fragments | RV64/MIR consumer | `252` rows total, with `145` move/terminator rows first | Generate after producer/authority guardrails |
| 3 | Global data producer/consumer split | prepared/global authority and RV64/global consumer, split into separate ideas | `70` rows | Generate as two or more single-owner ideas |
| 4 | ABI call/result and stack-frame lowering | ABI/RV64 consumer | `60` rows | Generate after local/global memory prerequisites are explicit |
| 5 | Recent-architecture-close prepared/RV64 wiring | prepared/RV64 authority and RV64 consumer, split by authority shape | at least `48` rows plus small tails | Generate as targeted extension ideas |

### Rank 1: BIR Local-Memory Producer Repair

Generate follow-up ideas for BIR local-memory semantics before target-local
work. The visible population is:

- local-memory loads: `82`
- local-memory stores: `56`
- local-memory GEP: `43`
- scalar/local-memory mixed semantics: `26`
- alloca local-memory semantics: `12`

This family is the largest ordinary-C producer blocker. It should be split into
load, store, GEP/address, and alloca/subobject ownership rather than one broad
implementation route. Global initializer bootstrap (`38` aggregate/byte rows
plus `2` string-pool rows) is adjacent but should be a separate producer idea,
because initializer bytes and local frame memory have different authority and
proof surfaces.

Expected breadth: local-memory producer ideas can expose up to `219` currently
blocked rows to prepared/RV64 stages. They will not all turn into passes, but
they are prerequisite to many later target consumers.

Why this is not testcase-overfit: the input evidence is a diagnostic family,
not one case name. A valid implementation must accept multiple load/store/GEP
or alloca rows and preserve nearby unsupported or architecture-bound rows as
classified failures instead of rewriting expectations.

### Rank 2: RV64/MIR Consumer Lowering For Prepared Fragments

Generate RV64/MIR consumer ideas only where upstream prepared authority already
exists or can be proven by the previous producer/authority route. The first
candidate split is:

- out-of-SSA move-bundle target shape: `75`
- unsupported terminator fragment: `70`
- binary/pointer instruction fragments: `42`
- cast fragments: `23`
- other instruction fragments: `32`

The high-yield opening slice should cover move-bundle target materialization
and terminator lowering separately. It must explicitly reject target-local
inference when a source, stack slot, branch operand, or destination authority
is missing.

Expected breadth: `252` rows are RV64/MIR consumer-owned, but the first
implementation ideas should target the `145` move/terminator rows before mixed
instruction tails.

Why this is not testcase-overfit: the repair target is a target consumer for
prepared BIR/MIR fragments across dozens of rows. Acceptance should require
same-family rows to progress while preserving diagnostics for rows whose
producer or prepared authority is still missing.

### Rank 3: Global Data Producer/Consumer Split

Generate global data follow-ups after local-memory producer planning because
the `70` global rows cross a real owner boundary:

- selected global object-data contract: `17`, prepared/global authority
- prepared global memory facts: `12`, prepared/global authority
- direct global-symbol base+offset: `11`, prepared/global authority
- global symbol emission: `17`, RV64/global consumer
- global access width: `13`, RV64/global consumer

This should not become one mixed global idea. The first idea should publish or
complete prepared global object-data and memory facts. A later RV64 idea should
consume already-supported global symbols and access widths.

Expected breadth: `40` prepared/global authority rows and `30` RV64/global
consumer rows.

Why this is not testcase-overfit: the split follows first-owner evidence and
forces producer facts to exist before RV64 emission. Proof should cover
multiple global initializer, symbol, access-width, and base+offset rows.

### Rank 4: ABI Call/Result And Stack-Frame Lowering

Generate an ABI/RV64 idea for ordinary same-module calls, result movement, and
stack-frame consumption once local/global memory prerequisites are clear. The
current bucket is:

- ordinary call ABI/result lowering: `46`
- stack frame layout: `12`
- return move-bundle target: `2`

The first idea should not include variadic/library call policy or runtime
triage. It should define the prepared call/return facts RV64 is allowed to
consume and reject inference from final assembly shape alone.

Expected breadth: `60` rows.

Why this is not testcase-overfit: the follow-up would repair a call, result,
and frame ABI contract visible across many ordinary-C rows. Acceptance should
check several call/result/frame cases and nearby cases that must remain
deferred when their owner is runtime, library, or unsupported policy.

### Rank 5: Recent-Architecture-Close Wiring

Generate smaller targeted ideas where ideas `587` through `600` already
created the authority model and the current logs show missing extension or
wiring:

- local memory frame-slot or pointer base+offset: `27`
- branch stack-load authority/source freshness residuals: `10`
- select publication stack-offset and move-bundle wiring: `11`
- ambiguous move-bundle source freshness: `2`
- scalar compare publication: `3`

These should be single-owner ideas: pointer/local-memory RV64 consumption,
branch stack-source residual audit, select publication source wiring, and
scalar compare publication should not be merged into one implementation route.

Expected breadth: at least `48` rows, plus small publication tails.

Why this is not testcase-overfit: these rows reuse a named architecture
contract already created by the recent idea series. A valid follow-up must
prove that the missing selected authority or target consumption path generalizes
across the same diagnostic vocabulary, not just the representative cases.

## Generate As Research Or Discussion Ideas

### Destination Fan-In Authority

Generate a research/design idea before implementation for the `125`
non-parallel multi-source stack-destination rows. This is the largest prepared
authority bucket, but it is not yet a straightforward extension of selected
source freshness. The unsettled question is destination fan-in: what ordering,
mutual-exclusion, or merge-authority rule makes multiple candidate stack
destinations legal for a non-parallel move bundle?

Expected breadth: `125` rows if the design becomes implementable.

Why this is not testcase-overfit: the immediate artifact should be an
architecture rule and reject signals, not a case-shaped lowering shortcut. It
should explain which rows can later become implementation work and which must
remain rejected until destination authority is explicit.

### Runtime Mismatch Ownership

Generate a runtime mismatch investigation idea, not an implementation idea, for
the `72` runtime mismatches and `3` run timeouts:

- runtime abort: `51`
- runtime segfault: `21`
- runtime timeout: `3`

These rows reached object emission, but exit symptoms do not identify whether
the root owner is ABI, layout, local/global memory, call lowering, or true
runtime support. The first artifact should map runtime failures by wrong
output, abort site, signal, and likely upstream owner after compile-time
blockers shrink.

Expected breadth: `75` rows, but implementation ownership is unresolved.

Why this is not testcase-overfit: the planned work is diagnostic ownership
mapping across runtime rows. It explicitly rejects treating one abort or
segfault case as proof of a runtime fix.

### Pointer/Address Architecture Boundary

Keep pointer/address research available for the `4` direct
`unsupported_pointer_arithmetic` rows and for any overlap discovered while
repairing BIR local-memory GEP or pointer base+offset consumption. This should
be a discussion/research follow-up only if implementation planning cannot
cleanly reuse ideas `597`, `599`, and `600`.

Expected breadth: small direct count now, but it may guard larger local-memory
or global-address implementation routes.

## Defer

Defer families that may matter later but are not on the first `1000+` route:

- memcpy intrinsic semantics: `17`
- memset intrinsic semantics: `14`
- unordered floating compare: `11`
- floating cast: `2`
- string-pool constants: `2`
- direct-call semantic tail: `3`
- small function-signature, scalar cast/binop, and vector tails: `7`
- compile timeout: `9`
- run timeout: `3`, until runtime ownership is mapped
- link failure after object generation: `1`

These are deferred because they are lower breadth, policy-heavy, or likely to
need a clearer first-owner map after local-memory, global data, ABI, and
RV64/MIR consumer blockers move.

## Quarantine

Quarantine inline asm carrier rows (`18`) and any F128-, library-, builtin-, or
environment-dependent rows that would require unsupported-policy changes,
expectation downgrades, allowlist filtering, timeout tuning, or weaker runtime
comparison. They should not be used as progress toward `1000+` unless a later
policy idea explicitly changes the backend route goals.

## Queue Shape For Step 4

Step 4 should generate follow-up ideas in this dependency-aware order:

1. BIR local-memory load/store/GEP/alloca producer repair ideas.
2. BIR global initializer bootstrap producer idea.
3. Destination fan-in authority research/design idea.
4. Prepared global data authority idea.
5. RV64 global data consumer idea.
6. RV64/MIR move-bundle target materialization idea.
7. RV64/MIR terminator and broad instruction-fragment consumer ideas.
8. ABI/RV64 call, result, return, and stack-frame idea.
9. Recent-architecture-close wiring ideas for pointer local-memory, branch
   stack-source residuals, select publication wiring, and scalar compare
   publication.
10. Runtime mismatch ownership investigation idea.

This queue can plausibly move from `470` toward `1000+` because the high-yield
families cover hundreds of current failures before deferred or quarantined
policy lanes are needed. It still preserves producer-before-consumer ordering:
BIR and prepared authority work must run before RV64 target consumers where
the consumer would otherwise have to infer missing semantic authority.
