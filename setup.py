#!/usr/bin/env python

import glob
import sys

from setuptools import setup, find_packages
from Cython.Build import cythonize


# Determine shared library suffix
if sys.platform == 'darwin':
    suffix = 'dylib'
else:
    suffix = 'so'

# Get version information from __init__.py. This is ugly, but more reliable than
# using an import.
with open('opensd/__init__.py', 'r') as f:
    version = f.readlines()[-1].split()[-1].strip("'")

kwargs = {
    'name': 'opensd',
    'version': version,
    'packages': find_packages(exclude=['tests*']),
    # 'scripts': glob.glob('scripts/openmc-*'),

    # # Data files and libraries
    # 'package_data': {
        # 'openmc.lib': ['libopenmc.{}'.format(suffix)],
        # 'openmc.data': ['mass_1.mas20.txt', 'BREMX.DAT', 'half_life.json', '*.h5'],
        # 'openmc.data.effective_dose': ['*.txt']
    # },

    # Metadata
    'author': 'VikramGovindarajan',
    'author_email': 'vickymailbuddy@gmail.com',
    'description': 'OpenSD',
    # 'url': 'https://openmc.org',
    # 'download_url': 'https://github.com/VikramGovindarajan/opensd/releases',
    'project_urls': {
        # 'Issue Tracker': 'https://github.com/openmc-dev/openmc/issues',
        # 'Documentation': 'https://docs.openmc.org',
        'Source Code': 'https://github.com/VikramGovindarajan/opensd',
    },
    'classifiers': [
        # 'Development Status :: 4 - Beta',
        'Intended Audience :: Developers',
        'Intended Audience :: End Users/Desktop',
        'Intended Audience :: Science/Research',
        'License :: OSI Approved :: GNU GPL Version 3',
        'Natural Language :: English',
        'Topic :: Scientific/Engineering'
        'Programming Language :: C++',
        # 'Programming Language :: Python :: 3',
        # 'Programming Language :: Python :: 3.7',
        # 'Programming Language :: Python :: 3.7',
        # 'Programming Language :: Python :: 3.8',
        # 'Programming Language :: Python :: 3.9',
        'Programming Language :: Python',
        # 'Programming Language :: Python :: 3.11',
    ],

    # Dependencies
    'python_requires': '>=3.10',
    'install_requires': [
        'numpy', 'scipy', 'ipython',
        'lxml','CoolProp',
    ],
    'extras_require': {
        'docs': ['sphinx', 'jupyter',
                 'sphinx-rtd-theme'],
        'test': ['pytest','Colorama',   
                 'PDSim'],
    },
    # # Cython is used to add resonance reconstruction and fast float_endf
    # 'ext_modules': cythonize('openmc/data/*.pyx'),
    # 'include_dirs': [np.get_include()]
}

setup(**kwargs)
