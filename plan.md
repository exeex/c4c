# CFG Contract Consumption For Short-Circuit And Guard-Chain

Status: Active
Source Idea: ideas/open/59_cfg_contract_consumption_for_short_circuit_and_guard_chain.md
Supersedes: ideas/open/68_prepared_local_slot_handoff_consumption_for_x86_backend.md

## Purpose

Make the x86 prepared emitter consume authoritative prepared control-flow
contracts for guard-chain and short-circuit families as a normal backend
capability instead of rejecting routes that already cleared upstream local-slot
and prepared-module ownership.

## Goal

Replace authoritative prepared guard-chain or short-circuit handoff rejection
with one generic prepared CFG consumption path that the x86 backend can render
without reconstructing raw topology or testcase shape.

## Core Rule

Fix this by consuming or extending the prepared CFG contract, not by adding one
more x86-local branch matcher for a named failing case.

## Read First

- `ideas/open/59_cfg_contract_consumption_for_short_circuit_and_guard_chain.md`
- `ideas/open/60_scalar_expression_and_terminator_selection_for_x86_backend.md`
- `ideas/open/61_call_bundle_and_multi_function_prepared_module_consumption.md`
- `ideas/open/68_prepared_local_slot_handoff_consumption_for_x86_backend.md`
- `test_after.log`

## Scope

- authoritative prepared guard-chain handoff failures in the x86 backend
- authoritative prepared short-circuit handoff failures in the x86 backend
- shared prepared CFG helpers or contract surfaces only when the current stored
  CFG contract is insufficient for one generic render plan

## Non-Goals

- local-slot handoff routes that still stop in idea 68's authoritative prepared
  local-slot family
- scalar expression or terminator selection leaves that still belong to idea 60
- prepared-module or call-bundle traversal work that still belongs to idea 61
- testcase-shaped x86 fast paths for one named failure
- runtime correctness follow-ons after codegen already succeeds

## Working Model

- treat repeated authoritative prepared guard-chain or short-circuit failures as
  family work, not isolated testcase repairs
- keep cases that have already graduated out of idea 68 under idea 59 instead
  of counting them as local-slot progress
- prefer `build -> focused backend subset -> broader x86 backend recheck`
  whenever a packet changes shared prepared CFG consumers

## Execution Rules

- start each packet by confirming the active subset still stops in idea 59's
  authoritative prepared guard-chain or short-circuit diagnostic families
- keep x86-side work focused on rendering from authoritative prepared branch,
  block, join-transfer, and parallel-copy facts rather than reopening raw CFG
  inspection
- if the stored CFG contract is insufficient, extend shared prepared helpers or
  contract structs before growing emitter-local branching
- graduate cases out of this idea as soon as their top-level blocker moves to
  idea 60, 61, a runtime leaf, or successful emission
- preserve `todo.md` as the packet log; only rewrite this runbook when the
  route itself changes

## Step 1: Re-establish The Current Owned Guard-Chain And Short-Circuit Inventory

Goal: confirm which current `c_testsuite x86_backend` failures still stop in
idea 59's authoritative prepared CFG families before taking a new repair
packet.

Primary targets:

- `test_after.log`
- focused `c_testsuite x86_backend` cases that currently report the
  authoritative prepared guard-chain or short-circuit diagnostics
- the re-homed subset `00081.c`, `00082.c`, and `00104.c`

Actions:

- collect the current named cases that fail in the authoritative prepared
  guard-chain and short-circuit families from `test_after.log`
- confirm that `00081.c`, `00082.c`, and `00104.c` now belong here because
  their top-level blocker moved downstream from idea 68's local-slot handoff
- separate out nearby scalar, prepared-module, or runtime cases that should
  stay with ideas 60, 61, or later leaves
- choose one coherent same-family subset whose failing route shares one guard-
  chain or short-circuit contract seam

Completion check:

- the active subset is explicitly owned by idea 59 and does not mix in cases
  whose top-level blocker belongs to idea 68, 60, 61, or a later runtime leaf

## Step 2: Normalize The Needed Prepared CFG Contract

Goal: identify the minimal authoritative prepared CFG facts required to render
the owned subset generically.

Primary targets:

- shared prepared CFG helpers and structs consumed by x86 codegen
- x86 guard-chain or short-circuit rendering entry points that still reopen raw
  CFG shape

Actions:

- trace the owned route to the point where x86 still re-derives branch,
  continuation, or join meaning from raw block shape or compare carriers
- map the missing meaning onto existing prepared branch-condition, block,
  join-transfer, and parallel-copy facts when possible
- if the stored contract is genuinely insufficient, define the missing shared
  prepared CFG helper or struct extension before changing x86 rendering logic

Completion check:

- there is one explicit generic prepared CFG render plan or a clearly bounded
  shared-contract addition that explains the owned failure subset

## Step 3: Rewire X86 CFG Rendering To Consume The Generic Plan

Goal: make the x86 backend render from the normalized prepared CFG plan
instead of rejecting the owned guard-chain or short-circuit route.

Primary targets:

- x86 codegen surfaces that currently throw the authoritative prepared
  guard-chain or short-circuit handoff diagnostics
- any shared prepared CFG helper or contract consumer introduced in step 2

Actions:

- move the owned decision-making behind the normalized prepared CFG plan
- keep x86-side logic focused on rendering from authoritative prepared facts
  instead of reopening raw topology or compare recovery
- preserve clean boundaries with idea 68 local-slot ownership and ideas 60 and
  61 downstream leaves

Completion check:

- the owned subset emits through the generic prepared CFG plan without regrowing
  branch-shaped or testcase-shaped special cases in x86 codegen

## Step 4: Prove The Family And Re-route Graduated Cases Honestly

Goal: show the repaired idea-59 family moves forward and record any cases that
now belong to a downstream leaf instead of keeping them here.

Primary targets:

- the focused owned subset from step 1
- broader `c_testsuite x86_backend` coverage when the packet blast radius
  justifies it

Actions:

- run build proof plus the chosen focused backend subset for the repaired
  guard-chain or short-circuit family
- rerun a broader backend check when the packet advances more than one CFG lane
  or changes shared prepared contract
- update `todo.md` so graduated cases are routed to idea 60, 61, a runtime
  leaf, or success instead of being counted as idea-59 wins after the blocker
  moves

Completion check:

- the packet proves real progress for the authoritative prepared CFG family,
  and any downstream blockers are explicitly re-homed instead of being absorbed
  into this idea
