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

request = urllib.Request(SYMBOL_SERVER, out)
request.add_header('Content-Type', 'text/plain')
server_response = urllib.urlopen(request).read().decode('utf8')
print(server_response)
