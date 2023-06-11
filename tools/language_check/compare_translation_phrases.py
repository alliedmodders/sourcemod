#!/usr/bin/python3
# Copyright (c) 2023 Peace-Maker
from collections import defaultdict
from dataclasses import dataclass
import os
import pathlib
import re
from smc_parser import smc_string_to_dict
from typing import Dict, List, Union
from github_gql import GithubGQL


@dataclass
class Translation:
    langid: str
    translation: str
    param_count: int


@dataclass
class Phrase:
    key: str
    format: Union[Translation, None]
    translations: List[Translation]


@dataclass
class PhraseFile:
    filename: str
    phrases: List[Phrase]
    error: Union[str, None] = None


@dataclass
class Language:
    langid: str
    name: str
    files: List[PhraseFile]


@dataclass
class Report:
    langid: str
    filename: str
    file_warning: str = ''
    phrase_key: str = ''
    phrase_warning: str = ''


def parse_translations(path: str):
    param_regex = re.compile(r'\{[0-9]+\}', re.MULTILINE)
    units = []
    for file in pathlib.Path(path).glob('*.txt'):
        if not file.is_file():
            continue

        try:
            phrases = smc_string_to_dict(file.read_text('utf-8'))
        except Exception as ex:
            print(f'Error parsing {file.name}: {ex}')
            units.append(PhraseFile(file.name, [], str(ex)))
            continue

        if 'Phrases' not in phrases:
            print(f'File {file.name} does not start with a "Phrases" section')
            continue

        parsed_phrases = []
        for phrase in phrases['Phrases']:
            for phrase_ident, raw_translations in phrase.items():
                translations = []
                format_special = None
                for child_langid, translation in raw_translations.items():
                    if child_langid == '#format':
                        format_special = Translation(
                            child_langid, translation,
                            translation.count(',') + 1)
                    else:
                        translations.append(
                            Translation(child_langid, translation,
                                        len(param_regex.findall(translation))))
                parsed_phrases.append(
                    Phrase(phrase_ident, format_special, translations))
        units.append(PhraseFile(file.name, parsed_phrases))
    return units


# Parse the languages.cfg file to know which languages could be available
print('Parsing languages.cfg...')
available_languages: Dict[str, Language] = {}
languages_cfg = smc_string_to_dict(
    pathlib.Path('../../configs/languages.cfg').read_text('utf-8'))
for langid, lang in languages_cfg['Languages'][0].items():
    available_languages[langid] = Language(langid, lang, [])

print(f'Available languages: {len(available_languages)}')

# Parse the english translation, since it doesn't use a subdirectory and is the baseline for all other translations
available_languages['en'].files = parse_translations('../../translations')

# Parse the other translations
for langid, lang in available_languages.items():
    if langid == 'en':
        continue
    lang.files = parse_translations(f'../../translations/{langid}')

reports: Dict[str, Dict[str,
                        List[Report]]] = defaultdict(lambda: defaultdict(list))

