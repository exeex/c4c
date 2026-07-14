# LIR Local Object Pointer Authority Convergence

Status: Open
Type: bounded LIR local/object pointer authority repair
Predecessor: `ideas/open/751_lir_phi_incoming_value_and_predecessor_identity.md`

## Goal

Publish a coherent local object and pointer-definition authority model for
alloca, stack, local address, load/store/GEP, and VLA stack save/restore routes
that currently rely on `%t` and local spelling.

## Why This Exists

`LirStackObject` has a stack-slot identity, and selected memcpy work introduced
small object/lifetime facts, but most local pointer producers still communicate
through display strings. Generic `LirValueId` ownership is not enough: pointer
receivers also need to know which local object a pointer denotes and whether
that object is live.

## In Scope

- Define the minimal current-function object/pointer authority shared by local
  allocas, named locals, temporary local objects, VLA stack save/restore, and
  local load/store/GEP producers.
- Populate selected representative producers without widening to every memory
  operation at once.
- Verify object ownership, pointer-definition ownership, type relation, and
  live-lifetime facts.
- Preserve display text only as rendering compatibility.
- Add nearby same-feature coverage for local load, store, GEP, alloca, and VLA
  stack lifetime boundaries.

## Out Of Scope

- Memcpy/memset/va-list semantic receipt; idea 753 owns memory/va intrinsic
  convergence after this object substrate exists.
- PHI/CFG work, aggregate/vector value semantics, Raw-BIR, target lowering,
  MIR, or emission.
- Alias analysis, overlap semantics, or recovery from local variable names.

## Acceptance Criteria

- Representative local/object pointer routes have typed current-function
  value/object/lifetime authority.
- The verifier rejects missing, invalid, foreign, mismatched, or dead object
  relations before downstream consumption.
- Existing textual local names and `%t` spellings are not semantic authority.
- Full baseline acceptance requires 100% passing tests. If a baseline run is
  below 100%, reject closure and trace `log/*` by time/commit to identify the
  first bad commit before continuing.
