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
  fp.write(out)
  fp.write(err)

lines = out.splitlines()

paths = set()
roots = {}

# Lets not even talk about this.
def fixWindowsPath(path):
  import ctypes
  GetShortPathName = ctypes.windll.kernel32.GetShortPathNameW
  shortp = ctypes.create_unicode_buffer(260)
  rv = GetShortPathName(path.capitalize(), shortp, 260)
  if rv == 0 or rv > 260:
    return path
  GetLongPathName = ctypes.windll.kernel32.GetLongPathNameW
  longp = ctypes.create_unicode_buffer(260)
  rv = GetLongPathName(shortp, longp, 260)
  if rv == 0 or rv > 260:
    return path
  return longp.value

for i, line in enumerate(lines):
  line = line.strip().split(None, 2)

  if line[0] != 'FILE':
    continue

  path = line[2]

  if os.name == 'nt' and os.path.exists(path):
    path = fixWindowsPath(path)
    line = ' '.join(['FILE', line[1], path])
    lines[i] = line

  path = os.path.dirname(path)
  if path in paths:
    continue

  paths.add(path)

  root = None
  url = None
  rev = None

  with open(os.devnull, 'w') as devnull:
    def runCommand(argv):
      proc = subprocess.Popen(argv, stdout=subprocess.PIPE, stderr=devnull, cwd=path, universal_newlines=True)
      procout, procerr = proc.communicate()
      if proc.returncode:
        raise RuntimeError('Failed to execute \'' + ' '.join(argv) + '\' = ' + str(proc.returncode))
      return procout.strip()

    try:
      root = runCommand(['git', 'rev-parse', '--show-toplevel'])
      root = os.path.normpath(root)

      if root in roots:
        continue

      url = runCommand(['git', 'ls-remote', '--get-url', 'origin'])
      rev = runCommand(['git', 'log', '--pretty=format:%H', '-n', '1'])
    except (OSError, RuntimeError) as e:
      #sys.stderr.write(str(e) + '\n')
      continue

  roots[root] = (url, rev)

index = 1
while lines[index].split(None, 1)[0] == 'INFO':
  index += 1;

for root, info in roots.items():
  lines.insert(index, 'INFO REPO ' + ' '.join([info[1], info[0], root]))
  index += 1;

out = os.linesep.join(lines).encode('utf8')

request = urllib.Request(SYMBOL_SERVER, out)
request.add_header('Content-Type', 'text/plain')
server_response = urllib.urlopen(request).read().decode('utf8').strip()
print(server_response)
