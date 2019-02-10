from distutils.core import setup, Extension
from glob import glob
import os

module1 = Extension('samuel.engine',
     sources = ['engine/enginemodule.cpp',
                'engine/ai.cpp',                
                ])

def read(fname):
    return open(os.path.join(os.path.dirname(__file__), fname)).read()

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
            ("share/doc/samuel-0.1.9", ["README", "LICENSE"]),
            ('share/applications',['samuel.desktop']),
            ('share/pixmaps', ['samuel.png']),
    ],    
    scripts = [
        'scripts/samuel'
    ]
               
    )
