import os

env = Environment(ENV = os.environ)
env.VariantDir('build', 'src')
source = Split("""
main.cpp
""")

env.Program('dodgeball', ['build/%s' % x for x in source])
