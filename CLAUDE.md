# Claude Code Guidelines

## Environment

Always use project-local tooling. For Python, use uv: `uv run ruff`,
`uv run cmake-format`, `uv run python`, etc.

## Config Files

Keep `pyproject.toml` minimal (project metadata + deps only). Tool configs go in
dedicated files: `.cmake-format.yaml`, `ruff.toml`, `pyrightconfig.json`,
`.mypy.ini`.

## Before Commit

Run all code through formatters before presenting for commit:

- `uv run ruff format .` and `uv run ruff check --fix .` for Python
- `uv run cmake-format -i <file>` for CMake
- `npx prettier --write "**/*.md"` for Markdown

If a formatter config doesn't exist for a file type, stop and create one before
proceeding. Never commit code formatted with implicit defaults.
