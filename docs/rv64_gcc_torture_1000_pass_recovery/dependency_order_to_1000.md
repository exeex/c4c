# Dependency Order Toward 1000+ RV64 gcc_torture Passes

Status: Step 5 handoff for the `470/1467` RV64 backend scan.

This document turns the generated follow-up idea queue, `602` through `618`,
into an activation order. The ordering is producer-before-consumer: BIR and
prepared authority work must publish facts before RV64 target consumers are
allowed to rely on them, and research-only gates must settle architecture
questions before implementation ideas use those decisions.

The first `1000+` route should not chase every current failure. It should move
through the high-yield ordinary-C blockers first, keep deferred families
outside the route, and keep quarantined policy lanes from being counted as
compiler/backend progress.

## Recommended Activation Order

| Order | Idea | Role in the route | Must run before |
| ---: | --- | --- | --- |
| 1 | `602` BIR local-memory load semantics | Publishes load facts for the largest local-memory producer family. | RV64 local-memory consumers and mixed local-memory follow-ups that need load facts. |
| 2 | `603` BIR local-memory store semantics | Publishes store-source and local-frame write facts. | Later local-memory consumers and ABI/frame work that must not infer store facts. |
| 3 | `604` BIR local-memory GEP and address semantics | Publishes local address/GEP facts while preserving the pointer/address boundary from ideas `597`, `599`, and `600`. | `614` and any RV64 pointer/local-memory consumption route. |
| 4 | `605` BIR local-memory alloca and scalar semantics | Clears alloca and scalar/local-memory producer stops left after load/store/GEP splits. | ABI stack-frame and RV64 local-memory consumers when they depend on local allocation facts. |
| 5 | `606` BIR global initializer bootstrap | Publishes initializer bytes and aggregate initializer facts before prepared/global authority consumes them. | `608` and all later global data consumers. |
| 6 | `607` destination fan-in authority research | Defines or rejects the authority rule for non-parallel multi-source stack destinations. | Any implementation for the `125` destination fan-in rows and any RV64 move-bundle work that would otherwise guess destinations. |
| 7 | `608` prepared global data authority | Publishes selected global object-data, memory facts, and direct global-symbol base+offset authority. | `609`. |
| 8 | `609` RV64 global data consumer | Emits prepared global symbols and supported global access widths. | Later ABI/runtime triage that would otherwise be polluted by missing global emission. |
| 9 | `610` RV64 move-bundle target materialization | Consumes already-authorized move-bundle target shapes. | ABI return-move tails and mixed RV64 instruction work that relies on move materialization. |
| 10 | `611` RV64 terminator fragment lowering | Consumes prepared terminator fragments while preserving branch-source authority guardrails. | Broad instruction-fragment work and runtime triage that needs branch lowering noise reduced. |
| 11 | `612` RV64 instruction fragment consumers | Sub-buckets binary/pointer, cast, and other instruction fragments before implementing target consumers. | ABI/runtime follow-up when ordinary instruction noise blocks attribution. |
| 12 | `613` ABI call, result, and stack-frame lowering | Consumes prepared call, result, return, and frame facts. | `618` runtime mismatch ownership mapping and any later runtime implementation ideas. |
| 13 | `614` RV64 pointer local-memory consumption | Consumes selected pointer base+offset and pointer-value authority from the recent architecture series. | Runtime triage and any pointer/local-memory target route depending on selected authority. |
| 14 | `615` branch stack-source residual audit and repair | Audits residual branch-source rows before narrow repair. | Further branch or terminator tail work. |
| 15 | `616` select publication source wiring | Wires select publication sources while preserving alias/source/destination boundaries. | Later select or move-bundle tails that need source freshness. |
| 16 | `617` scalar compare publication | Handles the small prepared compare-publication tail. | Later compare/branch consumers only if this family remains visible. |
| 17 | `618` runtime mismatch ownership investigation | Maps abort, segfault, wrong-output, and timeout rows to likely first owners. | Any runtime, ABI, layout, memory, or call implementation generated from runtime symptoms. |

## Producer-Before-Consumer Gates

The local-memory producer series, `602` through `605`, is the front of the
route because it covers at least `219` current local-memory rows and exposes
later prepared/RV64 failures without forcing the target to infer missing BIR
facts. `604` is the required gate before `614`: RV64 pointer/local-memory
consumption can only consume selected authority after BIR GEP/address facts
exist or the row is proven to have an upstream authority source already.

