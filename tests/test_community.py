import copy
from pathlib import Path
import tempfile
import unittest
import json
from tools.validate_community import validate_catalogue, validate_repository


class CommunityCatalogue(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.path = self.root / '001-wc' / 'community.yaml'
        self.path.parent.mkdir()
        self.entry = dict(author='Ágata 🦀', language='Rust',
                          url='https://codeberg.org/example/wc', status='completed')

    def write(self, entries):
        self.path.write_text(json.dumps({'implementations': entries}, ensure_ascii=False))

    def test_empty(self):
        self.write([])
        self.assertEqual(validate_catalogue(self.path), [])
        validate_repository(self.root)

    def test_valid_and_optional_fields(self):
        self.entry.update(authorUrl='https://example.org/ágata', notes='Leitura em fluxo.')
        self.write([self.entry])
        self.assertEqual(validate_catalogue(self.path), [self.entry])

    def test_partial_without_optional_fields(self):
        self.entry['status'] = 'partial'
        self.write([self.entry])
        self.assertEqual(validate_catalogue(self.path)[0]['status'], 'partial')

    def test_multiple_same_author_sorted_by_language(self):
        second = dict(self.entry, language='Zig', url='https://example.org/wc.zig')
        self.write([self.entry, second])
        self.assertEqual(len(validate_catalogue(self.path)), 2)
        self.write([second, self.entry])
        with self.assertRaisesRegex(ValueError, 'community.yaml.*implementations\\[2\\].*ordem'):
            validate_catalogue(self.path)

    def test_missing_required_fields(self):
        for field in ['author', 'language', 'url', 'status']:
            with self.subTest(field=field):
                entry = copy.copy(self.entry)
                del entry[field]
                self.write([entry])
                with self.assertRaisesRegex(ValueError, 'community.yaml.*implementations\\[1\\]'):
                    validate_catalogue(self.path)

    def test_invalid_fields(self):
        for field, value in [('author', 42), ('author', ' '), ('language', False),
                             ('notes', []), ('authorUrl', None), ('status', 'verified'),
                             ('extra', 'typo')]:
            with self.subTest(field=field, value=value):
                self.write([dict(self.entry, **{field: value})])
                with self.assertRaises(ValueError):
                    validate_catalogue(self.path)

    def test_invalid_urls(self):
        for field in ['url', 'authorUrl']:
            for url in ['http://example.org', 'javascript:alert(1)', 'data:text/plain,x',
                        'https://', 'https:///path', 'https://bad host/x',
                        'https://user:pass@example.org/x', 'https://example.org:bad/x', 'https://256.256.256.256/x', 'https://999999999999/x', 'https://%ZZ/x', 'https://a\u200db.com/x', 'https://a\u200cb.com/x']:
                with self.subTest(field=field, url=url):
                    self.write([dict(self.entry, **{field: url})])
                    with self.assertRaises(ValueError):
                        validate_catalogue(self.path)

    def test_duplicate_urls(self):
        self.write([self.entry, dict(self.entry, language='Zig')])
        with self.assertRaisesRegex(ValueError, 'implementations\\[2\\].*duplicada'):
            validate_catalogue(self.path)

    def test_invalid_yaml_and_shape(self):
        for text in ['implementations: [', '{}', 'implementations: {}',
                     'implementations: []\nextra: true',
                     'implementations: []\nimplementations: []']:
            with self.subTest(text=text):
                self.path.write_text(text)
                with self.assertRaisesRegex(ValueError, 'community.yaml'):
                    validate_catalogue(self.path)

    def test_missing_catalogue(self):
        with self.assertRaisesRegex(ValueError, '001-wc/community.yaml'):
            validate_repository(self.root)

    def test_orphan_catalogue(self):
        self.write([])
        other = self.root / 'docs' / 'community.yaml'
        other.parent.mkdir()
        other.write_text('implementations: []')
        with self.assertRaisesRegex(ValueError, 'docs/community.yaml'):
            validate_repository(self.root)

    def test_yaml_12_scalar_and_alias_semantics(self):
        self.path.write_text("""implementations:
  - author: &name On
    language: Rust
    url: https://example.org/code
    status: completed
    notes: *name
""")
        entry = validate_catalogue(self.path)[0]
        self.assertEqual(entry['author'], 'On')
        self.assertEqual(entry['notes'], 'On')
        self.path.write_text(self.path.read_text().replace('&name On', '&name 0o123'))
        with self.assertRaises(ValueError):
            validate_catalogue(self.path)

    def test_date_like_notes_remain_strings(self):
        for notes in ['2026-10-20', '2026-10-20T00:00:00Z']:
            self.path.write_text(f"""implementations:
  - author: Ada
    language: Rust
    url: https://example.org/code
    status: completed
    notes: {notes}
""")
            self.assertEqual(validate_catalogue(self.path)[0]['notes'], notes)

    def test_yaml_merge_is_validated_after_expansion(self):
        self.path.write_text("""implementations:
  - &base
    author: Ada
    language: C
    url: https://example.org/c
    status: completed
  - <<: *base
    language: Rust
    url: https://example.org/rust
""")
        entries = validate_catalogue(self.path)
        self.assertEqual(entries[1]['author'], 'Ada')
        self.assertEqual(entries[1]['language'], 'Rust')
