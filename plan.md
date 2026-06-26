# RV64 Same-Module Byval Aggregate Call Argument Runbook

Status: Active
Source Idea: ideas/open/386_rv64_object_route_same_module_byval_aggregate_call_args.md

## Purpose

Activate the next RV64 object-route boundary exposed after prepared
global-symbol publication moved `src/20030914-2.c` past global aggregate lane
loads.

## Goal

Lower or precisely route same-module RV64 calls whose prepared callsite passes
a byval aggregate/address argument.

## Core Rule

Call lowering must consume explicit prepared callsite, ABI placement,
aggregate-address, and byval facts. It must not reconstruct argument layout,
source addresses, outgoing stack state, or callee identity from testcase names,
source syntax, raw BIR text, or diagnostics.

## Read First

- `ideas/open/386_rv64_object_route_same_module_byval_aggregate_call_args.md`
- `ideas/closed/384_prepared_global_symbol_memory_access_publication.md`
- `ideas/closed/370_rv64_object_route_byval_aggregate_param_homes.md`
- The prepared dump for `src/20030914-2.c`, especially `main`, block `entry`,
  inst `36`
- Existing RV64 call, byval parameter-home, prepared call-contract, and object
  emission tests

## Current Targets

- Representative gcc torture case: `src/20030914-2.c`
- RV64 object-route lowering for `bir::CallInst`
- Prepared callsite records with `wrapper=same_module`
- Byval aggregate/address argument movement from register source to prepared
  ABI destination

## Non-Goals

- Do not reopen prepared global-symbol memory-access publication from idea 384.
- Do not reopen byval aggregate entry parameter homes from closed idea 370.
- Do not implement aggregate `va_arg` helper lowering; that belongs to
  `ideas/open/371_rv64_object_route_aggregate_va_arg_helper_lowering.md`.
- Do not implement non-register parameter homes; that belongs to
  `ideas/open/374_rv64_object_route_non_register_param_homes.md`.
- Do not expand into indirect calls, external calls, multi-block CFG, or broad
  RV64 call lowering unless the first callsite contract directly requires a
  small shared helper.

## Working Model

The representative now has explicit prepared global-symbol load facts. The
first remaining stop is a same-module call:

```text
%t1 = bir.call i32 f(ptr byval(size=72, align=4) %t0, i32 4660)
```

The prepared callsite classifies `arg0` as `bank=aggregate_address
from=register:s1 to=none` and `arg1` as `bank=gpr from=immediate:4660 to=a1`.
The active route is to determine which prepared call facts are authoritative
enough for RV64 object emission and then lower only that supported class.

## Execution Rules

- Keep implementation packets small and prove call-contract behavior before
  accepting target lowering.
- Preserve fail-closed diagnostics for incomplete callsite facts.
- Do not weaken unsupported contracts, allowlists, or representative
  expectations to claim progress.
- Record any newly exposed representative boundary in `todo.md` and route it
  to an existing or new idea instead of broadening this plan silently.
- Use canonical `test_after.log` only when delegated by the supervisor for
  executor proof.

## Steps

### Step 1: Audit Prepared Callsite Facts

Goal: identify the authoritative facts available for the first same-module
byval aggregate/address callsite.

Primary target: `src/20030914-2.c` `main`, `entry`, inst `36`.

Actions:

- Trace the prepared callsite record for the representative.
- List available facts: wrapper kind, callee identity, return placement,
  argument banks, byval size/alignment, source register/address, destination
  placement, clobbers, and outgoing stack requirements.
- Compare those facts with the RV64 object-route call emission requirements.
- Record whether the first supportable shape can be lowered directly or needs
  a narrower prepared call-contract prerequisite.

Completion check:

- `todo.md` names the exact call lowering surface and the first complete or
  missing fact set for the byval aggregate/address argument.

### Step 2: Add Focused Call-Contract Coverage

Goal: encode the selected same-module byval aggregate/address call boundary in
focused tests.

Primary target: prepared call-contract or RV64 object-emission tests that prove
the callsite facts before broad representative proof.

Actions:

- Add coverage for the first supportable same-module byval aggregate/address
  call argument.
- Add fail-closed coverage for missing byval metadata, unsupported aggregate
  source locations, ambiguous destination placement, dynamic layout, indirect
  callees, or outgoing stack requirements outside the selected class.
- Keep tests semantic and independent of `src/20030914-2.c`, `f`, `gs`, `s1`,
  or immediate `4660`.

Completion check:

- Focused tests prove the expected prepared call facts or the precise
  unsupported boundary before implementation is accepted.

### Step 3: Lower Supported Same-Module Byval Aggregate Call Argument

Goal: implement RV64 object-route lowering for the selected supportable call
argument shape.

Primary target: the RV64 `CallInst` object-emission path and any narrow helper
needed to consume prepared callsite facts.

Actions:

- Materialize the same-module callee and supported scalar argument placement
  from prepared call facts.
- Move or preserve the aggregate/address byval argument according to explicit
  prepared ABI placement and source facts.
- Preserve precise unsupported diagnostics for incomplete or unsupported
  byval aggregate/address call shapes.
- Avoid target-side reconstruction of aggregate layout or outgoing stack state.

Completion check:

- Focused RV64 call tests pass because semantic prepared call facts are
  consumed, not because the target matched raw BIR spelling.

### Step 4: Rerun Representative And Route Next Boundary

Goal: prove whether `src/20030914-2.c` advances beyond the same-module byval
aggregate/address call boundary.

Primary target: the narrow RV64 gcc torture object runner for
`src/20030914-2.c`.

Actions:

- Rerun the representative using the supervisor-selected command.
- Document whether the case passes or advances to aggregate `va_arg`,
  terminator, control-flow, non-register parameter-home, or another distinct
  owner.
- If a new distinct initiative is exposed, report it instead of broadening this
  plan.

Completion check:

- `todo.md` records the representative outcome, proof command, and next owner.

### Step 5: Broader Guard And Closure Review

Goal: prepare this call-lowering slice for supervisor acceptance and eventual
source-idea closure.

Primary target: prepared call-contract, byval, RV64 object-emission, and
representative coverage.

Actions:

- Run the focused proof required for the implementation packet.
- Run broader backend coverage if shared call lowering or ABI helper code was
  touched.
- Confirm no expectations, allowlists, unsupported contracts, or diagnostics
  were weakened to hide the current call boundary.
- Summarize any remaining same-module byval aggregate call gaps and route them
  to existing or new ideas.

Completion check:

- The implementation has fresh build/test proof, no testcase-overfit evidence,
  and clear lifecycle notes for any remaining owners.
