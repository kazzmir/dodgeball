import os, sys

sys.path.append('scons')
import helpers

env = Environment(ENV = os.environ)
env.VariantDir('build', 'src')
source = Split("""
main.cpp
world.cpp
""")

def sdlEnv(env):
    env.ParseConfig('sdl-config --libs --cflags')
    env.ParseConfig('freetype-config --libs --cflags')
    env.ParseConfig('libpng-config --libs --cflags')
    env.Append(CPPDEFINES = ['USE_SDL'])

def allegro5Env(env):
    env.ParseConfig('pkg-config allegro-5.1 allegro_ttf-5.1 allegro_primitives-5.1 allegro_font-5.1 allegro_acodec-5.1 allegro_audio-5.1 allegro_memfile-5.1 allegro_image-5.1 --libs --cflags')
    # env.ParseConfig('pkg-config allegro_monolith-5.1 --libs --cflags')
    env.ParseConfig('freetype-config --libs --cflags')
    env.ParseConfig('libpng-config --libs --cflags')
    env.Append(CPPDEFINES = ['USE_ALLEGRO5'])

env.Append(CCFLAGS = ['-g3'])
# env.Append(CCFLAGS = ['-O2'])

env.Append(CPPPATH = '#build')
env['LINKCOM'] = '$CXX $LINKFLAGS $SOURCES -Wl,--start-group $ARCHIVES $_LIBDIRFLAGS $_LIBFLAGS -Wl,--end-group -o $TARGET'

useSDL = True
useAllegro5 = False

options = {}
if useSDL:
    sdlEnv(env)
    options['sdl'] = True
elif useAllegro5:
    allegro5Env(env)
    options['allegro5'] = True

archives = env.SConscript('build/util/SConscript', exports = ['env', 'options'])
env.Append(ARCHIVES = archives)

env.Program('dodgeball', ['build/%s' % x for x in source])
