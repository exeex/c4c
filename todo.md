Status: Active
Source Idea Path: ideas/open/253_phase_f3_prepared_module_structural_exit_blocker_map.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Map `module` Structural Exit Preconditions

# Current Packet

## Just Finished

Step 2 - Map `module` Structural Exit Preconditions completed as an
analysis-only map for `PreparedBirModule::module`.

`module` structural exit map:

- Semantic BIR module/function/block structure:
  `PreparedBirModule::module` is still the semantic BIR carrier seeded by
  `BirPreAlloc` and returned after prealloc phases. `run_legalize` and
  `run_out_of_ssa` mutate it directly; `run_stack_layout`, `run_liveness`, and
  `run_regalloc` iterate functions/blocks/instructions and use
  `prepared_.module.names` to bridge BIR labels to prepared ids. These are
  semantic structure readers, but they are not one-reader exits because the
  live mutable module is also the phase handoff object.
- Prepared aggregate handoff:
  contract publishers still read `prepared.module.functions`, block contents,
  globals, calls, stores, atomics, intrinsics, inline asm, and
  `prepared.module.names` while publishing prepared facts. Concrete examples:
  `populate_call_plans` passes `prepared.module` to `resolve_direct_callee`,
  `build_memory_return_plan`, and `plan_call_argument_source`;
  `populate_store_source_publication_plans` scans stores and calls recovered
  source helpers with `prepared.module.names`; `populate_dynamic_stack_plan`,
  `populate_variadic_entry_plans`, `populate_atomic_operations`,
  `populate_intrinsic_carriers`, and `populate_inline_asm_carriers` scan BIR
  functions. These remain blocked prepared authority until each publisher has a
  semantic view or retained aggregate adapter plus fail-closed drift proof.
- Printer/module output compatibility:
  `prepared_printer.cpp::print` emits `bir::print(module.module)` and preserves
  the blank-line rule based on BIR functions/globals/string constants.
  `prepared_printer/functions.cpp`, `prepared_printer/value_locations.cpp`, and
  `prepared_printer/select_chains.cpp` re-find BIR functions/blocks from
  `module.module.functions` for debug rows. These readers are output
  compatibility surfaces; a structural exit cannot change text ordering,
  missing-id elision, or blank-line behavior without an explicit output
  compatibility decision.
- Target-policy consumers:
  x86 and AArch64 keep `module.module` as part of backend handoff. x86 readers
  include `find_consumed_bir_function`, fast-path classification, control-flow
  handoff validation, same-module global/function/string-constant policy,
  pointer-compare folding, function emission, and data emission. AArch64
  readers include `codegen::traversal::find_bir_function`,
  `append_string_constants`, `append_global_objects`, and ABI target-triple
  fallback. These are target-policy/output consumers, not semantic-only exits.
- Retained public compatibility:
  tests and handoff helpers directly seed, mutate, print, or assert
  `prepared.module`. That surface is evidence for compatibility, but it is not
  implementation authority for this packet. Any future adapter must preserve
  the public prepared aggregate shape or explicitly migrate tests as part of a
  compatibility-approved packet.

Concrete future packets:

- One-reader packet candidate:
  `prepared_lookups.cpp::prepared_bir_function`,
  `prepared_lookups.cpp::prepared_bir_block`, and
  `prepared_lookups.cpp::prepared_bir_block_label_id` can be grouped as a
  lookup-reader packet only if the new input is a semantic BIR function/block
  view. Reader: prepared lookup construction, including
  `make_prepared_function_lookups`. Semantic fact: resolve prepared
  function/block ids to BIR functions/blocks using function names, block label
  ids, and label spelling fallback. Compatibility surface: current null return
  and `kInvalidBlockLabel` behavior for missing ids, missing names, and stale
  labels. Fail-closed proof: valid lookup, missing function id, missing block
  id, stale BIR label id, and label-string fallback cases must continue to
  return the current null/invalid result instead of inventing authority.
- One-reader packet candidate:
  `prepared_printer.cpp::print` can be isolated only as a printer-output
  packet. Reader: the top-level prepared printer BIR body emission and
  non-empty-module blank-line check. Semantic fact: complete BIR module text
  supplied to `bir::print`. Compatibility surface: exact prepared-printer
  section order, BIR text, and blank-line behavior. Fail-closed proof: empty
  module, function-only module, global/string-constant module, and prepared
  phase/note header cases must produce unchanged printer output.

Blocked exits:

- Prealloc phase mutation (`run_legalize`, `run_out_of_ssa`) is blocked because
  the mutable BIR module is the phase output, not just a reader.
- Contract-plan publishers are blocked unless a packet names one publisher,
  the semantic BIR fact it needs, the retained prepared aggregate/output
  surface, and a fail-closed proof for missing ids, stale labels, and policy
  fallbacks.
- x86 and AArch64 backend readers are blocked because they combine semantic BIR
  structure with target policy, assembly output, target-triple fallback, and
  handoff validation.
- Broad `PreparedBirModule::module` deletion, privatization, or wrapper work is
  blocked by the public aggregate compatibility surface.

## Suggested Next

Proceed to Step 3 by mapping `PreparedBirModule::names` structural exit
preconditions, separating semantic lookup readers from debug/printer text,
target formatting, fallback rendering, and duplicate/conflict compatibility.

## Watchouts

Do not promote target-side `module.module` readers into semantic-only exits:
x86 same-module/global/string/fold paths and AArch64 global/string emission are
target-policy and output-sensitive. The two named future packets are bounded
analysis candidates, not implementation approval; each still needs the proof
listed above before code changes.

## Proof

Analysis-only packet; no build or test proof required. Ran
`git diff --check -- todo.md`.
