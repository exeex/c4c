# Route-Local Identity Domain Cleanup Runbook

Status: Active
Source Idea: ideas/open/169_route_local_identity_domain_cleanup.md

## Purpose

Clean up compiler-internal route-local identity domains that should not use
source `TextId`, link-visible `LinkNameId`, or ambiguous raw string authority.

Goal: classify route-local string keys and move at least one meaningful local
lookup family to typed local id authority or explicitly fence it as display-only.

Core Rule: keep source and link identity domains separate from function-local,
route-local, and backend-stage-local handles.

## Read First

- `ideas/open/169_route_local_identity_domain_cleanup.md`
- Prior classifications from `ideas/closed/167_whole_codebase_string_authority_final_audit.md`
- Backend surfaces around LIR, BIR, prealloc, MIR-prep, and shared backend helpers

## Current Scope

- LIR value names, stack object names, block labels, and string-pool names.
- BIR `Value::name`, local slots, block labels, phi incoming labels, and
  route-local memory-address base names.
- Prealloc prepared value names, slot pointer carriers, register assignment
  names, and temporary addressing handles.
- Local string maps that validate or connect two route-local artifacts.

## Non-Goals

- Do not reopen parser, sema, or HIR source identity cleanup.
- Do not reuse `TextId` or `LinkNameId` for route-local temporary domains.
- Do not rename final emitted labels or registers for cosmetic reasons.
- Do not rewrite the backend allocation pipeline wholesale.
- Do not claim progress from display-only refactors or printed-text tests alone.

## Working Model

- Display-only strings may stay as strings when they do not participate in
  lookup, validation, or cross-pass transport.
- Generated local handles should use a route-local id domain when multiple
  structures need to agree on identity.
- Validation should connect related local artifacts through ids where feasible,
  with rendering kept as a final spelling concern.
- Compatibility payloads should be explicitly fenced when they must remain
  string-shaped for now.

## Execution Rules

- Inventory before changing behavior; classify each candidate string authority
  site as display-only, generated local handle, validation key, cross-pass local
  identity, or compatibility payload.
- Prefer one narrow lookup family for the first code-changing packet.
- Preserve output spelling unless the step explicitly documents an intentional
  rendering change.
- Add or update tests that prove id-based route-local lookup or validation, not
  only cosmetic output stability.
- Run a fresh build or compile proof for each code-changing step. Escalate to a
  broader backend or full CTest checkpoint when a step touches shared backend
  representation or validation behavior.

## Steps

### Step 1: Inventory Route-Local String Authority

Goal: identify concrete route-local string keys and classify which are semantic
local identity rather than display.

Primary Targets:
- LIR local names and block labels.
- BIR values, local slots, block labels, phi incoming labels, and memory base names.
- Prealloc prepared names, slot pointer carriers, register assignment names,
  and temporary addressing handles.

Actions:
- Search for string-keyed maps, sets, lookup helpers, and `find(name)` style
  paths in the target backend regions.
- Record candidate families in `todo.md` as execution notes, not in the source
  idea, unless durable source intent changes.
- Choose one meaningful family where a route-local string connects multiple
  structures and is small enough for a bounded executor packet.

Completion Check:
- `todo.md` names the selected first lookup family, its files, its current
  string authority behavior, and the proof command the executor should run.

### Step 2: Introduce Or Reuse A Route-Local Id Domain

Goal: replace the selected lookup family with typed route-local id authority.

Primary Targets:
- The family selected in Step 1.

Actions:
- Add the smallest typed local id table or reuse an existing local id domain
  that matches the family.
- Store lookup authority in ids, while preserving the rendered local spelling
  through the existing name payload or an explicit renderer.
- Keep API changes local to the selected family unless adjacent signatures must
  carry the new id.
- Avoid adding named-case shortcuts or testcase-shaped matching.

Completion Check:
- The selected family no longer uses rendered route-local spelling as lookup
  authority where the new id exists.
- Existing output spelling remains stable unless intentionally documented.
- Fresh build or compile proof passes.

### Step 3: Add Route-Local Validation Or Lookup Tests

Goal: prove the new authority is semantic, not just cosmetic.

Actions:
- Add or update tests that would fail if two related route-local artifacts were
  connected only by rendered spelling after the id migration.
- Include nearby same-feature coverage when practical so the proof is not
  limited to one named testcase.
- Keep expectation changes limited to intentional behavior, not weaker contracts.

Completion Check:
- Narrow test proof passes and demonstrates id-based lookup or validation.
- Test changes do not downgrade supported paths to unsupported or weaken the
  contract without explicit supervisor approval.

### Step 4: Fence Remaining Route-Local String Families

Goal: leave the backend with clearer boundaries for remaining route-local strings.

Actions:
- For candidates not migrated in this runbook, mark them in `todo.md` as
  display-only, generated local handle, validation key, cross-pass local
  identity, or compatibility payload.
- Add compact comments only where a string authority site would otherwise be
  easy to misread as source or link identity.
- If a separate large initiative emerges, write it as a new `ideas/open/*.md`
  rather than expanding this runbook.

Completion Check:
- Remaining selected candidates have an explicit classification.
- No new `TextId` or `LinkNameId` use appears for route-local temporary domains.

### Step 5: Broader Backend Checkpoint

Goal: verify the route-local cleanup did not destabilize backend behavior.

Actions:
- Run the supervisor-selected broader backend or CTest subset after the first
  migrated family and tests are green.
- Compare failures against the route-local cleanup scope before accepting the
  slice.

Completion Check:
- Broader validation is green, or any failures are documented as pre-existing
  and outside the route-local identity changes.
