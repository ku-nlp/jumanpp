# General

> **Meta-rule**: Rules in this file include their rationale. Rules with rationales are followed more reliably and transfer better to novel situations.
- **Documentation is dual-use**: Agents start each session with no memory. All docs under `docs/` serve both humans and agents — include exact commands, paths, and rationales for non-obvious steps.
- **Do not be a yes-man**: Humans make bad decisions and forget to tell the whole picture. Ask the user to clarify and use data to improve your decisions.
- **Markdown line style**: One sentence per line, no hard-wrapping at 80 columns. Sentence-per-line keeps diffs readable and lets editors soft-wrap to the viewer's width.

# Codebase Stage & Work Modes

jumanpp v2 is a released, mature C++ morphological analyzer — **but the codebase is under active evolution**.
Treat existing code as *provisional where it serves the new direction* and *load-bearing where downstream users depend on it* (CLI, output format, model-file compatibility with released tarballs).

**Continuous refactor is the correct default, not the exception.**
Past experience with minimum-diff / patch-mode on this codebase produced technical-debt accumulation that later refactors had to pay off at higher cost.
"Prefer minimal edits" means: minimize comprehension cost for session N+5, not line count in this diff.

**Work modes** (user sets at session start or switches mid-session):
- **Evolve** (~80%): evolve domain model and codebase toward correct modeling. Refactors and rewrites welcome, including revolutionary ones. **Default mode.**
- **Analyze** (~15%): read code/logs, make plans, no code changes.
- **Meta** (~5%): improve interaction workflows (this file, `docs/`).
- **Patch**: minimal diff for a specific problem. **Never assume this mode** — user must request it explicitly. Defensive minimalism on this codebase has historically produced debt, not safety.

**Design rules** (evolve mode):
- Domain objects over god services. If logic only needs one object's data, it belongs on that object.
- Make invalid states non-representable. If two values are meaningless without each other, they're one type. If a pipeline has stages, the stage outputs are types.
- Concepts map to domain objects. Nouns are types. Verbs can be both methods and types.

# Project Rules & Guidelines

## Environment & Configuration
- **Git Protocol**: User intent is always partial; they edit files between turns. Never commit until triggered. Inspect actual state when committing.
- **Commit Prefixes** (match existing log style):
  - `fix:` bug fix
  - `build:` CMake / dependency / packaging
  - `refactor:` structural change, no behavior
  - `feat:` new user-visible capability
  - `docs:` human-and-agent system knowledge under `docs/` or README
  - `agent:` changes to `CLAUDE.md` or agent-only artifacts
  - `test:` test-only changes

## Workflow & Planning
- **Hypothesis Protocol**: When investigating:
    1. State multiple conflicting hypotheses. A single hypothesis is a conclusion in disguise.
    2. State them to the user before running off to validate for 10 minutes.
    3. Persist what survives in `docs/` (e.g. `docs/knowledge/` — create if needed). Session-scoped hypotheses stay in the plan/issue.
- **Review Mode**: For non-trivial changes: propose in text, read-only checks only, wait for approval. Surface everything relevant, not just what was asked.
- **Incremental Implementation**: Don't assume you know full scope. Work on small sub-tasks, verify alignment after each.
- **Plan Fidelity**: Don't silently reduce plan scope. If A turns out wrong mid-implementation, stop and update the plan — don't deliver partial work and call it done.
- **Scope Decisions Are Not Yours to Make Silently**: Related work is either included (if excluding breaks coherence) or asked about. "That's separate work" is never a silent conclusion.
- **Meta Feedback**: On `meta:` prefix, pause immediately. Propose instruction-file changes via Review Mode, apply after approval, resume. Update this file, not memory.

## Plans
Use GitHub Issues on `ku-nlp/jumanpp` for plan tracking.
Never use per-project memory — it's local only.

# Build & Test

Out-of-source build (CMake refuses in-source):

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug   # or Release for perf work
cmake --build build -j
ctest --test-dir build --output-on-failure
```

For formatting before commits: `./do_format.sh` (clang-format).

Ubuntu 22.04 needs `libprotobuf-dev protobuf-compiler`.

**Model compatibility warning**: Current git HEAD is not compatible with released model files (2.0-rc1 / rc2).
End-to-end analysis requires either rebuilding the dictionary or using a matching release tarball.
Do not commit model or dictionary binaries.

# Subsystem Map

- `src/core/` — analysis engine (lattice), feature computation, dictionary compilation, training, codegen, spec DSL.
  - `analysis/` — runtime lattice, beam search, char lattice, analyzer
  - `spec/` — DSL for declaring dictionary fields, features, unks
  - `training/` — structured perceptron / loss
  - `codegen/` — generated feature-compute C++
  - `dic/` — dictionary builder & reader
- `src/jumandic/` — Juman dictionary schema + `jumanpp` CLI.
- `src/rnn/` — RNNLM scorer. **Experimental replacement target** (transformer).
- `src/util/` — containers, mmap, serialization, flatmap, logging.
- `src/testing/` — standalone test harness used by `*_test.cc` files.

# Language & Style

- **C++14 baseline.** Widely supported everywhere we build (gcc, clang, MSVC, mingw64). No reason to artificially avoid its features; also no reason to reach for C++17/20 without discussing — CMake and CI expect C++14.
- Headers and sources colocated under `src/`; tests sit next to the code they test as `*_test.cc`.
- Run `./do_format.sh` before committing. It wraps `script/git-clang-format.py` and formats only changed hunks, not full files.
- **Do not mass-reformat the codebase.** Formatting migrates per-hunk as files are edited, so the tree gradually picks up whatever clang-format version contributors have locally. Full-file passes break `git blame` and produce churn with no real benefit.

# Testing

- In-tree harness: `src/testing/standalone_test.h` wraps Catch-style `TEST_CASE`. Tests are `*_test.cc` files next to their subject and are discovered by CMake.
- Run the full suite: `ctest --test-dir build --output-on-failure`. For one test: `ctest --test-dir build -R <name> --output-on-failure`.

# Documentation

> Agents start cold. Docs exist so session N+5 doesn't re-derive session N.

- **System docs** (`docs/`): existing files — `analysis.md`, `building.md`, `dictionary.md`, `output.md`, `spec.md`. High-level *what* and *why*; implementation lives in code.
- **Knowledge** (`docs/knowledge/`): instructive dead-ends and confirmed non-obvious facts from investigations. Create the directory on first use.
- **Code comments**: describe the *goal*, not the mechanism. If a comment only narrates what the code already says, delete it. Non-obvious code with no clear goal is a refactor target, not a comment target.