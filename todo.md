Status: Active
Source Idea Path: ideas/open/100_aarch64_00204_stdarg_hfa_runtime_repair.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Reproduce And Characterize 00204

# Current Packet

## Just Finished

Lifecycle activation created this execution scratchpad for Step 1 of
`plan.md`.

## Suggested Next

Start with Step 1: reproduce `c_testsuite_aarch64_backend_src_00204_c`,
capture the runtime mismatch, split stdarg string/integer corruption from HFA
payload corruption, and record the exact commands and outputs here.

## Watchouts

- Do not downgrade `00204.c` expectations or mark it unsupported.
- Do not special-case `00204.c` or its literal output shape.
- Keep `00032.c` and `00182.c` visible as AArch64 guard cases.
- Treat narrow probes as ABI-fact probes, not testcase-shaped shortcuts.

## Proof

Lifecycle-only activation. No build or backend tests were run.
