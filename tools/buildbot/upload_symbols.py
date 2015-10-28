# vim: ts=8 sts=2 sw=2 tw=99 et ft=python: 
import sys
import subprocess
import os
try:
  import urllib.request as urllib
except ImportError:
  import urllib2 as urllib

if len(sys.argv) < 3:
  sys.stderr.write('Usage: <symbol-file> <dump-syms-cmd> <args...>\n')
  sys.exit(1)

SYMBOL_SERVER = os.environ['BREAKPAD_SYMBOL_SERVER']
symbol_file = sys.argv[1]
cmd_argv = sys.argv[2:]

sys.stdout.write(' '.join(cmd_argv))
sys.stdout.write('\n')

p = subprocess.Popen(
  args = cmd_argv,
  stdout = subprocess.PIPE,
  stderr = subprocess.PIPE,
  shell = False
)
stdout, stderr = p.communicate()
out = stdout.decode('utf8')
err = stdout.decode('utf8')

with open(symbol_file, 'w') as fp:
  fp.write(stdout)
  fp.write(stderr)

lines = out.splitlines()

paths = set()
roots = {}

for line in lines:
  line = line.strip().split(None, 2)

  if line[0] != 'FILE':
    continue

  path = os.path.dirname(line[2])

  if path in paths:
    continue

  paths.add(path)

  root = None
  url = None
  rev = None

  with open(os.devnull, 'w') as devnull:
    try:
      root = subprocess.check_output(['git', 'rev-parse', '--show-toplevel'], stderr=devnull, cwd=path, universal_newlines=True).strip()
      root = os.path.normcase(root)

      if root in roots:
        continue

      url = subprocess.check_output(['git', 'ls-remote', '--get-url', 'origin'], stderr=devnull, cwd=path, universal_newlines=True).strip()
      rev = subprocess.check_output(['git', 'log', '--pretty=format:%H', '-n', '1'], stderr=devnull, cwd=path, universal_newlines=True).strip()
    except (OSError, subprocess.CalledProcessError):
      continue

  roots[root] = (url, rev)

index = 1
while lines[index].split(None, 1)[0] == 'INFO':
  index += 1;

for root, info in roots.items():
  lines.insert(index, 'INFO REPO ' + ' '.join([info[1], info[0], root]))
  index += 1;

out = os.linesep.join(lines)

request = urllib.Request(SYMBOL_SERVER, out)
request.add_header('Content-Type', 'text/plain')
server_response = urllib.urlopen(request).read().decode('utf8').strip()
print(server_response)
