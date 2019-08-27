from distutils.core import setup, Extension
from distutils.command.clean import clean
from glob import glob
import os

module1 = Extension('samuel.engine',
     sources = ['engine/enginemodule.cpp',
                'engine/ai.cpp',                
                ])

def read(fname):
    return open(os.path.join(os.path.dirname(__file__), fname)).read()

def create_localised_files():
    mo_files = []
    # os.system('bash create_po.sh')
    os.system('make -C po')
    os.system('make -C po DESTDIR=../ install')
    mo_files.append(('share/locale/de/LC_MESSAGES/', ['locale/de/LC_MESSAGES/samuel.mo']))
    mo_files.append(('share/locale/en/LC_MESSAGES/', ['locale/en/LC_MESSAGES/samuel.mo']))
    mo_files.append(('share/locale/es/LC_MESSAGES/', ['locale/es/LC_MESSAGES/samuel.mo']))
    mo_files.append(('share/locale/fr/LC_MESSAGES/', ['locale/fr/LC_MESSAGES/samuel.mo']))
    mo_files.append(('share/locale/it/LC_MESSAGES/', ['locale/it/LC_MESSAGES/samuel.mo']))
    mo_files.append(('share/locale/nl/LC_MESSAGES/', ['locale/nl/LC_MESSAGES/samuel.mo']))
    mo_files.append(('share/applications',['desktop/samuel.desktop']))
    return mo_files

class CleanFiles(clean):
    def run(self):
        super().run()
        cmd_list = dict(
            po_clean='make -C po clean',
            irm_locale_and_desktop='rm -rf locale samuel.desktop'
        )
        for key, cmd in cmd_list.items():
            os.system(cmd)


setup (name = 'samuel',
    version = '0.1.9',
    description = 'A Draughts Program',
    ext_modules = [module1],
    author='John Cheetham',
    author_email='developer@johncheetham.com',  
    url='http://www.johncheetham.com/projects/samuel/', 
    long_description=read("README.rst"),
    platforms = ['Linux'],

    license = "GPLv3+",

    packages=['samuel'],

    cmdclass={
        'clean': CleanFiles,
    },

    classifiers=[
          'Development Status :: 4 - Beta',
          'Environment :: X11 Applications :: GTK',
          'Intended Audience :: End Users/Desktop',                
          'License :: OSI Approved :: GNU General Public License (GPL)',
          'Operating System :: POSIX :: Linux',          
          'Programming Language :: Python :: 3',
          'Programming Language :: C++',
          'Topic :: Games/Entertainment :: Board Games',
          ],
    data_files = [("share/samuel/images", glob('images/*.png')),
            ("share/samuel/data", ['data/2pc.cdb', 'data/3pc.cdb', 'data/4pc.cdb', 'data/opening.gbk']),
            ("share/doc/samuel-0.1.9", ["README.rst", "LICENSE"]),
            ('share/pixmaps', ['samuel.png']),
    ]+ create_localised_files(),
    scripts = [
        'scripts/samuel'
    ]
               
    )
