Status: Active
Source Idea Path: ideas/open/581_rv64_ordinary_floating_cast_lowering.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Reproduce Ordinary Cast Owners

# Current Packet

## Just Finished

Activation created a fresh runbook from `ideas/open/581_rv64_ordinary_floating_cast_lowering.md`.

## Suggested Next

Execute `plan.md` Step 1 by confirming current RV64 ordinary floating-cast
owner facts for retained representatives such as `src/920618-1.c`,
`src/ieee/pr67218.c`, and `src/pr23941.c`. Use existing `550` salvage evidence
where sufficient and save fresh artifacts under
`build/agent_state/581_rv64_ordinary_floating_cast_lowering/step1/` when reruns
are needed.

## Watchouts

- Keep this lane limited to ordinary F32/F64 casts and the retained
  `uitofp i32 to float` evidence if it still shares the same owner.
- Do not use F128, long-double, soft-float helper, scalar compare, or variadic
  helper work as justification for this idea.
- Do not claim progress through unsupported-marker changes, expectation
  rewrites, route allowlist edits, named-case checks, or residual filename
  shortcuts.

## Proof

Lifecycle-only activation; no build or route proof required.
