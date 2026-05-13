Status: Active
Source Idea Path: ideas/open/212_bir_mir_allocation_contract.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Align Implemented AArch64 Surfaces

# Current Packet

## Just Finished

Step 3 of `plan.md` aligned the implemented AArch64 ABI, module, and codegen
record surfaces with `src/backend/mir/aarch64/ALLOCATION_CONTRACT.md`.

The ABI surface now exposes explicit AAPCS64 allocation pool vocabulary,
reserved MIR scratch sets (`x9`/`x10`, `v16`/`v17`), special/forbidden
classification, and long-lived home eligibility without implementing an
allocator. Module target-register records now distinguish prepared snapshots,
allocation results, and spill/reload scratch records, carry typed allocation
pool classification when physical names parse as AArch64 registers, and mark
stack offsets as prepared snapshots rather than frame-layout authority.
Codegen record operands now have explicit allocation-result and reserved MIR
scratch roles, while frame/memory byte offsets remain snapshot fields.

Focused backend tests cover the new boundary behavior for ABI pools, module
record snapshots, stack-offset snapshot markers, and codegen register roles.

## Suggested Next

Execute Step 4: align AArch64 markdown roadmap artifacts so future scalar,
memory, branch, call, return, vector, assembler, and object work routes
allocation-sensitive decisions through the shared allocation result contract
instead of local register or spill ownership.

## Watchouts

- The new ABI helpers classify register roles and candidate eligibility only;
  they do not choose value homes or implement an allocation algorithm.
- Module stack offsets remain copied prepared facts for inspection and
  diagnostics. They are not final frame-layout authority.
- Spill/reload records still preserve prepared spill-slot identity and optional
  snapshot offsets only; this slice did not introduce reload/store pseudo
  operations or machine-node emission.
- `x8`, `x16`, `x17`, `x18`, `x30`, frame-pointer-reserved `x29`, `sp`, and
  reserved MIR scratch registers remain unavailable as default long-lived BIR
  value homes.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
passed. Proof output is preserved in `test_after.log`, including
`100% tests passed, 0 tests failed out of 131`.

`git diff --check` also passed.
