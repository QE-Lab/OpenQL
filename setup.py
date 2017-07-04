import os
from setuptools import setup
from shutil import copyfile
import subprocess
from sys import platform

rootDir = os.path.dirname(os.path.realpath(__file__))
buildDir = os.path.join(rootDir, "cbuild")
clibDir = os.path.join(buildDir, "openql")

if not os.path.exists(buildDir):
    os.makedirs(buildDir)
os.chdir(buildDir)

if platform == "linux" or platform == "linux2":
    print('Detected Linux OS, compiling ...')
    cmd = 'cmake ..'
    ret = subprocess.check_output(cmd, shell=True)
    cmd = 'make'
    ret = subprocess.check_output(cmd, shell=True)
    clibname = "_openql.so"

elif platform == "darwin":
    print('Detected OSX, compiling ...')
    cmd = 'cmake ..'
    ret = subprocess.check_output(cmd, shell=True)
    cmd = 'make'
    ret = subprocess.check_output(cmd, shell=True)
    clibname = "_openql.so"

elif platform == "win32":
    print('Detected Windows OS, compiling ...')
    cmd = 'cmake -G "NMake Makefiles" ..'
    ret = subprocess.check_output(cmd, shell=True)
    cmd = 'nmake'
    ret = subprocess.check_output(cmd, shell=True)
    clibname = "_openql.pyd"

else:
    print('Unknown/Unsupported OS !!!')

genclib = os.path.join(clibDir, clibname)
clib = os.path.join(rootDir, "openql", clibname)
copyfile(genclib, clib)
copyfile(os.path.join(clibDir, "openql.py"),
         os.path.join(rootDir, "openql", "openql.py"))
os.chdir(rootDir)


def read(fname):
    return open(os.path.join(os.path.dirname(__file__), fname)).read()


setup(name='openql',
      version='0.1',
      description='OpenQL Python Package',
      long_description=read('README.md'),
      author='I. Ashraf',
      author_email='iimran.aashraf@gmail.com',
      url="https://github.com/DiCarloLab-Delft/OpenQL",
      packages=['openql'],
      include_package_data=True,
      package_data={'openql': [clib]},
      zip_safe=False)

# os.remove(os.path.join(rootDir, "openql", "openql.py"))
# os.remove(clib)