The global route is split the same way. `606` must run before `608`, because
prepared/global authority cannot publish coherent object-data and memory facts
for initializer shapes the BIR producer still rejects. `608` must then run
before `609`, because RV64 global emission must consume prepared global facts
rather than reconstructing object data from the target side.

Move-bundle and terminator consumers, `610` and `611`, may run after the
relevant prepared authority exists, but they must not absorb destination
fan-in rows. The `607` destination fan-in research gate decides the authority
rule for the `125` non-parallel multi-source stack-destination rows. Until
that gate closes with an implementation-ready rule, those rows stay out of
`610`.

ABI work in `613` should run after local/global producers and the main RV64
fragment consumers are explicit. That keeps call/result/frame lowering from
becoming a mixed-owner patch that hides missing memory, global, or
move-bundle facts.

## Ideas That Must Run Before RV64 Consumers

These producer or authority ideas are prerequisites for RV64 target consumers:

- `602`, `603`, `604`, and `605` must precede RV64 local-memory consumption
  when rows lack BIR load, store, GEP/address, alloca, or scalar/local-memory
  facts.
- `606` and `608` must precede `609`; the RV64 global consumer must rely on
  prepared global facts, not target-side reconstruction.
- `607` must precede implementation for destination fan-in rows and must guard
  `610` from guessing stack destinations.
- `604` plus the existing pointer/address authority from ideas `599` and
  `600` must precede `614` where the row needs selected pointer base+offset or
  pointer-value memory-use authority.
- `615`, `616`, and `617` should not be folded into broad RV64 consumers; each
  carries a prepared-authority or publication boundary that has to stay
  observable in proof.

## First 1000+ Route

The first route toward `1000+` should activate:

1. `602` through `606` to reduce the largest BIR producer blockers and expose
   better downstream owner evidence.
2. `608` and `609` to handle the `70` global data rows through the
   prepared/global then RV64/global split.
3. `610`, `611`, and sub-bucketed `612` to address the highest-count
   RV64/MIR consumer rows without violating authority guardrails.
4. `613` once memory/global and target-fragment prerequisites are clear.
5. `614` through `617` as close wiring tails where the recent architecture
   contracts already define the authority model.

`607` should be activated early as research because it controls a large `125`
row family, but destination fan-in implementation should not be counted in the
first route until the rule is documented. `618` should run after enough
compile-time blockers have moved that runtime symptoms can be mapped to real
owners instead of mixed upstream noise.

## Deferred Families

Deferred families are outside the first `1000+` route unless later evidence
changes their owner clarity or breadth:

- memcpy and memset intrinsic semantics.
- unordered floating compare and floating cast rows.
- string-pool constants, direct-call semantic tails, small scalar/vector tails,
  and the single post-object link failure.
- compile timeouts and run timeouts until performance, harness policy, or
  runtime ownership is explicitly investigated.

These lanes are deferred because they are lower breadth, policy-heavy, or
likely to become clearer after local-memory, global data, ABI, and RV64/MIR
consumer blockers shrink.

## Quarantined Lanes

Quarantined lanes must not be used to claim progress toward `1000+` in this
umbrella route:

- inline asm carrier fragments;
- F128-, library-, builtin-, or environment-dependent rows;
- any lane requiring expectation downgrades, unsupported-marker changes,
  allowlist filtering, timeout tuning, weaker runtime comparison, or pass/fail
  accounting changes.

Changing those policies may become a future source idea, but it is not part
of this activation order.

## Research And Discussion Gates

`607` is the required destination fan-in gate. It must produce the documented
authority rule, or explicitly explain why the `125` rows remain blocked, before
any implementation chooses between multiple stack destinations.

`618` is the required runtime mismatch gate. The current `72` runtime
mismatches plus `3` run timeouts are symptoms, not enough evidence for a
single implementation owner. The investigation should map abort, segfault,
wrong-output, and timeout rows to ABI, layout, memory, call, or true runtime
support owners before follow-up implementation ideas are opened.

The pointer/address boundary remains a standing discussion gate for `604` and
`614`. Direct unsupported pointer arithmetic must not be smuggled into either
BIR GEP production or RV64 local-memory consumption unless a later idea makes
that policy explicit.
