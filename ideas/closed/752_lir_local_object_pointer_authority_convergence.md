# LIR Local Object Pointer Authority Convergence

Status: Closed (capability complete)
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

## Accepted 734 Handoff And Closure Decision

Capability complete; closure is accepted. The sole first Raw-BIR receiver row
authorized for 734 is the
selected hoisted `LirAllocaOp` row. Its receiver may consume only the alloca
`result` and `local_object_authority` with typed
`pointer_definition`, `object`, `owner`, `pointer_type`, `pointee_type`, and
`live` fields. It must use those structured facts rather than the alloca's
display spelling.

The verifier guarantees that this authority binds that alloca result, names a
current-function pointer definition, agrees with the alloca pointee type, and
remains canonical when the same pointer is carried by repeated selected
records. Missing, invalid, foreign, pointer/object/type-mismatched, dead, or
disagreeing authority rejects before downstream use. The receiver must retain
that fail-closed boundary and transactional publication.

This is first because the accepted bounded route orders alloca before local
load/store/GEP and VLA stack-save/restore lifetime rows. Those later rows,
named/local-temporary variants beyond the selected alloca, memory/va,
aggregate/vector, PHI/CFG, target lowering, and every other family remain
outside the return packet.

Producer evidence is `ca26a8242` (authority publication), `b200ac033` (VLA
store alignment), and `0e8093025` (binding verification), with accepted
focused coverage and `^backend_` 5/5 proof. The fresh supervisor acceptance
proof is a build plus full `ctest --test-dir build -j --output-on-failure` at
3037/3037 passing (100%), recorded in `test_after.log`.

Exact return action: after 752 closes, reactivate 734 and repair its runbook
for exactly one selected-hoisted-`LirAllocaOp` Raw-BIR receiver packet. Receive
only the listed typed alloca/authority facts with importer dispatch, reachable
verification, and transactional positive/negative coverage; do not derive
semantics from displayed local names or `%t`, repeat prior 734 work, or absorb
any excluded later row.
