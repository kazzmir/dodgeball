import os, sys

sys.path.append('scons')
import helpers

env = Environment(ENV = os.environ)
env.VariantDir('build', 'src')
source = Split("""
main.cpp
world.cpp
""")

env.Append(CCFLAGS = ['-g3'])

env.Append(CPPPATH = '#build')
env['LINKCOM'] = '$CXX $LINKFLAGS $SOURCES -Wl,--start-group $ARCHIVES $_LIBDIRFLAGS $_LIBFLAGS -Wl,--end-group -o $TARGET'

env.ParseConfig('sdl-config --libs --cflags')
env.ParseConfig('freetype-config --libs --cflags')
env.ParseConfig('libpng-config --libs --cflags')

env.Append(CPPDEFINES = ['USE_SDL'])
options = {'sdl': True}
archives = env.SConscript('build/util/SConscript', exports = ['env', 'options'])
env.Append(ARCHIVES = archives)

env.Program('dodgeball', ['build/%s' % x for x in source])
