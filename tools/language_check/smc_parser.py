#!/usr/bin/python3

# BSD Zero Clause License
# 
# Copyright (C) 2023 by nosoop
# 
# Permission to use, copy, modify, and/or distribute this software for any purpose with or
# without fee is hereby granted.
# 
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
# SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL
# THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY
# DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
# CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE
# OR PERFORMANCE OF THIS SOFTWARE.

# https://gist.github.com/nosoop/8c6ccaec11b1d33340bec8dbc8096658
import collections
import enum
import itertools


class SMCOperation(enum.Enum):
    STRING = 1
    SUBSECTION_START = 2
    SUBSECTION_END = 3
    COMMENT = 4
    COMMENT_MULTILINE = 5
    KEYVALUE = 6


# https://stackoverflow.com/a/70762559
def takewhile_inclusive(predicate, it):
    for x in it:
        if predicate(x):
            yield x
        else:
            yield x
            break


def _is_whitespace(ch):
    return ch in (' ', '\t', '\n', '\r')


def _smc_stream_skip_whitespace(stream):
    # consumes whitespace and returns the first non-whitespace character if any, or None if EOS
    values = tuple(takewhile_inclusive(_is_whitespace, stream))
    if not values:
        return None
    *ws, last = values
    if not ws and not _is_whitespace(last):
        return last
    return last if ws and not _is_whitespace(last) else None


def _smc_stream_extract_multiline_comment(stream):
    while True:
        yield from itertools.takewhile(lambda ch: ch != '*', stream)
        ch = next(stream, None)
        if ch == '/':
            return
        yield '*'
        yield ch


_escape_mapping = str.maketrans({
    '"': '"',
    'n': '\n',
    'r': '\r',
    't': '\t',
    '\\': '\\',
})


def _smc_stream_extract_string(stream):
    for ch in stream:
        if ch == "\\":
            ch = next(stream).translate(_escape_mapping)
        elif ch == '"':
            return
        yield ch


def parse_smc_string(data):
    stream = iter(data)
    while True:
        ch = _smc_stream_skip_whitespace(stream)
        if ch is None:
            return
        elif ch == '"':
            # consume until the next quote, then determine if:
            # - the string marks the subsection name '{'
            # - we have another string to consume, making this a key / value pair
            key = ''.join(_smc_stream_extract_string(stream))

            ch = _smc_stream_skip_whitespace(stream)
            if ch == '{':
                yield SMCOperation.SUBSECTION_START, key
            elif ch == '"':
                value = ''.join(_smc_stream_extract_string(stream))
                yield SMCOperation.KEYVALUE, key, value
            else:
                raise ValueError(
                    f"Unexpected character {ch.encode('ascii', 'backslashreplace')} after end of string"
                )
        elif ch == '}':
            yield SMCOperation.SUBSECTION_END, None
        elif ch == '/':
            ch = next(stream)
            if ch == '/':
                # single line comment: consume until the end of the line
                value = ''.join(
                    itertools.takewhile(lambda ch: ch != '\n', stream))
                yield SMCOperation.COMMENT, value
            elif ch == '*':
                # multi line comment: consume until the sequence '*/' is reached
                value = ''.join(_smc_stream_extract_multiline_comment(stream))
                yield SMCOperation.COMMENT_MULTILINE, value
            else:
                raise ValueError(
                    f"Unexpected character {ch.encode('ascii', 'backslashreplace')} at start of comment"
                )
        else:
            raise ValueError(
                f"Unexpected character {ch.encode('ascii', 'backslashreplace')}"
            )


class MultiKeyDict(collections.defaultdict):
    # a dict that supports supports one-to-many mappings
    # init by passing keys pointing to a list of values
    def __init__(self, *args, **kwargs):
        super().__init__(list, *args, **kwargs)

    # yields a key, value pair for every array item associated with a key
    def items(self):
        yield from ((k, iv) for k, v in super().items() for iv in v)


def smc_string_to_dict(data):
    # returns a multidict instance
    root_node = MultiKeyDict()
    contexts = [root_node]
    for event, *info in parse_smc_string(data):
        if event == SMCOperation.SUBSECTION_START:
            key, *_ = info
            subkey = MultiKeyDict()
            contexts[-1][key].append(subkey)
            contexts.append(subkey)
        elif event == SMCOperation.SUBSECTION_END:
            contexts.pop()
        elif event == SMCOperation.KEYVALUE:
            key, value = info
            contexts[-1][key].append(value)
    return root_node


def main():
    SMC_STRING = """
	"thing"
	{
		// this is a comment node
		"key"	"value"
		
		"subthing"
		{
			// and another
			"subthing key"		"subthing value"
			"subthing key"		"duplicate key value"
		}
		"subthing"
		{
			"duplicate subthing" "yes"
		}
		
		/**
		 * this is a multiline comment node
		 */
		"another key"	"another value"
	}
	"""

    # sections = []
    # for event, *data in parse_smc_string(SMC_STRING):
    # 	print(event, data, tuple(sections))
    # 	if event == SMCOperation.SUBSECTION_START:
    # 		section, *_ = data
    # 		sections.append(section)
    # 	elif event == SMCOperation.SUBSECTION_END:
    # 		sections.pop()
    # assert(not sections)

    import json
    import pathlib
    # print(json.dumps(smc_string_to_dict(SMC_STRING), indent=4))
    for f in pathlib.Path('translations').rglob('*.txt'):
        print(f)
        print(json.dumps(smc_string_to_dict(f.read_text('utf8')), indent = 4))


if __name__ == "__main__":
    main()
