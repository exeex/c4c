# Execution State

Status: Active
Source Idea Path: ideas/open/86_full_x86_backend_contract_first_replan.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Subsystem Contract Completion Pass
Plan Review Counter: 0 / 6
# Current Packet

## Just Finished

- completed plan step `1 - Backend/X86 Architecture Contract Pass` by making
  `src/backend/README.md` and `src/backend/mir/x86/README.md` architectural
  contracts, adding subsystem README contracts for `assembler/` and
  `linker/`, removing the extra `codegen/` layer, shortening live x86
  filenames, enforcing one non-helper `.hpp` per x86 directory, and restoring
  a compilable x86 seam skeleton that matches the new ownership graph

## Suggested Next

- continue step `2 - Subsystem Contract Completion Pass` by promoting the key
  in-place x86 contract companions in `api/`, `module/`, `debug/`, `abi/`, and
  `lowering/` from extraction-only notes into full `Legacy Evidence` plus
  `Design Contract` documents

## Watchouts

- the new live x86 emit path is deliberately contract-first and still uses stub
  bodies in several subsystems; future packets must treat behavior recovery as
  a separate step, not silently expand this packet
- assembler and linker are now explicit deferred subsystem contracts; do not
  let new code route around them through unrelated x86 seams
- keep the new x86 filename policy stable: one non-helper `.hpp` per directory,
  short basename-first filenames, and root compatibility through `x86.hpp`
- BIR gaps remain upstream work and should be captured as follow-on ideas
  rather than patched into x86 interfaces

## Proof

- `cmake --build --preset default --target c4c_backend`
- current packet proof is compile-only because this slice changes lifecycle,
  interface seams, and contract docs rather than full behavior
