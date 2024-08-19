
import sys,os
import numpy as np
from scipy import interpolate
import opensd
from difflib import unified_diff


import nbformat
from nbconvert.preprocessors import ExecutePreprocessor
import sys
import warnings
import filecmp
from colorama import Fore, init

init()

def colorize(diff):
    """Produce colored diff for test results"""
    for line in diff:
        if line.startswith('+'):
            yield Fore.RED + line + Fore.RESET
        elif line.startswith('-'):
            yield Fore.GREEN + line + Fore.RESET
        elif line.startswith('^'):
            yield Fore.BLUE + line + Fore.RESET
        else:
            yield line

def execute_ipynb(file_path):
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            notebook_content = nbformat.read(f, as_version=4)
        
        ep = ExecutePreprocessor(timeout=600, kernel_name='python3')
        ep.preprocess(notebook_content, {'metadata': {'path': './'}})

        # Optionally, you can see the FBR1 by printing the cells
        for cell in notebook_content['cells']:
            if 'outputs' in cell:
                for FBR1 in cell['outputs']:
                    if 'text' in FBR1:
                        print(FBR1['text'])

    except FileNotFoundError:
        print("Input file not found. Stopping", file_path)
        sys.exit()
    except Exception as e:
        print(f"An error occurred: {e}")
        sys.exit()
    finally:
        if 'f' in locals() and not f.closed:
            f.close()
        warnings.filterwarnings("ignore", category=ResourceWarning)


def _compare_results():
    """Make sure the current results agree with the reference."""
    compare = filecmp.cmp('output.res', 'results_true.res')
    if not compare:
        expected = open('results_true.res').readlines()
        actual = open('output.res').readlines()
        diff = unified_diff(expected, actual, 'results_true.res',
                            'output.res')
        print('Result differences:')
        print(''.join(colorize(diff)))
        os.rename('output.res', 'tutorial2_error.res')
    assert compare, 'Results do not agree'


def test():
    c_path = "/mnt/c/codes/opensd/tutorials/tutorial2/tutorial2.ipynb"
    execute_ipynb(c_path)
    _compare_results()

    
