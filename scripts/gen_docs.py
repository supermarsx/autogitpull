#!/usr/bin/env python3
import re, json, sys
from pathlib import Path
from collections import defaultdict

# parse defaults from include/options.hpp
def parse_defaults():
    text = Path('include/options.hpp').read_text()
    pattern_eq = re.compile(r'^\s*[\w:<>\s,{}]+\s+(\w+)\s*=\s*([^;]+);', re.MULTILINE)
    pattern_br = re.compile(r'^\s*[\w:<>\s,{}]+\s+(\w+)\{([^}]+)\};', re.MULTILINE)
    pattern_decl = re.compile(r'^\s*[\w:<>\s,{}]+\s+(\w+);', re.MULTILINE)
    defaults = {}
    for m in pattern_eq.finditer(text):
        defaults[m.group(1)] = m.group(2).strip()
    for m in pattern_br.finditer(text):
        defaults[m.group(1)] = m.group(2).strip()
    for m in pattern_decl.finditer(text):
        if m.group(1) not in defaults:
            defaults[m.group(1)] = ''
    return defaults

# mapping for flags whose field names differ
FLAG_MAP = {
    '--refresh-rate':'refresh_ms',
    '--recursive':'recursive_scan',
    '--ignore':'ignore_dirs',
    '--include-dir':'include_dirs',
    '--attach':'attach_name',
    '--background':'run_background',
    '--respawn-limit':'respawn_max',
    '--max-runtime':'runtime_limit',
    '--print-skipped':'cli_print_skipped',
    '--keep-first':'keep_first_valid',
    '--cpu-poll':'cpu_poll_sec',
    '--mem-poll':'mem_poll_sec',
    '--thread-poll':'thread_poll_sec',
    '--cpu-percent':'cpu_percent_limit',
    '--cpu-cores':'cpu_core_mask',
    '--dump-large':'dump_threshold',
    '--help':'show_help',
    '--hide-date-time':'show_datetime_line',
    '--hide-header':'show_header',
    '--row-order':'sort_mode',
    '--color':'custom_color',
    '--theme':'theme_file',
    '--single-thread':'concurrency',
    '--threads':'concurrency',
    '--syslog':'use_syslog',
    '--verbose':'log_level',
    '--version':'print_version',
    '--vmem':'show_vmem',
    '--discard-dirty':'force_pull',
    '--list-daemons':'list_services',
}

# parse options from help_text.cpp
def parse_entries():
    text = Path('src/help_text.cpp').read_text()
    pattern_opt = re.compile(r'\{"([^"]+)",\s*"([^"]*)",\s*"([^"]*)",\s*"([^"]*)",\s*"([^"]*)"\}', re.MULTILINE)
    return [m.groups() for m in pattern_opt.finditer(text)]

# determine default string for a flag

def get_default(flag, field, defaults):
    if flag.startswith('--no-'):
        base = field[3:] if field.startswith('no_') else flag[5:].replace('-', '_')
        base_def = defaults.get(base, '')
        if base_def.lower() == 'true':
            return 'false (feature enabled)'
        elif base_def:
            return 'false'
        else:
            return 'false'
    if flag.startswith('--dont-'):
        base = field[5:] if field.startswith('dont_') else flag[7:].replace('-', '_')
        base_def = defaults.get(base, '')
        if base_def.lower() == 'true':
            return 'false (feature enabled)'
        else:
            return 'false'
    def_val = defaults.get(field, '')
    if def_val == '':
        return ''
    val = def_val
    if val.startswith('"') and val.endswith('"'):
        val = val[1:-1]
    if val.startswith('LogLevel::'):
        val = val.split('::',1)[1]
    if val.lower() == 'true':
        return 'true (enabled)'
    if val.lower() == 'false':
        return 'false (disabled)'
    return val

# convert default to JSON/YAML value

def to_value(default_str):
    if default_str.lower() == 'true (enabled)':
        return True
    if default_str.lower() == 'false (disabled)':
        return False
    if default_str.lower() in ('true', 'false'):
        return default_str.lower() == 'true'
    # handle strings like 'false (feature enabled)' for negative flags
    if default_str.startswith('false'):
        return False
    try:
        if '.' in default_str:
            return float(default_str)
        return int(default_str)
    except ValueError:
        return default_str


def main():
    defaults = parse_defaults()
    entries = parse_entries()
    by_cat = defaultdict(list)
    for long_flag, short_flag, arg, desc, cat in entries:
        field = FLAG_MAP.get(long_flag, long_flag[2:].replace('-', '_'))
        if field is None:
            default_str = ''
        else:
            default_str = get_default(long_flag, field, defaults)
        by_cat[cat].append((long_flag, default_str, desc))
    # generate docs
    lines = ['# Command Line Options\n']
    for cat in sorted(by_cat):
        lines.append(f'\n## {cat}\n')
        lines.append('| Option | Default | Description |')
        lines.append('|--------|---------|-------------|')
        for long_flag, def_str, desc in sorted(by_cat[cat]):
            lines.append(f'| `{long_flag}` | {def_str if def_str else ""} | {desc} |')
    Path('docs/cli_options.md').write_text('\n'.join(lines))
    # generate example configs
    json_data = defaultdict(dict)
    for cat, items in by_cat.items():
        for long_flag, def_str, desc in items:
            key = long_flag[2:]
            json_data[cat][key] = to_value(def_str if def_str else '')
    Path('examples/example-config.json').write_text(json.dumps(json_data, indent=4))
    try:
        import yaml
        Path('examples/example-config.yaml').write_text(yaml.dump(json_data, sort_keys=False))
    except Exception:
        # simple manual YAML dump
        def dump_yaml(d, indent=0):
            lines=[]
            for k,v in d.items():
                if isinstance(v, dict):
                    lines.append(' ' * indent + f'{k}:')
                    lines.extend(dump_yaml(v, indent+2))
                else:
                    lines.append(' ' * indent + f'{k}: {v}')
            return lines
        Path('examples/example-config.yaml').write_text('\n'.join(dump_yaml(json_data)))

if __name__ == '__main__':
    main()
