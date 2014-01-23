# vim: set ts=8 sts=2 sw=2 tw=99 et:
import re
import os, sys
import subprocess

argv = sys.argv[1:]
if len(argv) < 2:
  sys.stderr.write('Usage: generate_headers.py <source_path> <output_folder>\n')
  sys.exit(1)

SourceFolder = os.path.abspath(os.path.normpath(argv[0]))
OutputFolder = os.path.normpath(argv[1])

def get_hg_version():
  argv = ['hg', 'parent', '-R', SourceFolder]

  # Python 2.6 doesn't have check_output.
  if hasattr(subprocess, 'check_output'):
    text = subprocess.check_output(argv)
    if str != bytes:
      text = str(text, 'utf-8')
  else:
    p = subprocess.Popen(argv, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    output, ignored = p.communicate()
    rval = p.poll()
    if rval:
      raise subprocess.CalledProcessError(rval, argv)
    text = output.decode('utf8')

  m = re.match('changeset:\s+(\d+):(.+)', text)
  if m == None:
    raise Exception('Could not determine repository version')
  return m.groups()

def output_version_headers():
  rev, cset = get_hg_version()

  with open(os.path.join(SourceFolder, 'product.version')) as fp:
    contents = fp.read()
  m = re.match('(\d+)\.(\d+)\.(\d+)-?(.*)', contents)
  if m == None:
    raise Exception('Could not detremine product version')
  major, minor, release, tag = m.groups()
  fullstring = "{0}.{1}.{2}".format(major, minor, release)
  if tag != "":
    fullstring += "-{0}".format(tag)
    if tag == "dev":
      fullstring += "+{0}".format(rev)

  with open(os.path.join(OutputFolder, 'sourcemod_version_auto.h'), 'w') as fp:
    fp.write("""
#ifndef _SOURCEMOD_AUTO_VERSION_INFORMATION_H_
#define _SOURCEMOD_AUTO_VERSION_INFORMATION_H_

#define SM_BUILD_TAG		\"{0}\"
#define SM_BUILD_REV		\"{1}\"
#define SM_BUILD_CSET		\"{2}\"
#define SM_BUILD_MAJOR		\"{3}\"
#define SM_BUILD_MINOR		\"{4}\"
#define SM_BUILD_RELEASE	\"{5}\"

#define SM_BUILD_UNIQUEID SM_BUILD_REV \":\" SM_BUILD_CSET

#define SM_VERSION_STRING	\"{6}\"
#define SM_VERSION_FILE		{7},{8},{9},0

#endif /* _SOURCEMOD_AUTO_VERSION_INFORMATION_H_ */
    """.format(tag, rev, cset, major, minor, release, fullstring, major, minor, release))

  with open(os.path.join(OutputFolder, 'version_auto.inc'), 'w') as fp:
    fp.write("""
#if defined _auto_version_included
 #endinput
#endif
#define _auto_version_included

#define SOURCEMOD_V_TAG		\"{0}\"
#define SOURCEMOD_V_REV		{1}
#define SOURCEMOD_V_CSET	\"{2}\"
#define SOURCEMOD_V_MAJOR	{3}
#define SOURCEMOD_V_MINOR	{4}
#define SOURCEMOD_V_RELEASE	{5}

#define SOURCEMOD_VERSION	\"{6}\"
    """.format(tag, rev, cset, major, minor, release, fullstring))

output_version_headers()
