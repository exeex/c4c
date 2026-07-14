# Current Packet

Status: Active
Source Idea Path: ideas/open/772_lir_gep_pointer_authority_pr70460.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Trace and repair the production GEP pointer authority loss

## Just Finished

- 764 Step 1 is accepted and closed in `9680b15b9`: its fresh build, focused
  `^frontend_lir_call_type_ref$` proof, and preserved five computed-goto
  consumers passed 5/5. That bounded carrier success does not accept the
  rejected full-suite candidate because `pr70460` remains at empty
  `LirGepOp.ptr`.

## Suggested Next

- Executor: reproduce `pr70460`, establish the first empty-`LirGepOp.ptr`
  owner, and repair only that typed production seam with nearby fail-closed
  coverage. Do not rework 764 or 734.

## Watchouts

- The prior full-suite candidate remains rejected. Do not edit baseline logs,
  weaken `LirGepOp` verification, recover IDs from text, route by testcase, or
  absorb computed-goto carrier/Raw-BIR/734 work.

## Proof

- Required for Step 1: fresh `cmake --build --preset default`; a focused
  `pr70460` reproduction and nearby production/malformed authority proof.
  The supervisor, not this packet, selects and evaluates any full-suite
  candidate.
