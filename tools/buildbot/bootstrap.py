# vim: set sw=4 sts=4 ts=4 sw=99 et:
from contextlib import contextmanager
import argparse
import json
import os
import platform
import shutil
import subprocess

@contextmanager
def Chdir(path):
    old = os.getcwd()
    os.chdir(path)
    print('> cd {} # {}'.format(path, os.getcwd()))
    try:
        yield
    finally:
        os.chdir(old)

def run_shell(argv, env = None):
    if type(argv) is str:
        print('> {}'.format(argv))
        shell = True
    else:
        print('> {}'.format(' '.join(argv)))
        shell = False
    try:
        output = subprocess.check_output(argv, stderr = subprocess.STDOUT, env = env, shell = shell)
    except subprocess.CalledProcessError as cpe:
        if not shell:
            print(cpe.output.decode('utf-8', 'ignore'))
        raise
    print(output.decode('utf-8', 'ignore'))

def output_needs_cleaning():
    if not os.path.isdir('OUTPUT'):
        return False
    amb2_dir = os.path.join('OUTPUT', '.ambuild2')
    if not os.path.exists(os.path.join(amb2_dir, 'graph')):
        return True
    if not os.path.exists(os.path.join(amb2_dir, 'vars')):
        return True
    return False

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--config', type = str, required = True,
                        help = 'Buildbot slave name')
    parser.add_argument('--hl2sdk-root', type = str, required = True,
                        help = 'hl2sdk collection')
    parser.add_argument('--python-cmd', type = str, required = True,
                        default = 'python3',
                        help = 'Python command')
    parser.add_argument('--mms-path', type = str, required = True,
                        help = 'Metamod:Source path')
    args = parser.parse_args()

    run_shell("git submodule update --init --recursive")

    source = os.getcwd()

    with open(os.path.join('tools', 'buildbot', 'buildconfig.json'), 'rt') as fp:
        config_root = json.load(fp)

    config = config_root.get(args.config, {})

    # Set build properties.
    build_env = os.environ.copy()
    for env_var in ['CC', 'CXX']:
        if env_var not in config:
            continue
        build_env[env_var] = config[env_var]

    config_argv = [
        args.python_cmd,
        os.path.join(source, 'configure.py'),
        '--enable-optimize',
        '--breakpad-dump',
        '--no-color',
        '--symbol-files',
        '--targets=x86,x86_64',
        '--sdks=all',
        '--out=OUTPUT',
        '--hl2sdk-root={}'.format(args.hl2sdk_root),
        '--mms-path={}'.format(args.mms_path),
    ]

    print("Attempting to reconfigure...")

    with Chdir('..'):
        if output_needs_cleaning():
            print("Cleaning output directory...")
            shutil.rmtree('OUTPUT')
        if not os.path.isdir('OUTPUT'):
            print("Creating output directory...")
            os.mkdir('OUTPUT')
        print("Running configure.py...")
        run_shell(config_argv, env = build_env)

if __name__ == '__main__':
    main()
