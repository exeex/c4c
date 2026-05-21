Status: Active
Source Idea Path: ideas/open/357_aarch64_recursive_pointer_formal_home_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize The Formal-Home Boundary

# Current Packet

## Just Finished

Lifecycle activation created this packet skeleton for Step 1.

## Suggested Next

Localize `c_testsuite_aarch64_backend_src_00181_c` to the pointer-formal
preserved-home publication boundary described in `plan.md` Step 1.

## Watchouts

- Do not edit implementation files, tests, expectations, runners, timeout
  policy, CTest registration, or proof-log behavior during localization.
- Do not special-case `00181`, `Hanoi`, `Move`, one callee-saved register, one
  stack offset, or one emitted instruction neighborhood.
- Keep `00170` and `00189` in the regression subset once code changes touch
  call publication.
- Keep `00173` out of this owner; it belongs to semantic-BIR pointer-derived
  string-load work.

## Proof

No proof has been run for this newly activated packet.
