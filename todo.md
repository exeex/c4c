# Template Arg Transport De-Stringify Todo

Status: Active
Source Idea: ideas/open/51_template_arg_transport_de_stringify_refactor.md
Source Plan: plan.md

## Active Item

- Step 1: audit every `tpl_struct_arg_refs` read/write site and classify each
  use as construction, transport, identity, debug formatting, or semantic
  decode.
- Iteration focus: produce a migration map that tells us which call sites must
  move first before `TypeSpec::tpl_struct_arg_refs` can be deleted outright.
- Hard acceptance rule: the plan is not done until `tpl_struct_arg_refs` is
  fully removed from `TypeSpec` and from all semantic readers/writers.
- Next intended slice: start from the known hotspots in `ast.hpp`,
  `parser_types_base.cpp`, `parser_types_template.cpp`, `hir_templates.cpp`,
  and `compile_time_engine.hpp`, then capture any additional frontend readers
  found by targeted grep/build-break probes.

## Completed

- Activated `ideas/open/51_template_arg_transport_de_stringify_refactor.md`
  into `plan.md`.
- Rewrote the idea into an execution-oriented runbook with an explicit hard
  acceptance criterion: `tpl_struct_arg_refs` must be completely deleted for the
  refactor to count as done.
