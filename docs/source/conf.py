# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = 'OpenSD'
copyright = '2024, OpenSD contributors'
release = '0.1.0'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
    'sphinx.ext.mathjax',
    'nbsphinx', 
    'sphinx.ext.autodoc',
    'sphinxcontrib.katex',
    'sphinx_numfig',
    'sphinxcontrib.bibtex'
]

nbsphinx_execute = 'never'
nbsphinx_allow_errors = True

source_suffix = ['.rst', '.md']

templates_path = ['_templates']
exclude_patterns = ['_build', '**.ipynb_checkpoints']
bibtex_bibfiles = ['references.bib']


# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'sphinx_rtd_theme'
