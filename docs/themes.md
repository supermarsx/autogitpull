# Themes

Color themes for the TUI can be supplied as JSON or YAML files containing ANSI escape codes for each color field:

```
{
  "reset": "\u001b[0m",
  "green": "\u001b[32m",
  "yellow": "\u001b[33m",
  "red": "\u001b[31m",
  "cyan": "\u001b[36m",
  "gray": "\u001b[90m",
  "bold": "\u001b[1m",
  "magenta": "\u001b[35m"
}
```

Use the `--theme` option to select a theme file:

```
autogitpull --root <path> --theme examples/themes/light.json
```

Sample themes are available under `examples/themes/`. Create your own by copying one of these files and adjusting the ANSI codes.
