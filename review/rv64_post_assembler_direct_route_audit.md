# RV64 Post-Assembler-Direct Route Audit

Date: 2026-04-13
Active Plan: `plan.md`
Source Idea: `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`

## Checkpoint Reviewed

- chosen base commit: `738abcd9`
  (`[plan_change] [todo_change] [idea_open_change] lifecycle: advance rv64 assembler route to direct packet`)
- why this is the right checkpoint:
  `scripts/plan_change_gap.sh` reports `738abcd9` as the latest canonical
  lifecycle checkpoint, and that rewrite is the one that records assembler
  activation as complete while narrowing the active route to the first direct
  RV64 assembler packet
- commits since base: `2`
  (`23138979 rv64: route activation slice through parser-backed assembler`,
  `b5bd93a4 rv64: encode activation slice through parsed operands`)

## Scope Reviewed

- `plan.md`
- `todo.md`
- `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`
- `src/backend/riscv/assembler/mod.cpp`
- `src/backend/riscv/assembler/elf_writer.cpp`
- `src/backend/riscv/assembler/parser.cpp`
- `src/backend/riscv/assembler/parser.hpp`
- `tests/backend/backend_shared_util_tests.cpp`
- `rv64_assembler_direct_build.log`
- `rv64_assembler_direct_shared_route_test.log`
- `rv64_assembler_direct_variant_test.log`

## Findings

- Medium: commits `23138979` and `b5bd93a4` together now truthfully complete
  the direct packet selected by the current lifecycle checkpoint. The public
  RV64 assembler path parses assembly into shared `AsmStatement`/`Operand`
  types before object emission, and the bounded activation slice now derives
  its instruction words from parsed operands instead of a hard-coded five-line
  text matcher plus fixed activation byte vector:
  [src/backend/riscv/assembler/mod.cpp](/workspaces/c4c/src/backend/riscv/assembler/mod.cpp:41),
  [src/backend/riscv/assembler/elf_writer.cpp](/workspaces/c4c/src/backend/riscv/assembler/elf_writer.cpp:155),
  [src/backend/riscv/assembler/elf_writer.cpp](/workspaces/c4c/src/backend/riscv/assembler/elf_writer.cpp:178).
- Medium: the focused proof now demonstrates the intended bounded contract
  rather than only one exact spelling. The shared backend handoff still emits a
  parseable RV64 object for the activation slice, and the direct assembler
  variant test now accepts equivalent `x10` / `x0` register spellings that the
  previous activation-specific recognizer would not have accepted:
  [tests/backend/backend_shared_util_tests.cpp](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp:9102),
  [tests/backend/backend_shared_util_tests.cpp](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp:9132).
- Medium: there is no equally narrow direct follow-on left inside this exact
  activation slice route. The remaining visible assembler work widens into
  broader instruction/directive coverage or the still-stubbed translated writer
  surface rather than another same-shape activation packet, and the public path
  is still explicitly bounded to the minimal prepared-LIR return-add handoff:
  [src/backend/riscv/assembler/mod.cpp](/workspaces/c4c/src/backend/riscv/assembler/mod.cpp:45),
  [src/backend/riscv/assembler/elf_writer.cpp](/workspaces/c4c/src/backend/riscv/assembler/elf_writer.cpp:447).

## Judgments

- plan-alignment judgment: `on track`
- technical-debt judgment: `action needed`
- reviewer recommendation: `rewrite plan/todo before more execution`

## Route Decision

Rewrite `plan.md` / `todo.md` before more execution so canonical state records
commits `23138979` and `b5bd93a4` as the completed first direct RV64 assembler
packet and then selects the next truthful continuation from a fresh lifecycle
decision instead of guessing another executor packet.

The just-completed packet should now be recorded as:

- parser-backed RV64 activation-slice acceptance through shared parsed
  statements
- operand-driven encoding of the bounded `addi` / `ret` activation payload
- focused proof that the shared backend route and the public RV64 wrapper both
  emit the same parseable bounded object, including equivalent register
  spellings for the activation variant

The next route should not be implied from the current packet. It now needs a
fresh lifecycle choice between broader assembler coverage, deeper translated
writer activation, or a different RV64 integration lane.

## Rationale

The current plan checkpoint narrowed the lane to one direct assembler packet:
make the already-emitted activation slice truthful through the parser/object
path instead of exact-text matching and fixed activation bytes. That is now
true in the code and in the proof logs.

What remains is no longer the same packet. The branch is past the activation
recognizer stage, but it has not yet chosen the next honest RV64 route. That
decision belongs in canonical lifecycle state before more execution packets are
issued.