# Compare the english translation with the other translations
english = available_languages['en']
for langid, lang in available_languages.items():
    if langid == 'en':
        continue

    # See if this language has anything that English doesn't
    for file in lang.files:
        english_file = next(
            (x for x in english.files if x.filename == file.filename), None)
        if english_file is None:
            reports[langid][file.filename].append(
                Report(langid,
                       file.filename,
                       file_warning='File doesn\'t exist in English'))
            continue

        if not file.phrases:
            reports[langid][file.filename].append(
                Report(langid, file.filename, file_warning='File is empty'))
            continue

        for phrase in file.phrases:
            if phrase.format:
                reports[langid][file.filename].append(
                    Report(langid,
                           file.filename,
                           phrase_key=phrase.key,
                           phrase_warning='Includes a "#format" key'))
            english_phrase = next(
                (x for x in english_file.phrases if x.key == phrase.key), None)
            if english_phrase is None:
                # look for this phrase in a different english file
                warning = 'Phrase doesn\'t exist in English'
                for other_file in english.files:
                    other_phrase = next(
                        (x for x in other_file.phrases if x.key == phrase.key),
                        None)
                    if other_phrase:
                        warning = f'Phrase exists in a different file in English: {other_file.filename}'
                        break
                reports[langid][file.filename].append(
                    Report(langid,
                           file.filename,
                           phrase_key=phrase.key,
                           phrase_warning=warning))
                continue
            translation_found = False
            for translation in phrase.translations:
                if translation.langid == langid:
                    translation_found = True
                else:
                    reports[langid][file.filename].append(
                        Report(
                            langid,
                            file.filename,
                            phrase_key=phrase.key,
                            phrase_warning=
                            f'Includes a translation for language "{translation.langid}"'
                        ))
                if english_phrase.format and translation.param_count != english_phrase.format.param_count:
                    reports[langid][file.filename].append(
                        Report(
                            langid,
                            file.filename,
                            phrase_key=phrase.key,
                            phrase_warning=
                            f'Has {translation.param_count} format parameters, but English has {english_phrase.format.param_count}'
                        ))
            if not translation_found:
                reports[langid][file.filename].append(
                    Report(langid,
                           file.filename,
                           phrase_key=phrase.key,
                           phrase_warning=
                           'Phrase available, but translation missing'))

    # See if this language is missing anything that English has
    for file in english.files:
        lang_file = next(
            (x for x in lang.files if x.filename == file.filename), None)
        if lang_file is None:
            reports[langid][file.filename].append(
                Report(langid, file.filename, file_warning='File missing'))
            continue

        # The file doesn't contain any phrases. We reported that already, so don't spam every single missing phrase
        if not lang_file.phrases:
            continue

        for phrase in file.phrases:
            lang_phrase = next(
                (x for x in lang_file.phrases if x.key == phrase.key), None)
            if lang_phrase is None:
                reports[langid][file.filename].append(
                    Report(langid,
                           file.filename,
                           phrase_key=phrase.key,
                           phrase_warning='Phrase missing'))

    if langid not in reports:
        print(f'No issues found for {lang.name} ({langid})')
    else:
        print(
            f'Found {len(reports[langid])} issues for {lang.name} ({langid})')

GITHUB_TOKEN = os.environ.get('GITHUB_TOKEN')
if not GITHUB_TOKEN:
    raise Exception('GITHUB_TOKEN environment variable not set')
ORGANIZATION = os.environ.get('ORGANIZATION')
if not ORGANIZATION:
    raise Exception('ORGANIZATION environment variable not set')
PROJECT_NUMBER = os.environ.get('PROJECT_NUMBER')
if not PROJECT_NUMBER:
    raise Exception('PROJECT_NUMBER environment variable not set')

# Get the project and its draft issues
print('Getting project and draft issues...')
githubgql = GithubGQL(GITHUB_TOKEN)
project = githubgql.get_project(ORGANIZATION, int(PROJECT_NUMBER))
project_id = project['id']
field_ids = project['fields']['nodes']
status_field = [field for field in field_ids if field['name'] == 'Status']
assert len(status_field) == 1, 'Status field not found'
status_field_id = status_field[0]['id']
status_field_option_ids = {
    option['name']: option['id']
    for option in status_field[0]['options']
}
if 'Incomplete' not in status_field_option_ids:
    raise Exception('Incomplete status field option not found')
if 'Complete' not in status_field_option_ids:
    raise Exception('Complete status field option not found')
draft_issues = project['items']['nodes']

# Generate the report markdown for the project draft issues
for langid, lang in available_languages.items():
    markdown = ''
    status = ''

    if langid in reports:
        print(f'Generating report for {lang.name} ({langid})...')
        status = 'Incomplete'
        for filename, problems in reports[langid].items():
            markdown += f'## [{filename}](https://github.com/alliedmodders/sourcemod/blob/master/translations/{langid}/{filename})\n'
            added_phrase_warning = False
            for report in problems:
                if report.file_warning:
                    markdown += f'**{report.file_warning}**\n'
                    print(f'  {report.file_warning} ({report.filename})')
                if report.phrase_warning:
                    if not added_phrase_warning:
                        markdown += '| Phrase | Issue |\n| ------- | --------- |\n'
                        added_phrase_warning = True
                    markdown += f'| `{report.phrase_key}` | {report.phrase_warning} |\n'
                    print(
                        f'  {report.filename}: "{report.phrase_key}" -> {report.phrase_warning}'
                    )
            markdown += '\n'
    else:
        status = 'Complete'
        markdown = 'No issues found'

    print(f'Updating draft issue for {lang.name} ({langid})...')
    issue = next(
        (x for x in draft_issues if x['content']['title'] == lang.name), None)
    if issue is None:
        issue = githubgql.add_draft_issue(project_id, lang.name, markdown)
    else:
        githubgql.update_draft_issue(issue['content']['id'], lang.name,
                                     markdown)
    githubgql.update_item_field_value_option(project_id, issue['id'],
                                             status_field_id,
                                             status_field_option_ids[status])
