#!/usr/bin/env python3
"""Validate community metadata only; never fetch or execute linked projects."""
import json
from pathlib import Path
import re
import sys
from urllib.parse import unquote, urlsplit

from jsonschema import Draft202012Validator
from ruamel.yaml import YAML
from ruamel.yaml.error import YAMLError
from ruamel.yaml.resolver import VersionedResolver

ROOT = Path(__file__).resolve().parents[1]
IDENTIFIER = re.compile(r'(?!000-)[0-9]{3}-[a-z0-9]+(?:-[a-z0-9]+)*')
SCHEMA = json.loads((ROOT / 'community.schema.json').read_text())
VALIDATOR = Draft202012Validator(SCHEMA)


class CoreResolver(VersionedResolver):
    @property
    def versioned_resolver(self):
        # YAML 1.2 core treats date-like scalars as text, as the website does.
        return {key: [(tag, pattern) for tag, pattern in rules
                      if tag != 'tag:yaml.org,2002:timestamp']
                for key, rules in super().versioned_resolver.items()}


def load_yaml(text):
    parser = YAML(typ='safe', pure=True)
    parser.version = (1, 2)
    parser.Resolver = CoreResolver
    parser.allow_duplicate_keys = False
    return parser.load(text)


def location(path):
    result = ''
    for part in path:
        result += f'[{part + 1}]' if isinstance(part, int) else f'.{part}'
    return result.lstrip('.') or 'catálogo'


def valid_host(host):
    # URL consumers interpret numeric hosts as IPv4, including short/hex forms.
    # Reject numeric overflow instead of accepting an invalid IP as a DNS name.
    host = unquote(host, errors='strict')
    if '\u200c' in host or '\u200d' in host:
        return False
    host = host.encode('idna').decode('ascii')
    if re.search(r'[\s%/\\?#@<>^|"\x00-\x1f\x7f]', host):
        return False
    if ':' in host:  # IPv6 was already checked by urlsplit.
        return True
    parts = host.rstrip('.').split('.')
    if not re.fullmatch(r'(?:[0-9]+|0[xX][0-9a-fA-F]*)', parts[-1]):
        return True
    if len(parts) > 4:
        return False
    numbers = []
    for part in parts:
        if part.lower().startswith('0x'):
            number = int(part[2:] or '0', 16)
        elif len(part) > 1 and part.startswith('0'):
            number = int(part, 8)
        else:
            number = int(part, 10)
        numbers.append(number)
    return all(number < 256 for number in numbers[:-1]) and numbers[-1] < 256 ** (5 - len(parts))


def https_url(value):
    try:
        url = urlsplit(value)
        return (value.startswith('https://') and url.scheme == 'https' and
                bool(url.hostname) and valid_host(url.hostname) and
                not url.username and not url.password and
                not re.search(r'[\s\\\x00-\x1f\x7f]', value) and
                (url.port is None or 0 < url.port <= 65535))
    except (ValueError, UnicodeError):
        return False


def validate_catalogue(path):
    try:
        data = load_yaml(path.read_text(encoding='utf-8'))
        errors = list(VALIDATOR.iter_errors(data))
        if errors:
            error = errors[0]
            raise ValueError(f'{location(error.absolute_path)}: {error.message}')
        entries = data['implementations']
        seen = set()
        previous = None
        for index, entry in enumerate(entries, 1):
            prefix = f'implementations[{index}]'
            for field in ('url', 'authorUrl'):
                if field in entry and not https_url(entry[field]):
                    raise ValueError(f'{prefix}.{field}: URL HTTPS inválida')
            if entry['url'] in seen:
                raise ValueError(f'{prefix}.url: URL duplicada')
            seen.add(entry['url'])
            key = (entry['author'], entry['language'])
            if previous is not None and key < previous:
                raise ValueError(f'{prefix}: fora de ordem por autor e linguagem (Unicode)')
            previous = key
        return entries
    except (OSError, ValueError, YAMLError) as error:
        raise ValueError(f'{path}: {error}') from error


def validate_repository(root):
    expected = set()
    for directory in sorted(root.iterdir()):
        if not directory.name[:1].isdigit() or not directory.is_dir():
            continue
        if not IDENTIFIER.fullmatch(directory.name) or directory.is_symlink():
            raise ValueError(f'Diretório de desafio inválido: {directory}')
        path = directory / 'community.yaml'
        if path.is_symlink() or not path.is_file():
            raise ValueError(f'{path}: arquivo regular obrigatório ausente')
        expected.add(path)
        validate_catalogue(path)
    for path in root.rglob('community.yaml'):
        if path not in expected and '.git' not in path.parts:
            raise ValueError(f'{path}: catálogo fora de um diretório canônico de desafio')


if __name__ == '__main__':
    try:
        validate_repository(Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT)
        print('Catálogos da comunidade válidos.')
    except ValueError as error:
        print(error, file=sys.stderr)
        sys.exit(1)
