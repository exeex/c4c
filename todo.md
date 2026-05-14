Status: Active
Source Idea Path: ideas/open/228_aarch64_module_phoenix_drafts_to_implementation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish Common Hierarchical MIR Carrier

# Current Packet

## Just Finished

Lifecycle repair reopened Step 1 as the common MIR carrier step. Earlier
AArch64 module skeleton work is no longer the first implementation target and
should be treated as dependent on the shared carrier.

## Suggested Next

Delegate plan Step 1 to establish the common hierarchical MIR carrier in
`src/backend/mir/mir.hpp` before continuing AArch64 module skeleton work. The
carrier should model module -> function -> block -> vector of instructions,
require BIR function identity on `MachineFunction`, require BIR block identity
on `MachineBlock`, and keep instruction/node provenance lightweight and
optional.

## Watchouts

- Preserve staged migration; do not delete or disconnect legacy code until the
  replacement owner for that seam is live and proved.
- Step 1 targets `src/backend/mir/mir.hpp`; defer
  `src/backend/mir/aarch64/module.hpp` and `module.cpp` skeleton work until the
  shared carrier exists.
- Keep `module.hpp` as the single non-helper public header unless lifecycle
  repair authorizes a different layout.
- Do not make the common MIR carrier a flat `vector<MachineNode>`.
- Do not use cached display strings, source spellings, broad public records,
  raw prepared/source views, or register strings as semantic lowering
  authority.
- Do not weaken tests, mark supported paths unsupported, or claim
  expectation-only progress as implementation conversion.

## Proof

No proof run for this lifecycle-only repair. The next executor packet should
produce fresh build or narrow compile proof for `src/backend/mir/mir.hpp`.
