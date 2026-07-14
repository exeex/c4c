# Current Packet

Status: Active
Source Idea Path: ideas/open/756_lir_switch_selector_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Publish switch-selector authority
你該做code review了

## Just Finished

- Paused 734 after accepted Step 7.22; its next switch receiver requires this
  separate typed-selector producer handoff.

## Suggested Next

- Execute Step 1 only: publish and verify `LirSwitch` selector value identity.

## Watchouts

- Do not change Raw-BIR receipt or switch-successor authority, and do not
  recover a selector from `selector_name`, `selector_type`, labels, or printer
  output.

## Proof

- Executor: run a fresh build and focused positive/negative producer proof.
- Supervisor: select and record broader/full acceptance separately.
