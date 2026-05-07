# HIR Static Member Carrier Authority Decomposition Runbook

Status: Active
Source Idea: ideas/open/148_hir_static_member_carrier_authority_decomposition.md
Supersedes Active Runbook: ideas/open/147_rendered_qualified_compatibility_bridge_removal.md remains open; this runbook decomposes its blocked Step 4 HIR static-member carrier-policy problem.

## Purpose

Split the blocked HIR static-member carrier-policy problem into focused probes
before returning to idea 147 Step 4 bridge deletion.

Goal: HIR static-member owner authority must come from structured carriers, not
rendered `expr->name` splitting, while preserving dependent trait,
instantiated, inherited, and member-typedef static-member folding.

## Core Rule

Do not use rendered `A::B::member` spelling as owner authority. Member identity
may use `unqualified_text_id`, `unqualified_name`, or generated payload metadata
only after structured owner authority has been established.

## Read First

- Source idea: `ideas/open/148_hir_static_member_carrier_authority_decomposition.md`
- Blocked parent idea: `ideas/open/147_rendered_qualified_compatibility_bridge_removal.md`
- Route review: `review/147_step4_post_hir_route_review.md`
- Failed proof log from the rejected Step 4 HIR attempt: `test_after.log`

## Current Scope

- HIR static-member named-constant lowering in
  `src/frontend/hir/hir_types.cpp`
- HIR scalar `NK_VAR` static-member lowering in
  `src/frontend/hir/impl/expr/scalar_control.cpp`
- Focused HIR lookup probes in `tests/frontend/frontend_hir_lookup_tests.cpp`
- Existing dependent trait/member-typedef positive cases that regressed in the
  rejected Step 4 route

## Non-Goals

- Do not close or rewrite idea 147.
- Do not delete parser/Sema bridge helpers as part of this decomposition.
- Do not broaden into parser, Sema, or backend rewrites unless a focused HIR
  probe proves that dependency.
- Do not rewrite expectations, mark cases unsupported, or weaken dependent
  trait/member-typedef contracts.
- Do not treat display spelling cleanup as static-member carrier progress.

## Working Model

1. Baseline the blocked failure family from the rejected Step 4 HIR attempt.
2. Add focused HIR probes that separate static-member owner authority from
   member identity.
3. Split primitive trait template-static evaluation from instantiated and
   inherited static-member map behavior.
4. Preserve generated/member-typedef payload folding without rendered owner
   splits.
5. Hand the proved carrier policy back to idea 147 Step 4.

## Execution Rules

- Use `rg` first to inspect static-member lowering routes and existing focused
  tests.
- Add one focused probe per carrier seam before larger HIR repair.
- Treat testcase-shaped matching, expectation rewrites, or unsupported markers
  as route failures.
- Keep owner authority and member identity decisions separate in both tests and
  implementation packets.
- For code-changing packets, get fresh build proof plus the supervisor-chosen
  focused HIR/Sema subset.
- Escalate to idea 147 review only after this runbook defines a proved handoff
  back to Step 4.

## Steps

### Step 1: Establish The Blocked HIR Static-Member Baseline

Goal: make the failure family and candidate seams explicit before more HIR
patching.

Primary targets:
- `review/147_step4_post_hir_route_review.md`
- `test_after.log`
- `src/frontend/hir/hir_types.cpp`
- `src/frontend/hir/impl/expr/scalar_control.cpp`

Actions:
- Confirm the five known regressing tests from the rejected route.
- Inventory the static-member owner/member carriers used by each HIR route.
- Record the first focused probe packet in `todo.md`.

Completion check:
- `todo.md` identifies the blocked failure family, separates the visible
  carrier seams, and names the first probe target.

### Step 2: Probe Member Identity Versus Owner Authority

Goal: prove stale member text/name behavior without letting rendered owner
spelling provide authority.

Primary targets:
- `tests/frontend/frontend_hir_lookup_tests.cpp`
- HIR `NK_VAR` static-member fixtures
- named-constant folding fixtures if needed

Actions:
- Add focused probes for stale `unqualified_text_id`,
  stale `unqualified_name`, and structured qualifier segment carriers.
- Require structured owner authority before any member-name fallback can
  resolve.
- Keep probes small enough that a failed assertion identifies one carrier seam.

Completion check:
- Focused HIR tests fail on rendered owner authority but pass when structured
  owner metadata plus valid member identity is present.

### Step 3: Split Primitive Trait Evaluation From Instantiated Maps

Goal: keep primitive trait template-static folding independent from stale
instantiated static-member maps.

Primary targets:
- primitive trait static-member evaluation
- instantiated template static-member lookup
- inherited static-member lookup

Actions:
- Identify where primitive trait values should fold before probing stale
  instantiated maps.
- Preserve structured owner identity for instantiated and inherited owners.
- Keep the known trait runtime positives green without rendered owner splits.

Completion check:
- Trait folding and instantiated/inherited static-member lookup have separate
  probes and no route needs rendered owner spelling to pass.

### Step 4: Preserve Generated And Member-Typedef Payload Folding

Goal: keep generated/member-typedef payload static-member folding working
through structured carriers.

Primary targets:
- generated typedef payload folding
- dependent member-typedef static-member cases
- inherited member-typedef static-member cases

Actions:
- Add or tighten focused probes for generated/member-typedef payload carriers.
- Repair the HIR carrier path one seam at a time.
- Keep the known dependent and inherited member-typedef positives green.

Completion check:
- Generated/member-typedef folding remains green, and owner authority still
  comes from structured metadata.

### Step 5: Hand Back To Idea 147 Step 4

Goal: return to bridge deletion only after the HIR static-member carrier policy
is explicit and proved.

Primary targets:
- `ideas/open/147_rendered_qualified_compatibility_bridge_removal.md`
- Step 4 HIR static-member routes identified by review

Actions:
- Summarize the proved carrier policy in `todo.md`.
- Identify the exact remaining idea 147 Step 4 bridge-deletion packet.
- Request supervisor/plan-owner switch back to idea 147 when the decomposition
  acceptance criteria are met.

Completion check:
- Focused HIR/Sema proof is green, the carrier-policy route is reviewer-ready,
  and the next Step 4 bridge-removal packet no longer has unresolved
  static-member carrier ambiguity.
