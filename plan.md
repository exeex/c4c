# AArch64 C-Testsuite Failure Family Inventory Runbook

Status: Active
Source Idea: ideas/open/284_aarch64_c_testsuite_failure_family_inventory.md

## Purpose

Use the AArch64 backend c-testsuite inventory as the umbrella route for
classifying remaining failures, separating ownership boundaries, and deciding
whether to close the inventory or split another focused repair idea.

## Goal

Keep the inventory current after focused owner 294 closed, without turning this
umbrella idea into implementation work.

## Core Rule

Classify first, then split or close. Do not implement repairs, weaken tests, or
claim progress from filename-specific changes inside this inventory owner.

## Read First

- `ideas/open/284_aarch64_c_testsuite_failure_family_inventory.md`
- `ideas/closed/294_aarch64_pointer_derived_address_lvalue_lowering_authority.md`
- Current lifecycle state:
  - `todo.md`
- Canonical proof artifacts if the supervisor says they are current:
  - `test_before.log`
  - `test_after.log`

## Current Targets

- Post-294 inventory state after the pointer-derived address/lvalue owner
  closed.
- Accepted full-suite baseline at commit `5b37c5906`, reported as clean
  `3159/3159`.
- Boundary evidence from Step 4 of owner 294:
  - `src/00019.c` became a same-family pointer/address win.
  - `src/00137.c` and `src/00138.c` were separated as
    return/control-value publication failures, not pointer-derived
    address/lvalue failures.

## Non-Goals

- Do not edit implementation files, tests, expected outputs, allowlists,
  unsupported classifications, runner behavior, timeout policy, or CTest
  registration.
- Do not ask an executor to perform implementation from this inventory plan.
- Do not run broad runtime scans without explicit timeout and stale-process
  cleanup.
- Do not reopen closed focused owners from residual counts alone.
- Do not create a new focused idea unless there is current evidence of a
  remaining semantic owner.

## Working Model

This idea has repeatedly reactivated after focused owners closed, used current
scan evidence to choose the next semantic family, and switched to that focused
owner before coding. After owner 294, the reported full-suite baseline is clean,
so the next packet should verify whether the umbrella inventory is now complete
or whether a remaining separated family still needs a durable focused idea.

## Execution Rules

- Treat `ideas/open/284_aarch64_c_testsuite_failure_family_inventory.md` as the
  durable source of inventory history.
- Preserve new evidence in `todo.md` first.
- Update the source idea only for durable deactivation or closure notes, or if
  a new focused idea must be split.
- If any scan is rerun, use explicit timeout and stale-process cleanup.
- If no remaining semantic family exists, route to plan-owner closure rather
  than inventing implementation work.

## Steps

### Step 1: Reconcile Post-294 Inventory State

Goal: determine whether the inventory has any remaining actionable AArch64
backend failure family after owner 294 closed.

Primary target: accepted full-suite baseline at commit `5b37c5906` and the
current post-294 lifecycle/proof evidence.

Actions:

- Confirm the active lifecycle files point to this inventory idea.
- Review the accepted clean full-suite baseline evidence and any current
  canonical logs the supervisor names.
- Preserve the owner-294 boundary evidence for `00019`, `00137`, and `00138`
  in `todo.md`.
- If the accepted baseline is sufficient and no remaining family exists,
  record that the next lifecycle action should be inventory closure.
- If the baseline is not sufficient, identify the exact missing evidence before
  requesting any rerun.

Completion check:

- `todo.md` states whether the inventory appears complete, or names the exact
  missing evidence needed before close/split.

### Step 2: Decide Close vs Split

Goal: make the lifecycle decision after Step 1 evidence is recorded.

Primary target: this active runbook and the linked inventory source idea.

Actions:

- If the inventory is complete, ask plan-owner to close the source idea and
  delete active lifecycle state after the required close gate.
- If a current semantic failure family remains, create a focused
  `ideas/open/*.md` file with concrete acceptance criteria and reviewer reject
  signals, then switch lifecycle state to that focused idea.
- If evidence is ambiguous, keep the inventory active and narrow the next
  classification packet instead of coding.

Completion check:

- The lifecycle state is either closed, switched to a focused owner, or still
  active with a precise next classification packet.

### Step 3: Preserve Durable Inventory Outcome

Goal: ensure the final inventory decision is discoverable after activation.

Primary target: `ideas/open/284_aarch64_c_testsuite_failure_family_inventory.md`
or its closed archive path if closure is accepted.

Actions:

- Add only durable closure/deactivation notes to the source idea.
- Do not mirror routine packet detail into the source idea.
- Keep separated owner notes concise and concrete.

Completion check:

- A new agent can tell whether the inventory is closed, switched to a focused
  owner, or still active, and why.
