# Configuration file for the Sphinx documentation builder.
#
# This file only contains a selection of the most common options. For a full
# list see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Path setup --------------------------------------------------------------

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
#
# import os
# import sys
# sys.path.insert(0, os.path.abspath('.'))

# -- Project information -----------------------------------------------------

project = 'iCon'
copyright = '2022, Fideltronik'
author = 'Mateusz Fiolek'

# The full version, including alpha/beta/rc tags
release = 'v1.0.1'

# -- General configuration ---------------------------------------------------

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = ['myst_parser']

# Add any paths that contain templates here, relative to this directory.
templates_path = ['_templates']

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = []

# -- Options for HTML output -------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#
html_theme = 'sphinx_rtd_theme'

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = ['_static']

html_css_files = [
    'custom.css',
]

html_logo = 'images/logo_vertical_transparent_web.png'

# -- Options for LaTeX output ---------------------------------------------

latex_engine = 'pdflatex'

# For "manual" documents, if this is true, then toplevel headings are parts, not chapters.
latex_use_parts = False

latex_elements = {
    # The paper size ('letter' or 'a4').
    'papersize': 'a4',

    # remove blank pages (between the title page and the TOC, etc.)
    'classoptions': ',openany,oneside',

    # Figures positioning
    'figure_align':'htbp',

    # The font size ('10pt', '11pt' or '12pt')
    'pointsize': '11pt',

    # Additional stuff for the LaTeX preamble.
    'preamble': r'''
    \usepackage{hyperref}
    \setcounter{tocdepth}{2}
    '''
}

# If false, no module index is generated.
latex_use_modindex = True

# This value determines the topmost sectioning unit. It should be chosen from 'part', 'chapter' or 'section'
latex_toplevel_sectioning = 'chapter'

latex_logo = 'images/logo_vertical_transparent_web.png'
