# Dependency And Priority Order

Status: Step 6 complete

## Purpose

This document orders the follow-up ideas and deferred families from the
umbrella by first owning layer and dependency pressure. It is an activation
handoff, not an implementation plan, and it does not reopen any closed
freshness, branch-stack, target-consumption, expectation, or diagnostic route.

## Ordering Rules

- Activate the first owner before downstream consumers. Semantic-model work
  comes before MIR view contracts that would expose those semantics, and
  shared authority comes before target-local consume-side migration.
- Prefer narrow source ideas over broad remaining-family umbrellas.
- Treat closed ideas 587, 588, 589, 590, 592, 593, 594, and 596 as evidence,
  not as active queues to duplicate.
- Treat idea 591 as the existing Prepared MIR view contract research line,
  not as a substitute owner for producer publication, consumer authority,
  pointer/address semantics, or target migration.

## Recommended Execution Order

| Order | Follow-up or family | First owning layer | Dependency pressure | Activation decision |
| --- | --- | --- | --- | --- |
| 1 | `ideas/open/597_pointer_address_semantic_model_research.md` | Pointer/address semantic model | High: later MIR view exposure and target consumers must not invent pointer/address authority locally | Activate immediately after this umbrella closes |
| 2 | `ideas/open/598_select_carrier_alias_freshness_contract.md` | Shared-prealloc consumer authority | Medium: narrow select/alias contract is ready after 587, 588, and 589, but it is isolated from the pointer/address model | Activate after 597, or earlier only if the supervisor wants a narrow shared-prealloc consumer slice instead of research |
| 3 | `ideas/open/591_prepared_mir_view_contract_research.md` | Target-independent Prepared MIR view contract | High for MIR migration, but some pointer/address conclusions should be consumed from 597 rather than invented inside 591 | Activate after 597 when the route needs final pointer/address fact classification; a scoped dependency-inventory packet may run earlier if it does not settle pointer/address semantics |
| 4 | Deferred prepared-publication residue | Split between shared-prealloc producer publication, shared-prealloc consumer authority, and target consumers | Blocked: the families still mix aggregate-adjacent branch sources, scalar-condition-register branch shapes, string assembly, target tails, destination fan-in, and predecessor suppression | Wait until one subfamily has a single first owner, prerequisites, proof surface, and reject signals |
| 5 | Deferred call-boundary post-call publication/rematerialization | Call-boundary freshness, then shared producer publication or target consumption | Blocked: no concrete stale-home or missing-publication call path is named beyond 587 and 591 evidence | Wait for a named call consumer or producer gap |
| 6 | Deferred standalone diagnostics/reviewer policy | Diagnostics/reviewer policy outside MIR view | Blocked: 591 already owns the Prepared MIR diagnostic/proof taxonomy | Wait for a non-MIR verifier or reviewer boundary where a diagnostic/proof artifact can influence codegen authority |
| 7 | Deferred AArch64/x86 or other target consume-side migrations | Target consumer after shared authority and MIR contract exist | Blocked: target routes must know which shared authority or MIR-facing fact they consume | Wait for the relevant shared authority and 591 view contract, then open one target-consumer idea at a time |

## Immediate Next Activation

Activate `ideas/open/597_pointer_address_semantic_model_research.md` next.

Reason:

- It is the first owner for pointer/address identity, relocation meaning,
  local-memory boundary authority, stack/local address derivation, and
  published pointer value freshness.
- It is upstream of any MIR-view decision that would expose pointer/address
  facts as required codegen inputs, verifier facts, route proofs, or
  diagnostic-only artifacts.
- It prevents target RV64/AArch64/x86 routes from treating local-memory
  layout, stack homes, target operand shape, or diagnostic dumps as semantic
  pointer freshness.
- It can start from closed authority evidence without waiting for idea 591.

## Per-Idea Prerequisites And Waits

### Idea 597: Pointer/Address Semantic Model Research

Prerequisites already available:

- Closed freshness authority baseline from ideas 587, 588, 589, and 590.
- Closed narrow branch pointer stack-source evidence from ideas 592, 593,
  594, and 596.
- Step 3 through Step 5 umbrella handoff docs that classify pointer/address
  semantics as not globally closed.

Does not wait for:

- Idea 591. The Prepared MIR view may consume the semantic model later, but
  it is not the first owner for unresolved pointer/address authority.
- Idea 598. Select-carrier alias source acceptance is a separate
  shared-prealloc consumer contract.

