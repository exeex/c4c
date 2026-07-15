# LIR PHI Incoming Successor-Occurrence Identity Publication Runbook

Status: Active
Source Idea: ideas/open/788_lir_phi_incoming_successor_occurrence_identity.md
Activated from: paused 734 Step 7.25 after accepted safe partial receiver `006d79aaf`

## Purpose

Supply the one missing native LIR authority required to select an exact PHI
incoming CFG successor occurrence when a predecessor has parallel conditional
or switch edges.

## Goal

Publish, verify, and hand off a per-`LirPhiIncoming` typed successor-occurrence
identifier for the selected PHI producer families.

## Core Rule

A predecessor `LirBlockId` is not an occurrence selector. Derive and validate
the new identifier only from typed producer CFG structure; never use incoming
order, labels, spellings, printer output, LLVM text, or testcase identity.

## Read First

- `ideas/open/788_lir_phi_incoming_successor_occurrence_identity.md`
- `ideas/open/734_lir_to_new_bir_container_completeness.md` resumption record
- closed 751, 786, and 787 handoffs
- the selected `LirPhiIncoming` schema, PHI producers, and LIR verifier seams

## Non-Goals

- Do not edit Raw-BIR, its importer/container/verifier, backend tests, target
  lowering, MIR/emission, or later LIR families.
- Do not complete parent 734 Step 7.25 in this plan.

## Ordered Steps

### Step 1 - Trace the selected PHI incoming occurrence source

Goal: identify the typed producer CFG occurrence and the smallest carrier seam
for the selected ternary, logical, AArch64-vaarg, and AMD64-vaarg PHI families.

Actions:

- inspect `LirPhiIncoming`, its producer construction sites, and verifier
  ownership/coherence checks;
- identify the existing typed conditional/switch occurrence representation
  without inferring from presentation or PHI ordering;
- record unsupported producer forms as fail-closed boundaries.

Completion check: one minimal native occurrence-ID contract and producer list
are identified without expanding into receiver/lowering work.

### Step 2 - Publish native per-incoming occurrence authority

Goal: extend the LIR carrier and selected producers so each incoming names its
exact successor occurrence.

Actions:

- add the typed carrier field and populate it from the producer's exact CFG
  occurrence;
- preserve duplicate conditional and ordered switch occurrences distinctly;
- leave unsupported producers absent/fail-closed.

Completion check: selected producers emit structural, current-function
occurrence authority rather than a predecessor-only approximation.

### Step 3 - Verify authority and prove malformed rejection

Goal: make invalid per-incoming occurrence authority fail closed before
downstream use.

Actions:

- verify presence, validity, same-function ownership, predecessor/destination
  coherence, uniqueness, and multiplicity/order;
- add nearby selected-family positive coverage and malformed missing, foreign,
  mismatched, duplicate, and parallel-occurrence cases;
- avoid all text-based semantic probes and testcase-shaped matching.

Completion check: positive parallel occurrences remain distinct and malformed
authority cannot publish as valid LIR.

### Step 4 - Record the exact parent handoff

Goal: provide the evidence and contract needed for 734 to resume unchanged at
Step 7.25.

Actions:

- document carrier field/type, producer populations, verifier guarantees,
  coverage, proof, and implementation commit;
- state the exact parent return point and prohibition on repeating accepted
  `006d79aaf` partial receiver work;
- request lifecycle return to 734 after supervisor accepts the handoff.

Completion check: a new agent can resume only the parent parallel-edge receiver
portion without rediscovering the authority contract.

## Proof Ladder

For implementation packets: fresh build, selected focused proof, matching
`^backend_` 5/5 before-and-after guard with `--allow-non-decreasing-passed`,
then fresh full `ctest` 3037/3037. The supervisor owns canonical regression
logs and final acceptance.
