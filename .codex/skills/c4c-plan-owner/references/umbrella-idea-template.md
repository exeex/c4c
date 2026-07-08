# Umbrella Idea Template

Use this reference when creating an `ideas/open/*.md` umbrella idea.

Umbrella ideas classify evidence and generate follow-up ideas. They are not
implementation ideas. Their output is usually a refreshed handoff directory
under `docs/<topic>/` plus ordered source ideas under `ideas/open/`.

## Required Shape

Prefer this section order:

```markdown
# <Title>

Status: Open
Type: Umbrella triage and follow-up idea generator
Parent: `<source path or none>`
Handoff Directory: `docs/<topic>/`
Related:
- `<related idea/doc/log/code path>`

## Goal

Use <current evidence> to classify <problem space> and generate ordered
follow-up ideas.

## Why This Exists

Explain why direct implementation would be premature, why the evidence must be
classified first, and what route drift the umbrella should prevent.

## Current Evidence

List the concrete logs, scans, docs, closed ideas, or summaries that should
drive triage. State which evidence supersedes stale counts or older summaries
when relevant.

## In Scope

- Refresh or create handoff docs under `docs/<topic>/`.
- Classify failures or open questions by first owning layer.
- Generate follow-up ideas under `ideas/open/`.
- Record follow-up ordering and dependency rules.

## Out Of Scope

- Implementing fixes inside this umbrella idea.
- Mixing producer repair and target consumer work in one follow-up idea.
- Expectation rewrites, unsupported-marker edits, allowlist edits, or runtime
  behavior changes as proof of progress.

## Priority Model

State how follow-up ideas should be ordered. Prefer broad semantic impact,
first owning layer, and evidence frequency over novelty or named testcase
pressure.

## Required Follow-Up Ideas

List the minimum follow-up idea families unless fresh evidence proves a better
split. Each generated follow-up should name its owning layer.

## Acceptance Criteria

- The handoff directory contains the required summary, classification, and
  follow-up-plan documents.
- The documents agree on the same current evidence source.
- Follow-up ideas are generated under `ideas/open/` and ordered.
- Each follow-up idea names its owning layer and avoids mixed ownership.
- The umbrella does not change implementation, tests, expectations,
  unsupported markers, allowlists, runtime behavior, or default harness
  contracts.

## Closure Note Requirements

The closure note must state which evidence was used, which docs were written,
which follow-up ideas were generated, how they were ordered, and what remains
unassigned or intentionally deferred.

## Reviewer Reject Signals

- Reject direct implementation inside the umbrella idea.
- Reject output that only lists counts without ownership classification and
  follow-up ideas.
- Reject stale evidence left as authoritative after newer evidence is chosen.
- Reject follow-up ideas that mix first owning layers.
- Reject testcase-shaped shortcuts, expectation rewrites, unsupported
  downgrades, allowlist filtering, or weaker runtime checks as progress.
```

## Style Rules

- Use an umbrella when the right implementation queue is unknown or stale.
- Make the handoff directory explicit near the top.
- Require generated follow-up ideas to name owning layer and dependency order.
- Include closure-note requirements; umbrella closure needs durable handoff
  traceability.
- Reviewer reject signals must block direct fixes and mixed-owner follow-up
  ideas.