Must produce before downstream work:

- A semantic authority rule and fail-closed rule for each surveyed
  pointer/address family.
- A classification of facts as semantic authority, verifier/support,
  target-consume, route-proof, or diagnostic-only.
- Any implementation follow-ups split by first owner, prerequisite, proof
  surface, and reviewer reject signals.

### Idea 598: Select-Carrier Alias Freshness Contract

Prerequisites already available:

- Idea 587's freshness authority vocabulary and fail-closed lookup model.
- Idea 588's select-alias blocked-on-contract inventory.
- Idea 589's direct edge-publication rule that alias, destination, complete
  home, and local move facts are not source freshness by themselves.

Does not wait for:

- Idea 597, unless an audited select-carrier route turns out to depend on
  pointer/address semantic identity rather than alias/source freshness.
- Idea 591, because this is a shared-prealloc consumer authority contract,
  not a MIR view design route.

Must wait if:

- The work expands into destination fan-in, predecessor-edge consumed
  suppression, producer publication repair, target migration, or MIR view
  design. Those are separate first-owner families.

### Idea 591: Prepared MIR View Contract Research

Prerequisites already available:

- Current backend and MIR consumption surfaces.
- Closed freshness and branch-stack evidence as input examples.
- The umbrella classification docs, including the generated 597 and 598
  split.

Waits or constraints:

- Wait for 597 before making final claims about which pointer/address facts
  are required semantic authority in `PreparedMirView`.
- Do not absorb 598's select-carrier alias ownership rule as MIR-view design.
  The MIR view may expose the result later only after shared-prealloc consumer
  authority has settled the contract.
- Do not expand into producer-publication repair, target consumer migration,
  or broad `PreparedBirModule` exposure under a new name.

Can run earlier only for:

- Inventorying current MIR dependencies and separating live dependencies from
  markdown-only legacy artifacts.
- Drafting core-view shape and diagnostic/proof boundaries while explicitly
  marking unresolved pointer/address semantics as pending 597.

## Deferred Families And Activation Gates

### Prepared Publication Remaining-Family Residue

Wait for:

- A single selected subfamily with one first owner.
- Proof that the route needs shared producer publication, shared consumer
  authority, or target consumption first.
- A concrete proof surface that is not a named-case shortcut.

Do not activate as:

- A broad "publication completeness" implementation.
- A combined aggregate-adjacent, scalar-condition-register, string assembly,
  target-tail, destination-fan-in, and predecessor-suppression route.

### Call-Boundary Post-Call Publication And Rematerialization

Wait for:

- A concrete stale-home, missing-publication, or rematerialization failure
  at a named call boundary.
- A first owner: call-boundary freshness, shared-prealloc producer
  publication, target consumption, or 591 feature-view classification.

Do not activate as:

- Broad call ABI cleanup.
- Target-local preservation or register-identity tuning without selected
  authority.

### Standalone Diagnostic Or Reviewer Policy

Wait for:

- A non-MIR verifier or reviewer boundary where diagnostic/proof artifacts can
  affect codegen authority.
- A fail-closed policy that prevents diagnostics from becoming semantic facts.

Do not activate as:

- A route that claims capability progress through dumps, diagnostics,
  expectation rewrites, unsupported markers, or allowlist changes.

### AArch64/X86 Or Other Target Consume-Side Migrations

Wait for:

- The shared producer or consumer authority that the target route must
  consume.
- A MIR-facing contract from 591 when the target should consume a
  `PreparedMirView` fact rather than a direct prepared/prealloc structure.
- A single target consumer and nearby same-feature proof surface.

Do not activate as:

- A broad multi-target sweep.
- Target-local inference replacing selected shared authority.

## Reviewer Reject Signals For Dependency-Order Drift

Reject the route if it:

- Activates 591 as a way to hide unresolved pointer/address semantic authority
  that belongs first to 597.
- Expands 598 into destination fan-in, predecessor-edge suppression, producer
  publication repair, target migration, or MIR view design.
- Opens broad prepared-publication, call-boundary, diagnostics, or target
  migration umbrellas without a single first owner and proof surface.
- Treats closed branch pointer stack-source work from 592, 593, 594, or 596 as
  open work to redo.
- Claims progress through expectation rewrites, unsupported-marker edits,
  allowlist changes, diagnostic-only changes, target-local operand shape,
  stack-home completeness, alias-only facts, or destination-only legality.
- Proves only one named testcase while nearby same-feature routes remain
  unexamined.
