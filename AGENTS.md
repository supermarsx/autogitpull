# Contribution Guidelines

To maintain code quality, all commits must pass the following checks **before** they are committed:

1. `make lint` – runs clang-format, cpplint **and Prettier checks** to ensure all sources follow the project's style rules.
2. `npm run format` – formats all Markdown and JSON files using Prettier.
3. The full test suite via `make test`.

Run these commands locally from the repository root:

```bash
make lint
npm run format
make test
```

If any command fails, fix the issues before committing. Pull requests failing any of these checks will be rejected.
