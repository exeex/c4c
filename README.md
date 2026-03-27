# c4c

`c4c` is a C/C++ compiler project for AI infrastructure and general RISC-V-based xPU systems.

It is designed for a future where accelerator software is not limited to a fixed vendor toolchain. The goal is to make C/C++ a practical systems language for heterogeneous AI hardware, while keeping the path from language feature to hardware feature short enough for real hardware teams to use.

## Vision

`c4c` is being built to support the compilation of modern C++ AI projects such as `llama.cpp`, `torch`, and other performance-critical runtime stacks.

Beyond compiling conventional C/C++ code, `c4c` aims to provide an integrated heterogeneous programming and linking model similar in spirit to `__host__` and `__device__` in CUDA, but designed for open and customizable RISC-V-based accelerator systems.

This means one toolchain can eventually describe, compile, and link software across:

- host CPU code
- RISC-V control processors
- DMA engines
- custom ASIC datapaths
- vendor-specific xPU execution units

## Why c4c

Today, adding custom instructions or hardware-specific behavior to a compiler stack often requires touching multiple LLVM layers and maintaining `.td` files across a complex backend flow. That approach is powerful, but it is also expensive, slow to iterate, and difficult for many hardware vendors to adopt.

`c4c` takes a different direction.

By strengthening `consteval` as a core mechanism, `c4c` is intended to give hardware vendors a much faster way to introduce custom instructions and hardware-specific programming models, without having to modify the compiler core for every architectural experiment.

The long-term idea is simple:

- vendors should be able to express hardware behavior in C++-level constructs
- custom instruction definitions should be easier to prototype and evolve
- architecture bring-up should not require deep LLVM backend expertise

## Custom xPU Architecture Definition

`c4c` is intended to let you define your own heterogeneous xPU architecture on top of arbitrary combinations of:

- RISC-V cores
- DMA subsystems
- fixed-function ASIC blocks
- custom accelerators and instruction extensions

The key goal is that you can customize this hardware programming model without touching the compiler kernel itself.

In other words, `c4c` is not only a compiler project. It is an attempt to make compiler-driven hardware customization accessible enough that vendors can explore and productize new xPU designs with much lower integration cost.

## Build

```bash
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j
```

Notes:

- `clang` is optional for building `c4cll` itself.
- `clang` is required for runtime or execute-style tests and for turning emitted LLVM IR into a runnable binary.

## Testing

Run the default core suite:

```bash
cd build
ctest --output-on-failure
```

Or use the convenience target:

```bash
cd build
cmake --build . --target ctest_core
```

## Agent Workflows

The repo keeps four planning locations:

1. `ideas/open/*.md`: open ideas that agents should consider
2. `ideas/closed/*.md`: closed idea archive
3. [`plan.md`](/workspaces/c4c/plan.md): the single active runbook
4. [`plan_todo.md`](/workspaces/c4c/plan_todo.md): mutable execution state for the active plan

Lifecycle state is determined by file path and file existence:

1. no [`plan.md`](/workspaces/c4c/plan.md) and no [`plan_todo.md`](/workspaces/c4c/plan_todo.md): no active plan
2. both [`plan.md`](/workspaces/c4c/plan.md) and [`plan_todo.md`](/workspaces/c4c/plan_todo.md) exist: active plan exists
3. only one of them exists: inconsistent state that should be repaired first

The standard lifecycle is:

1. human discussion produces or updates one `ideas/open/*.md`
2. activate one idea from `ideas/open/` into [`plan.md`](/workspaces/c4c/plan.md)
3. run implementation work from [`plan.md`](/workspaces/c4c/plan.md), tracking slices in [`plan_todo.md`](/workspaces/c4c/plan_todo.md)
4. deactivate, switch, or close the plan as work changes

Closing an idea means:

1. review whether [`plan.md`](/workspaces/c4c/plan.md), [`plan_todo.md`](/workspaces/c4c/plan_todo.md), and the implementation actually match
2. delete [`plan_todo.md`](/workspaces/c4c/plan_todo.md)
3. delete [`plan.md`](/workspaces/c4c/plan.md)
4. update the corresponding file in `ideas/open/` to mark it complete and optionally record leftover issues
5. move that idea file into `ideas/closed/`

Agents should only scan `ideas/open/` for candidate work. `ideas/closed/` is archive storage.

The active [`plan.md`](/workspaces/c4c/plan.md) should always identify which `ideas/open/*.md` it comes from.

Agent entry guidance lives in [`AGENTS.md`](/workspaces/c4c/AGENTS.md).

The repo keeps a small set of agent-oriented planning prompts under [`prompts/`](/workspaces/c4c/prompts):

- [`prompts/AGENT_PROMPT_EXECUTE_PLAN.md`](/workspaces/c4c/prompts/AGENT_PROMPT_EXECUTE_PLAN.md): execute the current [`plan.md`](/workspaces/c4c/plan.md)
- [`prompts/AGENT_PROMPT_PLAN_FROM_IDEA.md`](/workspaces/c4c/prompts/AGENT_PROMPT_PLAN_FROM_IDEA.md): convert one `ideas/open/*.md` document into a runbook-style [`plan.md`](/workspaces/c4c/plan.md)
- [`prompts/AGENT_PROMPT_ACTIVATE_PLAN.md`](/workspaces/c4c/prompts/AGENT_PROMPT_ACTIVATE_PLAN.md): activate one idea into the current active [`plan.md`](/workspaces/c4c/plan.md)
- [`prompts/AGENT_PROMPT_DEACTIVATE_PLAN.md`](/workspaces/c4c/prompts/AGENT_PROMPT_DEACTIVATE_PLAN.md): fold the active runbook back into its source idea without declaring completion
- [`prompts/AGENT_PROMPT_SWITCH_PLAN.md`](/workspaces/c4c/prompts/AGENT_PROMPT_SWITCH_PLAN.md): deactivate one active plan and activate another
- [`prompts/AGENT_PROMPT_CLOSE_PLAN.md`](/workspaces/c4c/prompts/AGENT_PROMPT_CLOSE_PLAN.md): close the active plan and linked idea after completion

There are also local skills for stabilizing planning behavior:

- [idea-to-runbook-plan](/workspaces/c4c/.codex/skills/idea-to-runbook-plan/SKILL.md)
- [plan-lifecycle](/workspaces/c4c/.codex/skills/plan-lifecycle/SKILL.md)

Example:

```bash
./scripts/run_agent.sh --cli codex prompts/AGENT_PROMPT_ACTIVATE_PLAN.md
```

## License

This project is licensed under the Apache License v2.0 with LLVM Exceptions.
See [LICENSE](/workspaces/c4c/LICENSE) for details.

## Contributing

By submitting contributions to this project, you agree that they will be licensed under the project license.
See [CONTRIBUTING.md](/workspaces/c4c/CONTRIBUTING.md) for details.
