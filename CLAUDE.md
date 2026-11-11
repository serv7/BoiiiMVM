# BoiiiMVM — Project Rules

## Project Isolation
- Treat this project independently from all other projects.
- Never carry over assumptions, architecture or implementation details from other projects.
- Only reference another project's implementation when explicitly asked to.
- When asked to reference another project, inspect its actual source files manually.

## Context Efficiency
- Avoid unnecessarily rereading the entire project.
- Inspect only files relevant to the current task.
- Avoid repeatedly revisiting completed features and previous debugging attempts.
- Keep important technical decisions and implementation conventions documented.
- Avoid unnecessary terminal output and excessive context consumption.

## Development Continuity
- Maintain an accurate understanding of the current project state.
- Update relevant documentation when significant features, architectural changes or important fixes are completed.
- Preserve important unfinished tasks and known issues.
- Treat the actual source code as the source of truth.

## Compaction
- When compacting, preserve all important information about this project.
- Remove redundant conversation history, outdated decisions and unrelated discussions.
- Never sacrifice important implementation details simply to reduce context size.

## Technical Rules
- **Constant vectors**: BOIII's GSC compiler corrupts constant vectors like `(0,0,4)`. Always build vectors from components at runtime.
- **Thread sampling**: Never SuspendThread-sample the BO3 main thread — it crashes the game.
- **Python encoding**: Always pass `encoding='utf-8'` to `read_text()`/`write_text()` — Codex Python defaults to cp1252.
- **VirtualQuery**: Use `memory_regions.hpp` cache — raw VirtualQuery takes ~5ms per call on BO3's ~400MB regions.

## Build & Test
- Build: `./build.ps1`
- Tests: `./tools/test.ps1`
- GSC compiler: `gsc-tool.exe -m comp -g t7 -s pc64 -i client file.csc`
- Game install: `E:\SteamLibrary\steamapps\common\Call of Duty Black Ops III`
