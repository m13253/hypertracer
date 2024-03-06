#!/usr/bin/env python3

import cbor2  # pip install cbor2
import json
import sys
from typing import Any, BinaryIO, Dict, TextIO, Optional


def convert(file_in: BinaryIO, file_out: TextIO) -> None:
    root = set()
    cache: Dict[Optional['cbor2.CBORTag'], Any] = {None: root}

    def process_value(value: Any) -> Any:
        if isinstance(value, list) and isinstance(value[0], cbor2.CBORTag) and value[0].tag == 39:
            child_id, child = value
            cache[child_id] = child
            return child
        return value

    file_out.write('[')
    line_no = 0
    for entry in cbor2.load(file_in):
        entry = list(entry)
        if len(entry) == 1:
            value_id = entry[0]
            value = cache[value_id]
            if id(value) in root:
                root.remove(id(value))
                file_out.write(',\n    ' if line_no != 0 else '\n    ')
                line_no += 1
                custom_json_serializer(value, file_out)
            del cache[value_id]
        else:
            parent_id, container = entry
            parent = cache[parent_id]
            if isinstance(container, list):
                for value in container:
                    value = process_value(value)
                    if isinstance(parent, list):
                        parent.append(value)
                    elif isinstance(parent, set):
                        parent.add(id(value))
                    else:
                        raise TypeError(f'invalid conainer type, expect list, got {type(parent)}')
            elif isinstance(container, dict):
                for key, value in container.items():
                    value = process_value(value)
                    if isinstance(parent, dict):
                        parent[key] = value
                    else:
                        raise TypeError(f'invalid conainer type, expect dict, got {type(parent)}')
    file_out.write(']\n')


def custom_json_serializer(obj: Any, file_out: TextIO) -> None:
    circular_check = set()

    def serialize(obj):
        if isinstance(obj, (list, set)):
            if id(obj) in circular_check:
                file_out.write('...')
                return
            circular_check.add(id(obj))
            file_out.write('[')
            for i, v in enumerate(obj):
                if i != 0:
                    file_out.write(', ')
                serialize(v)
            file_out.write(']')
        elif isinstance(obj, dict):
            if id(obj) in circular_check:
                file_out.write('...')
                return
            circular_check.add(id(obj))
            file_out.write('{')
            for i, (k, v) in enumerate(obj.items()):
                if i != 0:
                    file_out.write(', ')
                serialize(k)
                file_out.write(': ')
                serialize(v)
            file_out.write('}')
        elif isinstance(obj, cbor2.CBORTag) and obj.tag == 1001 and isinstance(obj.value, dict) and len(obj.value) == 2 and 1 in obj.value and -9 in obj.value:
            sec, nsec = obj.value[1], obj.value[-9]
            msec = sec * 1000000 + nsec // 1000
            frac = nsec % 1000
            file_out.write(f'{msec}.{frac:03}')
        else:
            json.dump(obj, file_out, ensure_ascii=False, check_circular=False)

    serialize(obj)


def main(argv: list[str]) -> None:
    name_in: Optional[str] = None
    name_out: Optional[str] = None
    state = 0
    for arg in argv[1:]:
        if state == 1:
            if name_in is not None:
                raise ValueError('input file specified multiple times')
            name_in = arg
        elif state == 2:
            if name_out is not None:
                raise ValueError('output file specified multiple times')
            name_out = arg
            state = 0
        elif arg == '--':
            state = 1
        elif arg == '-?' or arg == '--help':
            return print_help(argv[0])
        elif arg == '-o' or arg == '--output':
            state = 2
        elif arg.startswith('-o'):
            if name_out is not None:
                raise ValueError('output file specified multiple times')
            name_out = arg[2:]
        elif arg.startswith('--output='):
            if name_out is not None:
                raise ValueError('output file specified multiple times')
            name_out = arg[9:]
        else:
            if name_in is not None:
                raise ValueError('input file specified multiple times')
            name_in = arg
    if state == 2:
        raise ValueError('option -o is missing argument')
    if name_in is None:
        return print_help(argv[0])
    if name_out is not None:
        with (
            open(name_in, 'rb') as file_in,
            open(name_out, 'w') as file_out
        ):
            return convert(file_in, file_out)
    else:
        with open(name_in, 'rb') as file_in:
            return convert(file_in, sys.stdout)

    
def print_help(program_name: str) -> None:
    print(f'Usage {program_name} INPUT.trace [-o OUTPUT.trace]')
    print()


if __name__ == '__main__':
    main(sys.argv)
