"""Compare deterministic macro expansions with an independent C99 preprocessor."""
import argparse
import random
import re
import subprocess
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument('--cc1', required=True)
parser.add_argument('--clang', required=True)
parser.add_argument('--scratch', required=True)
args = parser.parse_args()
scratch = Path(args.scratch)
scratch.mkdir(parents=True, exist_ok=True)

# Preserve whitespace inside literals while ignoring formatting between tokens.
pattern = re.compile(r'''"(?:\\.|[^"\\])*"|'(?:\\.|[^'\\])*'|[A-Za-z_][A-Za-z_0-9]*|(?:[0-9]|\.[0-9])(?:[A-Za-z_0-9.]|(?<=[eEpP])[+-])*|%:%:|<<=|>>=|\.\.\.|##|\+\+|--|->|&&|\|\||<=|>=|==|!=|<<|>>|[+*/%&|^=-]=|<%|%>|<:|:>|%:|\S''')
def tokens(text):
    return pattern.findall(text)

def run(command):
    result = subprocess.run(command, text=True, capture_output=True, timeout=8)
    if result.returncode:
        raise RuntimeError(f'{command}\n{result.stderr}')
    return result.stdout

definitions = '''#define A 3
#define B A
#define ID(x) x
#define ADD(x,y) ((x)+(y))
#define TWICE(x) x + x
#define STR(x) #x
#define XSTR(x) STR(x)
#define CAT(a,b) a ## b
#define XCAT(a,b) CAT(a,b)
#define ALIAS ADD
#define ARGS(...) __VA_ARGS__
#define VSTR(...) #__VA_ARGS__
#define VC(prefix,...) prefix ## __VA_ARGS__
#define SELF(x) SELF(x)
#define LEFT(x) RIGHT(x)
#define RIGHT(x) LEFT(x)
#define EMPTY
#define HASH_HASH # ## #
#define DROP(x)
#define THIS_LINE(x) __LINE__ + x
'''
cases = [
    'STR(A) XSTR(A)', 'ID(ADD)(1,2)', 'ALIAS(1,2)',
    'TWICE(TWICE(1))', 'SELF(SELF(3))', 'LEFT(1) RIGHT(2)',
    'CAT(,) CAT(,x) CAT(x,) CAT(a b,c d)',
    'CAT(+,=) CAT(1,e3) CAT(1,.5)', 'CAT(A,2) XCAT(A,2)',
    'ARGS() ARGS(1,2,(3,4))', 'VSTR(a, b,c) VC(pre,fix) VC(pre,)',
    'STR( a+\n b /* removed */ c )', r'''STR("a\n" '\t')''',
    'XSTR(HASH_HASH)', 'XSTR(a EMPTY b)', 'XSTR(a EMPTY+b)',
    'STR(%:) XSTR(CAT(%,:))', 'STR(<:x:>)',
    'XSTR(ID(a)b) XSTR(a ID(b)) XSTR(a ID(EMPTY)b)',
    'XSTR(CAT(a,b)c) XSTR(a DROP(1)b)',
    'XSTR(+ +) XSTR(CAT(+,+))',
    'THIS_LINE(\n__LINE__\n)',
    'ID(__LI\\\nNE__)',
    'ID\n(\n4\n)', '__LINE__ __FILE__',
    '__STDC__ __STDC_VERSION__ __STDC_HOSTED__',
    '#if ADD(A,B) == 6 && defined(ALIAS)\nyes\n#else\nno\n#endif',
]
rng = random.Random(1701)
def expression(depth):
    if depth == 0:
        return rng.choice(['A', 'B', '0', '7', '(1,2)'])
    choice = rng.randrange(5)
    if choice == 0:
        return f'ID({expression(depth-1)})'
    if choice == 1:
        return f'TWICE({expression(depth-1)})'
    if choice == 2:
        return f'ADD({expression(depth-1)}, {expression(depth-1)})'
    if choice == 3:
        return f'ALIAS({expression(depth-1)}, {expression(depth-1)})'
    return f'({expression(depth-1)} + {expression(depth-1)})'
for _ in range(200):
    value = expression(rng.randrange(1, 5))
    cases.append(value)
    if rng.randrange(3) == 0:
        cases.append(f'XSTR({value})')
source = scratch / 'macro-comparison.c'
source.write_text(definitions + '\n'.join(cases) + '\n')
actual = run([args.cc1, '-fno-color-diagnostics', '-E', str(source)])
expected = run([args.clang, '-std=c99', '-ffreestanding', '-E', '-P', str(source)])
(scratch / 'cc1.out').write_text(actual)
(scratch / 'clang.out').write_text(expected)
a, b = tokens(actual), tokens(expected)
if a != b:
    import difflib
    differences = '\n'.join(difflib.unified_diff(a,b,fromfile='cc1',tofile='clang',n=8))
    raise RuntimeError(f'Macro expansions differ:\n{differences}\nSee {scratch}')
print(f'{len(cases)} macro expansion cases agree with Clang')
