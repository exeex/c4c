Status: Active
Source Idea Path: ideas/open/353_c_testsuite_rv64_asm_objdump_roundtrip_scan_target.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Existing Test Surfaces

# Current Packet

## Just Finished

Lifecycle activation created the active runbook and execution-state skeleton
for Step 1.

## Suggested Next

Inspect existing c-testsuite and backend RV64 roundtrip test surfaces, then
update this file with the selected Step 2 integration route and proof command.

## Watchouts

- Keep the large scan opt-in and out of default full CTest.
- Report unsupported generated assembly explicitly; do not silently skip or
  rewrite unsupported lines.
- The scan must use `c4cll --codegen asm`, `c4c-as`, and `c4c-objdump`; do not
  substitute external assembler or objdump tools as the source of truth.

## Proof

Lifecycle-only activation. No build proof required yet.
