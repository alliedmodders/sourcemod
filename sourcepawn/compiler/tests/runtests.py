# vim: set ts=4 sw=4 tw=99 et:
import os, sys
import argparse
import subprocess

def run_tests(args):
    testdir = os.path.dirname(os.path.abspath(__file__))
    tests = []
    for filename in os.listdir(testdir):
        base, ext = os.path.splitext(filename)
        if ext == '.sp':
            tests += [base]

    failed = False

    for test in tests:
        if test.startswith('fail-'):
            kind = 'fail'
        elif test.startswith('warn-'):
            kind = 'warn'
        elif test.startswith('ok-'):
            kind = 'pass'

        try:
            argv = [os.path.abspath(args.spcomp), os.path.join(testdir, test + '.sp')]
            p = subprocess.Popen(argv, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            stdout, stderr = p.communicate()
            stdout = stdout.decode('utf-8')
            stderr = stderr.decode('utf-8')

            smx_path = test + '.smx'
            compiled = os.path.exists(smx_path)
            if compiled:
                os.unlink(smx_path)

            status = 'ok'
            if compiled and kind == 'fail':
                status = 'fail'
            elif not compiled and kind != 'fail':
                status = 'fail'

            fails = []
            if status == 'ok' and kind != 'pass':
                lines = []
                with open(os.path.join(testdir, test + '.txt')) as fp:
                    for line in fp:
                        lines.append(line.strip())
                for line in lines:
                    if line not in stdout:
                        fails += [
                            'Expected to find text in stdout: >>>\n',
                            line,
                            '<<<\n',
                        ]
                        break
            
            if status == 'fail' or len(fails):
                print('Test {0} ... FAIL'.format(test))
                failed = True
                sys.stderr.write('FAILED! Dumping stdout/stderr:\n')
                sys.stderr.write(stdout)
                sys.stderr.write(stderr)
                for line in fails:
                    sys.stderr.write(line)
            else:
                print('Test {0} ... OK'.format(test))

        except Exception as exn:
            raise
            sys.stderr.write('FAILED! {0}\n'.format(exn.message))

    if failed:
        sys.stderr.write('One or more tests failed!\n')
        sys.exit(1)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('spcomp', type=str, help='Path to spcomp')
    args = parser.parse_args()
    run_tests(args)

if __name__ == '__main__':
    main()
