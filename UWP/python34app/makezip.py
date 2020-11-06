#!python3.4
import compileall, os, zipfile, imp, sys
force = False
#force = True

print(sys.version)

imp_suffix = "."+imp.get_tag()+".pyc"

try:
    os.unlink('python34app.zip')
except Exception:
    pass
out = zipfile.ZipFile('python34app.zip', 'w')#, compression=zipfile.ZIP_DEFLATED)
prefix = '../../Lib/'
for dir, subdirs, files in os.walk(prefix):
  for d in subdirs[:]:
    if d in ('test', 'tests') or d.startswith('plat-'):
      subdirs.remove(d)
  ddir = dir[len(prefix):]
  for f in files:
    if (f.endswith('.py') and f != '__phello__.foo.py'):
        compileall.compile_file(os.path.join(dir, f), ddir=ddir, force=force)
        out.write(os.path.join(dir, "__pycache__", f[:-3]+imp_suffix), ddir+"/"+f+"c")
    elif f.endswith('.cfg'):
        out.write(os.path.join(dir, f), ddir+"/"+f)
    else:
	    continue
out.close()

