#!/usr/bin/python3
# Copyright (c) 2023 Peace-Maker
import pathlib
from smc_parser import smc_string_to_dict

# Parse the languages.cfg file to know which languages could be available
had_problem = False
file_count = 0
languages_cfg = smc_string_to_dict(
    pathlib.Path('../../configs/languages.cfg').read_text('utf-8'))
print(f'Checking {len(languages_cfg["Languages"][0])} languages...')

# Try to parse all the files as a simple smoke test for syntax errors
for langid, lang in languages_cfg['Languages'][0].items():
    if langid == 'en':
        path = '../../translations'
    else:
        path = f'../../translations/{langid}'

    for file in pathlib.Path(path).glob('*.txt'):
        if not file.is_file():
            continue

        file_count += 1
        try:
            phrases = smc_string_to_dict(file.read_text('utf-8'))
            if 'Phrases' not in phrases:
                print(
                    f'Error in {langid}/{file.name}: File does not start with a "Phrases" section'
                )
                had_problem = True
                continue
        except Exception as ex:
            print(f'Error in {langid}/{file.name}: Error parsing: {ex}')
            had_problem = True
            continue

print(f'Checked {file_count} files.')

if had_problem:
    print('Sanity check failed!')
    exit(1)
