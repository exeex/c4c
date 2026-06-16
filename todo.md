Status: Active
Source Idea Path: ideas/open/283_cpp_dependent_reference_alias_c_style_cast_lowering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Locate The Reference Alias Cast Boundary

# Current Packet

## Just Finished

Lifecycle activation created this active plan from idea 283.

## Suggested Next

Execute Step 1: reproduce one or more failing dependent reference alias
C-style cast targets, locate the first bad HIR/BIR/LLVM boundary, and record
the narrow proof command plus evidence here.

## Watchouts

- Do not weaken or skip the three positive runtime tests.
- Do not special-case the filenames or `Holder<T>::Rebind`.
- Preserve lvalue/rvalue reference overload behavior.

## Proof

Lifecycle-only activation; no build or CTest proof required.
