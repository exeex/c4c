Status: Active
Source Idea Path: ideas/open/74_aarch64_local_scalar_register_helper_fold_back.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Duplicate Scalar Helper Logic

# Current Packet

## Just Finished

Lifecycle activation created this executor-compatible scratchpad for
`plan.md` Step 1.

## Suggested Next

Execute Step 1 by inventorying duplicate scalar type, register-view, and
compare predicate helper logic across the AArch64 owned files, then record the
selected first fold-back family here.

## Watchouts

- Keep BIR facts and `abi::convert_prepared_register` as inputs rather than
  re-deriving shared semantics.
- Do not move AArch64 record schemas, condition-code spelling, register-view
  spelling, or printer formatting into shared code.
- Do not claim progress through helper renames that leave the same duplication
  behind a new name.

## Proof

Not run; lifecycle activation only.
